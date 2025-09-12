#include "json_helper.h"

#include "espace_define.h"

#include "esp_clk_tree.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

#include "stdint.h"
#include "string.h"

#include "freertos/FreeRTOS.h"

#include "hal/efuse_hal.h"
#include "hal/efuse_ll.h"

#include "driver/dac_cosine.h"

#include "peripherals/cosine_wave.h"
#include "peripherals/func_wave.h"
#include "peripherals/ledc_pwm.h"

#include "service/mpu_service.h"

#include "cJSON.h"

static const char* TAG = "JSON_HELPER";
extern user_config_t user_config;

cJSON* get_device_info(void)
{
    cJSON* data = cJSON_CreateObject();
    if (data == NULL) {
        goto get_device_info_end;
    }

    cJSON_AddStringToObject(data, "compile_time", __TIMESTAMP__);
    cJSON_AddStringToObject(data, "git_commit_id", "GIT_COMMIT_SHA1");
    cJSON_AddStringToObject(data, "firmware_version", SW_VERSION);
    cJSON_AddNumberToObject(data, "package_version", efuse_ll_get_chip_ver_pkg());
    cJSON_AddNumberToObject(data, "chip_version", efuse_hal_chip_revision());
    uint32_t cpu_freq_value;
    ESP_ERROR_CHECK(esp_clk_tree_src_get_freq_hz(SOC_MOD_CLK_CPU, ESP_CLK_TREE_SRC_FREQ_PRECISION_APPROX, &cpu_freq_value));
    cJSON_AddNumberToObject(data, "cpu_freq", cpu_freq_value);
    char idf_version_str[16];
    sprintf(idf_version_str, "%d.%d.%d", ESP_IDF_VERSION_MAJOR, ESP_IDF_VERSION_MINOR, ESP_IDF_VERSION_PATCH);
    cJSON_AddStringToObject(data, "idf_version", idf_version_str);

get_device_info_end:
    return data;
}

