#include "mpu_task.h"

#include "espace_define.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "stdint.h"
#include "string.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/message_buffer.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "service/mpu_service.h"
#include "service/ws2812_service.h"

#include "user_util.h"

static const char* TAG = "MPU_TASK";
extern user_config_t user_config;

extern MessageBufferHandle_t xMessageBufferReqSend;
extern MessageBufferHandle_t xMessageBufferMPUOut2CNN;
mpu_command_t received_command;
EventGroupHandle_t x_mpu_event_group;

extern SemaphoreHandle_t mpu_mutex;
const int SAMPLE_START_OFFSET = 2;
int sample_offset = 0;
static double ax, ay, az, gx, gy, gz;
static float ax_f, ay_f, az_f, gx_f, gy_f, gz_f;

float* imu_data = NULL;
float* imu_data_oneshot = NULL;
int total_bytes = 0;

static char err_json[256];

static void send_imu_data_to_ws()
{
    imu_data[0] = 0;
    imu_data[1] = received_command.sample_size;
    size_t bytes_sent = xMessageBufferSend(xMessageBufferReqSend, imu_data, total_bytes, portMAX_DELAY);
    if (bytes_sent != total_bytes) {
        ESP_LOGE(TAG, "Failed to send imu_data to ws buffer, sent %d bytes, expected %d bytes.", bytes_sent, total_bytes);
    } else {
        ESP_LOGI(TAG, "Sent imu data to ws buffer, size: %d bytes", total_bytes);
    }
}

static void send_imu_data_to_tflite()
{
    for (size_t i = 0; i < received_command.sample_size; i++) {
        imu_data[SAMPLE_START_OFFSET + 0 * received_command.sample_size + i] /= 4;
        imu_data[SAMPLE_START_OFFSET + 1 * received_command.sample_size + i] /= 4;
        imu_data[SAMPLE_START_OFFSET + 2 * received_command.sample_size + i] /= 4;
        imu_data[SAMPLE_START_OFFSET + 3 * received_command.sample_size + i] /= 500;
        imu_data[SAMPLE_START_OFFSET + 4 * received_command.sample_size + i] /= 500;
        imu_data[SAMPLE_START_OFFSET + 5 * received_command.sample_size + i] /= 500;
    }

    size_t data_size = received_command.sample_size * 6 * sizeof(float);
    size_t bytes_sent = xMessageBufferSend(xMessageBufferMPUOut2CNN, imu_data + 2, data_size, portMAX_DELAY);
    if (bytes_sent != data_size) {
        ESP_LOGE(TAG, "Failed to send imu data to tflite, sent %d bytes, expected %d bytes.", bytes_sent, data_size);
    } else {
        ESP_LOGI(TAG, "Sent imu data to tflite, size: %d bytes", bytes_sent);
    }
}

static void mpu_periodic_timer_callback(void* arg)
{
    if (imu_data == NULL) {
        ESP_LOGE(TAG, "imu_data is NULL, cannot proceed with periodic callback.");
        return;
    } else if (received_command.sample_size == 0 || received_command.sample_tick == 0) {
        ESP_LOGE(TAG, "Invalid sample size or tick, cannot proceed with periodic callback.");
        return;
    }

    xSemaphoreTake(mpu_mutex, pdMS_TO_TICKS(received_command.sample_tick) || portMAX_DELAY);
    _getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    xSemaphoreGive(mpu_mutex);

    ax_f = ax;
    ay_f = ay;
    az_f = az;
    gx_f = gx;
    gy_f = gy;
    gz_f = gz;

    if (received_command.model_type == MODEL_TYPE_PERIODIC) {
        imu_data[0 * received_command.sample_size + sample_offset] = ax_f;
        imu_data[1 * received_command.sample_size + sample_offset] = ay_f;
        imu_data[2 * received_command.sample_size + sample_offset] = az_f;
        imu_data[3 * received_command.sample_size + sample_offset] = gx_f;
        imu_data[4 * received_command.sample_size + sample_offset] = gy_f;
        imu_data[5 * received_command.sample_size + sample_offset] = gz_f;
        sample_offset++;
        if (sample_offset >= received_command.sample_size + SAMPLE_START_OFFSET) {
            sample_offset = SAMPLE_START_OFFSET;
            if (received_command.send_to_ws) {
                send_imu_data_to_ws();
            }
        }

    } else if (received_command.model_type == MODEL_TYPE_ONESHOT) {
        if (sample_offset >= user_config.mpu_one_shot_max_sample_size + SAMPLE_START_OFFSET) {
            ESP_LOGE(TAG, "Reached maximum one-shot sample size, quit.");
            sample_offset = 0;
            xEventGroupSetBits(x_mpu_event_group, MPU_SAMPLING_STOP_BIT);
            strcpy(err_json, "{ \"type\": \"error\", \"requestId\": \"req_error\", \"error\": \"error.eached_maximum_oneshot_sample_size\" }");
            xMessageBufferSend(xMessageBufferReqSend, err_json, strlen(err_json), portMAX_DELAY);
            return;
        }

        imu_data_oneshot[0 * user_config.mpu_one_shot_max_sample_size + sample_offset] = ax_f;
        imu_data_oneshot[1 * user_config.mpu_one_shot_max_sample_size + sample_offset] = ay_f;
        imu_data_oneshot[2 * user_config.mpu_one_shot_max_sample_size + sample_offset] = az_f;
        imu_data_oneshot[3 * user_config.mpu_one_shot_max_sample_size + sample_offset] = gx_f;
        imu_data_oneshot[4 * user_config.mpu_one_shot_max_sample_size + sample_offset] = gy_f;
        imu_data_oneshot[5 * user_config.mpu_one_shot_max_sample_size + sample_offset] = gz_f;
        sample_offset++;
    }
}

