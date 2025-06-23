#include "nvs_util.h"

static const char* TAG = "NVS_UTIL";

esp_err_t save_to_namespace(char* user_namespace, const char* key, void* value, size_t size)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(user_namespace, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        goto save_to_namespace_end;
    }
    err = nvs_set_blob(nvs_handle, key, value, size);
    if (err != ESP_OK) {
        goto save_to_namespace_end;
    }
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        goto save_to_namespace_end;
    }
save_to_namespace_end:
    nvs_close(nvs_handle);
    return err;
}

esp_err_t load_from_namespace(char* user_namespace, const char* key, void* out_data, size_t size)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(user_namespace, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        goto load_from_namespace_end;
    }
    err = nvs_get_blob(nvs_handle, key, out_data, &size);
    if (err != ESP_OK) {
        goto load_from_namespace_end;
    }

load_from_namespace_end:
    nvs_close(nvs_handle);
    return err;
}

extern user_config_t user_config;

void reset_user_config()
{
    ESP_LOGI(TAG, "Reset user config");

    memset(&user_config, 0, sizeof(user_config_t));

    user_config.up_key_gpio_num = 18;
    user_config.down_key_gpio_num = 0;

    user_config.mpu_sda_gpio_num = 10;
    user_config.mpu_scl_gpio_num = 5;

    user_config.ws2812_gpio_num = 22;

    user_config.wifi_scan_list_size = 20;
    user_config.wifi_connect_max_retry = 10;
    strcpy(user_config.username, "murasame");
    strcpy(user_config.password, "0d00");
    strcpy(user_config.wifi_ap_ssid, "ESPace-AP");
    strcpy(user_config.wifi_ap_pass, "07210721");
    strcpy(user_config.mdns_host_name, "espace");

    user_config.ws_recv_buf_size = 1024;
    user_config.msg_buf_recv_size = 2 * 1024;

    user_config.ws_send_buf_size = 10 * 1024;
    user_config.msg_buf_send_size = 30 * 1024;

    user_config.button_period_ms = 3000;

    user_config.mpu_command_buf_size = 2 * 1024;
    user_config.mpu_buf_out_size = 20 * 1024;
    user_config.mpu_buf_out_to_cnn_size = 20 * 1024;
}

void load_user_config()
{
    if (load_from_namespace(USER_CONFIG_NVS_NAMESPACE, USER_CONFIG_NVS_KEY, &user_config, sizeof(user_config)) == ESP_OK) {
        ESP_LOGI(TAG, "Loaded user config from NVS");
    } else {
        ESP_LOGW(TAG, "Failed to load user config from NVS, using default values");
        reset_user_config();
    }

    cJSON* config = get_user_config_json();
    if (config == NULL) {
        ESP_LOGE(TAG, "Failed to get user config JSON");
        return;
    }

    char* config_str = cJSON_Print(config);
    if (config_str == NULL) {
        ESP_LOGE(TAG, "Failed to print user config JSON");
        cJSON_Delete(config);
        return;
    }
    ESP_LOGI(TAG, "User config JSON: %s", config_str);
    cJSON_Delete(config);
    free(config_str);
}

void save_user_config()
{
    if (save_to_namespace(USER_CONFIG_NVS_NAMESPACE, USER_CONFIG_NVS_KEY, &user_config, sizeof(user_config)) == ESP_OK) {
        ESP_LOGI(TAG, "Saved user config to NVS");
    } else {
        ESP_LOGE(TAG, "Failed to save user config to NVS");
    }
}