cJSON* get_task_state(void)
{
    // 定义一个 heap_caps_info_t 结构体来存储内存信息
    multi_heap_info_t heap_info;

    uint32_t internal_free_bytes;
    uint32_t total_free_bytes;
    uint32_t total_allocated_bytes;
    uint32_t largest_free_block;
    uint32_t minimum_free_bytes;
    uint8_t task_count = uxTaskGetNumberOfTasks(); // 获取当前任务数量
    TaskStatus_t* task_status_array;

    cJSON* data = cJSON_CreateObject();
    cJSON* task_list = cJSON_CreateArray();
    cJSON* task = NULL;
    if (data == NULL || task_list == NULL) {
        goto get_state_info_end;
    }

    task_status_array = (TaskStatus_t*)malloc(sizeof(TaskStatus_t) * task_count);

    if (task_status_array != NULL) {
        // 获取所有任务的状态信息
        task_count = uxTaskGetSystemState(task_status_array, task_count, NULL);
        cJSON_AddNumberToObject(data, "task_count", task_count);

        // 打印任务状态信息
        for (uint8_t i = 0; i < task_count; i++) {
            const char* pcTaskName = task_status_array[i].pcTaskName;
            uint8_t xTaskNumber = task_status_array[i].xTaskNumber;
            uint8_t eCurrentState = task_status_array[i].eCurrentState;
            uint16_t usStackHighWaterMark = task_status_array[i].usStackHighWaterMark;

            task = cJSON_CreateObject();
            if (task == NULL) {
                goto get_state_info_end;
            }
            cJSON_AddStringToObject(task, "pcTaskName", pcTaskName);
            cJSON_AddNumberToObject(task, "xTaskNumber", xTaskNumber);
            cJSON_AddNumberToObject(task, "eCurrentState", eCurrentState);
            cJSON_AddNumberToObject(task, "usStackHighWaterMark", usStackHighWaterMark);
            cJSON_AddItemToArray(task_list, task);

            // ESP_LOGI(TAG, "Task %s (ID: %u) is in state %u and water mark %uB.", pcTaskName, xTaskNumber, eCurrentState, usStackHighWaterMark);
        }

        free(task_status_array); // 释放分配的内存
    } else {
        ESP_LOGE(TAG, "Failed to allocate memory for task state information.");
    }

    cJSON_AddItemToObject(data, "task_list", task_list);

    // 获取堆内存的信息
    heap_caps_get_info(&heap_info, MALLOC_CAP_8BIT);

    total_free_bytes = heap_info.total_free_bytes;
    total_allocated_bytes = heap_info.total_allocated_bytes;
    largest_free_block = heap_info.largest_free_block;
    minimum_free_bytes = heap_info.minimum_free_bytes;

    heap_caps_get_info(&heap_info, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    internal_free_bytes = heap_info.total_free_bytes;

    cJSON_AddNumberToObject(data, "total_free_bytes", total_free_bytes);
    cJSON_AddNumberToObject(data, "internal_free_bytes", internal_free_bytes);
    cJSON_AddNumberToObject(data, "total_allocated_bytes", total_allocated_bytes);
    cJSON_AddNumberToObject(data, "largest_free_block", largest_free_block);
    cJSON_AddNumberToObject(data, "minimum_free_bytes", minimum_free_bytes);

    // ESP_LOGI(TAG, "Heap Info: Total free: %lu, Total allocated: %lu, Largest free block: %lu, Minimum free: %lu", total_free_bytes, total_allocated_bytes, largest_free_block, minimum_free_bytes);
    // ESP_LOGI(TAG, "data size: %d", data_index);

get_state_info_end:
    return data;
}

cJSON* get_user_config_json(void)
{
    cJSON* data = cJSON_CreateObject();
    if (data == NULL) {
        goto get_user_config_json_end;
    }

    cJSON_AddNumberToObject(data, "up_key_gpio_num", user_config.up_key_gpio_num);
    cJSON_AddNumberToObject(data, "down_key_gpio_num", user_config.down_key_gpio_num);
    cJSON_AddNumberToObject(data, "mpu_sda_gpio_num", user_config.mpu_sda_gpio_num);
    cJSON_AddNumberToObject(data, "mpu_scl_gpio_num", user_config.mpu_scl_gpio_num);
    cJSON_AddNumberToObject(data, "ws2812_gpio_num", user_config.ws2812_gpio_num);

    cJSON_AddStringToObject(data, "username", user_config.username);
    cJSON_AddStringToObject(data, "password", user_config.password);
    cJSON_AddStringToObject(data, "mdns_host_name", user_config.mdns_host_name);
    cJSON_AddStringToObject(data, "wifi_ap_ssid", user_config.wifi_ap_ssid);
    cJSON_AddStringToObject(data, "wifi_ap_pass", user_config.wifi_ap_pass);
    cJSON_AddStringToObject(data, "wifi_ssid", user_config.wifi_ssid);
    cJSON_AddStringToObject(data, "wifi_pass", user_config.wifi_pass);

    cJSON_AddNumberToObject(data, "wifi_scan_list_size", user_config.wifi_scan_list_size);
    cJSON_AddNumberToObject(data, "wifi_connect_max_retry", user_config.wifi_connect_max_retry);

    cJSON_AddNumberToObject(data, "ws_recv_buf_size", user_config.ws_recv_buf_size);
    cJSON_AddNumberToObject(data, "ws_send_buf_size", user_config.ws_send_buf_size);

    cJSON_AddNumberToObject(data, "msg_buf_recv_size", user_config.msg_buf_recv_size);
    cJSON_AddNumberToObject(data, "msg_buf_send_size", user_config.msg_buf_send_size);

    cJSON_AddNumberToObject(data, "button_period_ms", user_config.button_period_ms);

    cJSON_AddNumberToObject(data, "mpu_one_shot_max_sample_size", user_config.mpu_one_shot_max_sample_size);
    cJSON_AddNumberToObject(data, "mpu_buf_out_to_cnn_size", user_config.mpu_buf_out_to_cnn_size);

    cJSON_AddNumberToObject(data, "tflite_arena_size", user_config.tflite_arena_size);
    cJSON_AddNumberToObject(data, "tflite_model_size", user_config.tflite_model_size);

    cJSON_AddNumberToObject(data, "esplog_max_length", user_config.esplog_max_length);

    cJSON_AddNumberToObject(data, "periph_pwr_gpio_num", user_config.periph_pwr_gpio_num);
    cJSON_AddNumberToObject(data, "i2s_bck_gpio_num", user_config.i2s_bck_gpio_num);
    cJSON_AddNumberToObject(data, "i2s_ws_gpio_num", user_config.i2s_ws_gpio_num);
    cJSON_AddNumberToObject(data, "i2s_dout_gpio_num", user_config.i2s_dout_gpio_num);
    cJSON_AddNumberToObject(data, "ir_rx_gpio_num", user_config.ir_rx_gpio_num);
    cJSON_AddNumberToObject(data, "ir_tx_gpio_num", user_config.ir_tx_gpio_num);

get_user_config_json_end:
    return data;
}

void assign_user_config_from_json(const cJSON* data)
{
    if (data == NULL) {
        ESP_LOGE(TAG, "Invalid data for LEDC config");
        return;
    }

    const cJSON* up_key_gpio_num = cJSON_GetObjectItem(data, "up_key_gpio_num");
    user_config.up_key_gpio_num = (gpio_num_t)up_key_gpio_num->valuedouble;
    const cJSON* down_key_gpio_num = cJSON_GetObjectItem(data, "down_key_gpio_num");
    user_config.down_key_gpio_num = (gpio_num_t)down_key_gpio_num->valuedouble;
    const cJSON* mpu_sda_gpio_num = cJSON_GetObjectItem(data, "mpu_sda_gpio_num");
    user_config.mpu_sda_gpio_num = (gpio_num_t)mpu_sda_gpio_num->valuedouble;
    const cJSON* mpu_scl_gpio_num = cJSON_GetObjectItem(data, "mpu_scl_gpio_num");
    user_config.mpu_scl_gpio_num = (gpio_num_t)mpu_scl_gpio_num->valuedouble;
    const cJSON* ws2812_gpio_num = cJSON_GetObjectItem(data, "ws2812_gpio_num");
    user_config.ws2812_gpio_num = (gpio_num_t)ws2812_gpio_num->valuedouble;

    const cJSON* username = cJSON_GetObjectItem(data, "username");
    strcpy(user_config.username, username->valuestring);
    const cJSON* password = cJSON_GetObjectItem(data, "password");
    strcpy(user_config.password, password->valuestring);
    const cJSON* mdns_host_name = cJSON_GetObjectItem(data, "mdns_host_name");
    strcpy(user_config.mdns_host_name, mdns_host_name->valuestring);
    const cJSON* wifi_ap_ssid = cJSON_GetObjectItem(data, "wifi_ap_ssid");
    strcpy(user_config.wifi_ap_ssid, wifi_ap_ssid->valuestring);
    const cJSON* wifi_ap_pass = cJSON_GetObjectItem(data, "wifi_ap_pass");
    strcpy(user_config.wifi_ap_pass, wifi_ap_pass->valuestring);
    const cJSON* wifi_ssid = cJSON_GetObjectItem(data, "wifi_ssid");
    strcpy(user_config.wifi_ssid, wifi_ssid->valuestring);
    const cJSON* wifi_pass = cJSON_GetObjectItem(data, "wifi_pass");
    strcpy(user_config.wifi_pass, wifi_pass->valuestring);
    const cJSON* wifi_scan_list_size = cJSON_GetObjectItem(data, "wifi_scan_list_size");
    user_config.wifi_scan_list_size = wifi_scan_list_size->valuedouble;
    const cJSON* wifi_connect_max_retry = cJSON_GetObjectItem(data, "wifi_connect_max_retry");
    user_config.wifi_connect_max_retry = wifi_connect_max_retry->valuedouble;
    const cJSON* ws_recv_buf_size = cJSON_GetObjectItem(data, "ws_recv_buf_size");
    user_config.ws_recv_buf_size = ws_recv_buf_size->valuedouble;
    const cJSON* ws_send_buf_size = cJSON_GetObjectItem(data, "ws_send_buf_size");
    user_config.ws_send_buf_size = ws_send_buf_size->valuedouble;
    const cJSON* msg_buf_recv_size = cJSON_GetObjectItem(data, "msg_buf_recv_size");
    user_config.msg_buf_recv_size = msg_buf_recv_size->valuedouble;
    const cJSON* msg_buf_send_size = cJSON_GetObjectItem(data, "msg_buf_send_size");
    user_config.msg_buf_send_size = msg_buf_send_size->valuedouble;

    const cJSON* button_period_ms = cJSON_GetObjectItem(data, "button_period_ms");
    user_config.button_period_ms = button_period_ms->valuedouble;

    const cJSON* mpu_one_shot_max_sample_size = cJSON_GetObjectItem(data, "mpu_one_shot_max_sample_size");
    user_config.mpu_one_shot_max_sample_size = mpu_one_shot_max_sample_size->valuedouble;
    const cJSON* mpu_buf_out_to_cnn_size = cJSON_GetObjectItem(data, "mpu_buf_out_to_cnn_size");
    user_config.mpu_buf_out_to_cnn_size = mpu_buf_out_to_cnn_size->valuedouble;

    const cJSON* tflite_arena_size = cJSON_GetObjectItem(data, "tflite_arena_size");
    user_config.tflite_arena_size = tflite_arena_size->valuedouble;
    const cJSON* tflite_model_size = cJSON_GetObjectItem(data, "tflite_model_size");
    user_config.tflite_model_size = tflite_model_size->valuedouble;

    const cJSON* esplog_max_length = cJSON_GetObjectItem(data, "esplog_max_length");
    user_config.esplog_max_length = esplog_max_length->valuedouble;

    const cJSON* periph_pwr_gpio_num = cJSON_GetObjectItem(data, "periph_pwr_gpio_num");
    user_config.periph_pwr_gpio_num = (gpio_num_t)periph_pwr_gpio_num->valuedouble;

    const cJSON* i2s_bck_gpio_num = cJSON_GetObjectItem(data, "i2s_bck_gpio_num");
    user_config.i2s_bck_gpio_num = (gpio_num_t)i2s_bck_gpio_num->valuedouble;

    const cJSON* i2s_ws_gpio_num = cJSON_GetObjectItem(data, "i2s_ws_gpio_num");
    user_config.i2s_ws_gpio_num = (gpio_num_t)i2s_ws_gpio_num->valuedouble;

    const cJSON* i2s_dout_gpio_num = cJSON_GetObjectItem(data, "i2s_dout_gpio_num");
    user_config.i2s_dout_gpio_num = (gpio_num_t)i2s_dout_gpio_num->valuedouble;

    const cJSON* ir_rx_gpio_num = cJSON_GetObjectItem(data, "ir_rx_gpio_num");
    user_config.ir_rx_gpio_num = (gpio_num_t)ir_rx_gpio_num->valuedouble;

    const cJSON* ir_tx_gpio_num = cJSON_GetObjectItem(data, "ir_tx_gpio_num");
    user_config.ir_tx_gpio_num = (gpio_num_t)ir_tx_gpio_num->valuedouble;
}

cJSON* get_ledc_timer_config_json(int index)
{
    ledc_timer_config_t ledc_timer_config;
    get_ledc_timer_config_by_index(index, &ledc_timer_config);
    cJSON* ledc_timer_json = cJSON_CreateObject();
    cJSON_AddNumberToObject(ledc_timer_json, "index", index);
    cJSON_AddNumberToObject(ledc_timer_json, "speed_mode", ledc_timer_config.speed_mode);
    cJSON_AddNumberToObject(ledc_timer_json, "duty_resolution", ledc_timer_config.duty_resolution);
    cJSON_AddNumberToObject(ledc_timer_json, "freq_hz", ledc_timer_config.freq_hz);
    cJSON_AddNumberToObject(ledc_timer_json, "clk_cfg", ledc_timer_config.clk_cfg);
    return ledc_timer_json;
}

cJSON* get_ledc_channel_config_json(int index)
{
    ledc_channel_config_t ledc_channel_config;
    ledc_timer_config_t ledc_timer_config;

    get_ledc_channel_config_by_index(index, &ledc_channel_config);
    get_ledc_timer_config_by_index(ledc_channel_config.timer_sel, &ledc_timer_config);
    ledc_timer_bit_t duty_resolution = ledc_timer_config.duty_resolution;

    cJSON* ledc_channel_json = cJSON_CreateObject();
    cJSON_AddNumberToObject(ledc_channel_json, "index", index);
    cJSON_AddNumberToObject(ledc_channel_json, "gpio_num", ledc_channel_config.gpio_num);
    cJSON_AddNumberToObject(ledc_channel_json, "speed_mode", ledc_channel_config.speed_mode);
    cJSON_AddNumberToObject(ledc_channel_json, "timer_sel", ledc_channel_config.timer_sel);
    cJSON_AddNumberToObject(ledc_channel_json, "duty", ledc_channel_config.duty);
    cJSON_AddNumberToObject(ledc_channel_json, "hpoint", ledc_channel_config.hpoint);
    cJSON_AddNumberToObject(ledc_channel_json, "duty_resolution", duty_resolution);
    return ledc_channel_json;
}

cJSON* get_imu_data_json()
{
    double roll, pitch, ax, ay, az, gx, gy, gz;
    get_angle_and_m6(&roll, &pitch, &ax, &ay, &az, &gx, &gy, &gz);
    cJSON* imu_data = cJSON_CreateObject();
    cJSON_AddNumberToObject(imu_data, "roll", roll);
    cJSON_AddNumberToObject(imu_data, "pitch", pitch);
    cJSON_AddNumberToObject(imu_data, "ax", ax);
    cJSON_AddNumberToObject(imu_data, "ay", ay);
    cJSON_AddNumberToObject(imu_data, "az", az);
    cJSON_AddNumberToObject(imu_data, "gx", gx);
    cJSON_AddNumberToObject(imu_data, "gy", gy);
    cJSON_AddNumberToObject(imu_data, "gz", gz);
    return imu_data;
}

cJSON* get_dac_cosine_config_json(int index)
{
    dac_cosine_config_t dac_cosine_config;
    get_cosine_config_by_index(index, &dac_cosine_config);
    cJSON* dac_cosine_config_json = cJSON_CreateObject();
    cJSON_AddNumberToObject(dac_cosine_config_json, "index", index);
    cJSON_AddNumberToObject(dac_cosine_config_json, "freq_hz", dac_cosine_config.freq_hz);
    cJSON_AddNumberToObject(dac_cosine_config_json, "atten", dac_cosine_config.atten);
    cJSON_AddNumberToObject(dac_cosine_config_json, "phase", dac_cosine_config.phase);
    cJSON_AddNumberToObject(dac_cosine_config_json, "offset", dac_cosine_config.offset);
    return dac_cosine_config_json;
}

extern dac_cont_wav_t dac_cont_wav;
void load_dac_cont_data_from_json(const cJSON* data)
{
    cJSON* wav_data = cJSON_GetObjectItem(data, "wav_data");
    dac_cont_wav.length = cJSON_GetArraySize(wav_data);

    if (dac_cont_wav.data_wav != NULL) {
        free(dac_cont_wav.data_wav);
    }

    dac_cont_wav.data_wav = (uint8_t*)heap_caps_malloc(dac_cont_wav.length, MALLOC_CAP_INTERNAL);
    if (dac_cont_wav.data_wav == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory %dB for dac_cont_wav.data_wav", dac_cont_wav.length);
        return;
    }
    for (size_t i = 0; i < dac_cont_wav.length; i++) {
        cJSON* item = cJSON_GetArrayItem(wav_data, i);
        dac_cont_wav.data_wav[i] = item->valuedouble;
    }

    ESP_LOGI(TAG, "Successfully loaded dac_cont_wavs");
}

void* malloc_fn(size_t size)
{
    return heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

void cjson_hook_init(user_def_err_t* user_def_err)
{
    cJSON_Hooks memoryHook;

    memoryHook.malloc_fn = malloc_fn;
    memoryHook.free_fn = heap_caps_free;
    cJSON_InitHooks(&memoryHook);
    if (user_def_err) {
        user_def_err->esp_err = ESP_OK;
    }
}