void mpu_task(void* pvParameters)
{
    BaseType_t core_id = xPortGetCoreID(); // 返回当前任务所在的核心 ID
    ESP_LOGI(TAG, "Task is running on core %d.", core_id);

    x_mpu_event_group = xEventGroupCreate();

    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &mpu_periodic_timer_callback,
        .name = "mpu periodic"
    };
    esp_timer_handle_t mpu_periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &mpu_periodic_timer));

    mpu_mutex_init();
    reset_imu();

    int imu_data_oneshot_size = user_config.mpu_one_shot_max_sample_size * 6 * sizeof(float) + 16;
    imu_data_oneshot = (float*)malloc(imu_data_oneshot_size);
    if (imu_data_oneshot == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for imu_data_oneshot, size: %d bytes", imu_data_oneshot_size);
        vTaskDelete(NULL);
    }

    while (1) {
        EventBits_t uxBits = xEventGroupWaitBits(x_mpu_event_group,
            MPU_SAMPLING_READY_BIT | MPU_SAMPLING_UNREADY_BIT | MPU_SAMPLING_START_BIT | MPU_SAMPLING_STOP_BIT,
            pdTRUE, pdFALSE, portMAX_DELAY);
        if (uxBits & MPU_SAMPLING_READY_BIT) {
            ESP_LOGI(TAG, "Sampling ready event triggered.");
            if (received_command.need_predict) {
                ws2812_blink(get_color_by_name(received_command.model_name));
            } else {
                ws2812_blink(COLOR_MPU_SAMPLING);
            }
        } else if (uxBits & MPU_SAMPLING_UNREADY_BIT) {
            ESP_LOGI(TAG, "Sampling unready event triggered.");
            memset(&received_command, 0, sizeof(received_command));
            sample_offset = 0;
            ws2812_set_static_color(COLOR_MPU_SAMPLING_STOP);
        } else if (uxBits & MPU_SAMPLING_START_BIT) {
            ESP_LOGI(TAG, "Sampling start event triggered.");
            if (received_command.model_type >= 0 && received_command.sample_size > 0 && received_command.sample_tick > 0) {
                sample_offset = SAMPLE_START_OFFSET;
                if (imu_data != NULL) {
                    free(imu_data);
                    imu_data = NULL;
                }

                total_bytes = received_command.sample_size * 6 * sizeof(float) + SAMPLE_START_OFFSET * sizeof(float);
                imu_data = (float*)malloc(total_bytes);
                if (imu_data == NULL) {
                    ESP_LOGE(TAG, "Failed to allocate memory for imu_data, size: %d bytes", total_bytes);
                    continue;
                }
                ESP_ERROR_CHECK(esp_timer_start_periodic(mpu_periodic_timer, received_command.sample_tick * 1000));
                ESP_LOGI(TAG, "Started MPU periodic.");

            } else {
                ESP_LOGE(TAG, "Invalid or empty command parameters.");
            }
        } else if (uxBits & MPU_SAMPLING_STOP_BIT) {
            ESP_LOGI(TAG, "Sampling stop event triggered.");
            if (esp_timer_is_active(mpu_periodic_timer)) {
                ESP_ERROR_CHECK(esp_timer_stop(mpu_periodic_timer));
            } else {
                ESP_LOGI(TAG, "MPU periodic timer is not running.");
            }

            if (received_command.model_type == MODEL_TYPE_ONESHOT && sample_offset > SAMPLE_START_OFFSET) {
                int real_size = sample_offset - SAMPLE_START_OFFSET;
                ESP_LOGI(TAG, "Finish one-shot data sample, real sample size: %d, expected:%lu",
                    real_size, received_command.sample_size);
                for (int i = 0; i < 6; i++) {
                    linear_interpolation(i * user_config.mpu_one_shot_max_sample_size + imu_data_oneshot + SAMPLE_START_OFFSET, real_size,
                        i * received_command.sample_size + imu_data + SAMPLE_START_OFFSET, received_command.sample_size);
                }

                if (received_command.send_to_ws) {
                    send_imu_data_to_ws();
                }

                if (received_command.need_predict) {
                    send_imu_data_to_tflite();
                }
            }
        }
    }

    vTaskDelete(NULL);
}