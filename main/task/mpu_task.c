#include "mpu_task.h"

static const char* TAG = "MPU_TASK";
extern user_config_t user_config;

extern MessageBufferHandle_t xMessageBufferReqSend;
MessageBufferHandle_t xMessageBufferMPUOut;
MessageBufferHandle_t xMessageBufferMPUOut2CNN;
mpu_command_t received_command;
EventGroupHandle_t x_mpu_event_group;

extern SemaphoreHandle_t mpu_mutex;
const int SAMPLE_START_OFFSET = 2;
int sample_offset = SAMPLE_START_OFFSET;
static double ax, ay, az, gx, gy, gz;
static float ax_f, ay_f, az_f, gx_f, gy_f, gz_f;

float* imu_data = NULL;
int total_bytes = 0;

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

    imu_data[0 * received_command.sample_size + sample_offset] = ax_f;
    imu_data[1 * received_command.sample_size + sample_offset] = ay_f;
    imu_data[2 * received_command.sample_size + sample_offset] = az_f;
    imu_data[3 * received_command.sample_size + sample_offset] = gx_f;
    imu_data[4 * received_command.sample_size + sample_offset] = gy_f;
    imu_data[5 * received_command.sample_size + sample_offset] = gz_f;
    sample_offset++;
    if (sample_offset >= received_command.sample_size + SAMPLE_START_OFFSET) {
        sample_offset = SAMPLE_START_OFFSET;

        if (received_command.need_predict == 0) {
            if (received_command.model_type == MODEL_TYPE_ONESHOT) {

            } else if (received_command.model_type == MODEL_TYPE_PERIODIC) {
                imu_data[0] = 0;
                imu_data[1] = received_command.sample_size;
                size_t bytes_sent = xMessageBufferSend(xMessageBufferReqSend, imu_data, total_bytes, portMAX_DELAY);
                if (bytes_sent != total_bytes) {
                    ESP_LOGE(TAG, "Failed to send imu_data to ws buffer, sent %d bytes, expected %d bytes.", bytes_sent, total_bytes);
                } else {
                    ESP_LOGI(TAG, "Sent imu data to ws buffer, size: %d bytes", total_bytes);
                }
            }
        }
    }
}

void mpu_task(void* pvParameters)
{
    BaseType_t core_id = xPortGetCoreID(); // 返回当前任务所在的核心 ID
    ESP_LOGI(TAG, "Task is running on core %d.", core_id);

    x_mpu_event_group = xEventGroupCreate();
    // xMessageBufferMPUCommand = xMessageBufferCreate(user_config.mpu_command_buf_size);
    xMessageBufferMPUOut = xMessageBufferCreate(user_config.msg_buf_recv_size);
    xMessageBufferMPUOut2CNN = xMessageBufferCreate(user_config.msg_buf_recv_size);
    // configASSERT(xMessageBufferMPUCommand);
    configASSERT(xMessageBufferMPUOut);
    configASSERT(xMessageBufferMPUOut2CNN);

    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &mpu_periodic_timer_callback,
        .name = "mpu periodic"
    };
    esp_timer_handle_t mpu_periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &mpu_periodic_timer));

    mpu_mutex_init();
    reset_imu();

    while (1) {
        EventBits_t uxBits = xEventGroupWaitBits(x_mpu_event_group,
            MPU_SAMPLING_START_BIT | MPU_SAMPLING_STOP_BIT | MPU_PREDICT_START_BIT | MPU_PREDICT_STOP_BIT,
            pdTRUE, pdFALSE, portMAX_DELAY);
        if (uxBits & MPU_SAMPLING_START_BIT) {
            ESP_LOGI(TAG, "Sampling event triggered.");
            ws2812_blink(COLOR_MPU_SAMPLING);
        } else if (uxBits & MPU_SAMPLING_STOP_BIT) {
            ESP_LOGI(TAG, "Sampling stop event triggered.");
            memset(&received_command, 0, sizeof(received_command));
            ws2812_set_static_color(COLOR_MPU_SAMPLING_STOP);
        } else if (uxBits & MPU_PREDICT_START_BIT) {
            ESP_LOGI(TAG, "Predict start event triggered.");
            if (received_command.model_type > 0 && received_command.sample_size > 0 && received_command.sample_tick > 0) {
                sample_offset = SAMPLE_START_OFFSET;
                if (imu_data != NULL) {
                    free(imu_data);
                }

                total_bytes = received_command.sample_size * 6 * sizeof(float) + SAMPLE_START_OFFSET * sizeof(float);
                imu_data = malloc(total_bytes);
                if (imu_data == NULL) {
                    ESP_LOGE(TAG, "Failed to allocate memory for imu_data, size: %d bytes", total_bytes);
                    continue;
                }
                ESP_ERROR_CHECK(esp_timer_start_periodic(mpu_periodic_timer, received_command.sample_tick * 1000));
                ESP_LOGI(TAG, "Started MPU periodic.");

            } else {
                ESP_LOGE(TAG, "Invalid or empty command parameters.");
            }
        } else if (uxBits & MPU_PREDICT_STOP_BIT) {
            ESP_LOGI(TAG, "Predict stop event triggered.");
            if (esp_timer_is_active(mpu_periodic_timer)) {
                ESP_ERROR_CHECK(esp_timer_stop(mpu_periodic_timer));
            } else {
                ESP_LOGI(TAG, "MPU periodic timer is not running.");
            }
        }
    }

    vTaskDelete(NULL);
}