#include "device_service.h"

static const char* TAG = "DEVICE_SERVICE";
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
    cJSON* data = cJSON_CreateObject();
    cJSON* task_list = cJSON_CreateArray();
    cJSON* task = NULL;
    if (data == NULL || task_list == NULL) {
        goto get_state_info_end;
    }

    uint8_t task_count = uxTaskGetNumberOfTasks(); // 获取当前任务数量
    TaskStatus_t* task_status_array = malloc(sizeof(TaskStatus_t) * task_count);

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

    // 定义一个 heap_caps_info_t 结构体来存储内存信息
    multi_heap_info_t heap_info;
    // 获取堆内存的信息
    heap_caps_get_info(&heap_info, MALLOC_CAP_8BIT);

    uint32_t total_free_bytes = heap_info.total_free_bytes;
    uint32_t total_allocated_bytes = heap_info.total_allocated_bytes;
    uint32_t largest_free_block = heap_info.largest_free_block;
    uint32_t minimum_free_bytes = heap_info.minimum_free_bytes;

    cJSON_AddNumberToObject(data, "total_free_bytes", total_free_bytes);
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

    cJSON_AddNumberToObject(data, "mpu_command_buf_size", user_config.mpu_command_buf_size);
    cJSON_AddNumberToObject(data, "mpu_one_shot_max_sample_size", user_config.mpu_one_shot_max_sample_size);
    cJSON_AddNumberToObject(data, "mpu_buf_out_to_cnn_size", user_config.mpu_buf_out_to_cnn_size);

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
    user_config.up_key_gpio_num = up_key_gpio_num->valuedouble;
    const cJSON* down_key_gpio_num = cJSON_GetObjectItem(data, "down_key_gpio_num");
    user_config.down_key_gpio_num = down_key_gpio_num->valuedouble;
    const cJSON* mpu_sda_gpio_num = cJSON_GetObjectItem(data, "mpu_sda_gpio_num");
    user_config.mpu_sda_gpio_num = mpu_sda_gpio_num->valuedouble;
    const cJSON* mpu_scl_gpio_num = cJSON_GetObjectItem(data, "mpu_scl_gpio_num");
    user_config.mpu_scl_gpio_num = mpu_scl_gpio_num->valuedouble;
    const cJSON* ws2812_gpio_num = cJSON_GetObjectItem(data, "ws2812_gpio_num");
    user_config.ws2812_gpio_num = ws2812_gpio_num->valuedouble;

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

    const cJSON* mpu_command_buf_size = cJSON_GetObjectItem(data, "mpu_command_buf_size");
    user_config.mpu_command_buf_size = mpu_command_buf_size->valuedouble;
    const cJSON* mpu_one_shot_max_sample_size = cJSON_GetObjectItem(data, "mpu_one_shot_max_sample_size");
    user_config.mpu_one_shot_max_sample_size = mpu_one_shot_max_sample_size->valuedouble;
    const cJSON* mpu_buf_out_to_cnn_size = cJSON_GetObjectItem(data, "mpu_buf_out_to_cnn_size");
    user_config.mpu_buf_out_to_cnn_size = mpu_buf_out_to_cnn_size->valuedouble;
}
