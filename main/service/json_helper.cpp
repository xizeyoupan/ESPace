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

#include "user_util.h"

#include "cJSON.h"

static const char* TAG = "JSON_HELPER";
extern user_config_t user_config;

void* malloc_fn(size_t size)
{
    return heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

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
    cJSON* data = toCjsonObj(&user_config, &user_config_t_info);
    return data;
}

void assign_user_config_from_json(const cJSON* data)
{
    if (data == NULL) {
        ESP_LOGE(TAG, "Invalid data for LEDC config");
        return;
    }

    fromCjsonObj(&user_config, &user_config_t_info, data);
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

    dac_cont_wav.data_wav = (uint8_t*)malloc_fn(dac_cont_wav.length);
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
