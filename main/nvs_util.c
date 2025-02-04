#include "nvs_util.h"

static const char *TAG = "NVS_UTIL";

// 保存 Wi-Fi 配置到 NVS
esp_err_t save_wifi_config(char *wifi_ssid, char *wifi_pass)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("wifi_config", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        goto save_wifi_config_end;
    }
    err = nvs_set_str(nvs_handle, "ssid", wifi_ssid);
    if (err != ESP_OK)
    {
        goto save_wifi_config_end;
    }
    err = nvs_set_str(nvs_handle, "password", wifi_pass);
    if (err != ESP_OK)
    {
        goto save_wifi_config_end;
    }
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK)
    {
        goto save_wifi_config_end;
    }
save_wifi_config_end:
    nvs_close(nvs_handle);
    return err;
}

// 从 NVS 加载 Wi-Fi 配置
esp_err_t load_wifi_config(char *wifi_ssid, char *wifi_pass)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("wifi_config", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK)
    {
        goto load_wifi_config_end;
    }
    size_t ssid_len = WIFI_SSID_MAX_LEN;
    size_t pass_len = WIFI_PASS_MAX_LEN;
    err = nvs_get_str(nvs_handle, "ssid", wifi_ssid, &ssid_len);
    if (err != ESP_OK)
    {
        goto load_wifi_config_end;
    }
    err = nvs_get_str(nvs_handle, "password", wifi_pass, &pass_len);
    if (err != ESP_OK)
    {
        goto load_wifi_config_end;
    }

load_wifi_config_end:
    nvs_close(nvs_handle);
    return err;
}

esp_err_t save_to_namespace(char *user_namespace, const char *key, void *value, size_t size)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(user_namespace, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        goto save_to_namespace_end;
    }
    err = nvs_set_blob(nvs_handle, key, value, size);
    if (err != ESP_OK)
    {
        goto save_to_namespace_end;
    }
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK)
    {
        goto save_to_namespace_end;
    }
save_to_namespace_end:
    nvs_close(nvs_handle);
    return err;
}

esp_err_t load_from_namespace(char *user_namespace, const char *key, void *out_data, size_t size)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(user_namespace, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK)
    {
        goto load_from_namespace_end;
    }
    err = nvs_get_blob(nvs_handle, key, out_data, &size);
    if (err != ESP_OK)
    {
        goto load_from_namespace_end;
    }

load_from_namespace_end:
    nvs_close(nvs_handle);
    return err;
}

extern user_config_t user_config;

void load_user_config()
{

    // if (load_from_namespace(USER_CONFIG_NVS_NAMESPACE, USER_CONFIG_NVS_KEY, &user_config, sizeof(user_config)) == ESP_OK)
    // {
    //     ESP_LOGI(TAG, "Loaded user config from NVS");
    // }
    // else
    // {
    ESP_LOGW(TAG, "Failed to load user config from NVS, using default values");
    user_config.ws2812_gpio_num = GPIO_NUM_22;
    user_config.mpu_sda_gpio_num = GPIO_NUM_10;
    user_config.mpu_scl_gpio_num = GPIO_NUM_5;
    user_config.enable_imu = 1;
    user_config.enable_imu_det = 1;
    // }

    ESP_LOGI(TAG, "WS2812 GPIO: %d", user_config.ws2812_gpio_num);
    ESP_LOGI(TAG, "MPU SDA GPIO: %d", user_config.mpu_sda_gpio_num);
    ESP_LOGI(TAG, "MPU SCL GPIO: %d", user_config.mpu_scl_gpio_num);
    ESP_LOGI(TAG, "Enable IMU: %d", user_config.enable_imu);
}

void save_user_config()
{
    if (save_to_namespace(USER_CONFIG_NVS_NAMESPACE, USER_CONFIG_NVS_KEY, &user_config, sizeof(user_config)) == ESP_OK)
    {
        ESP_LOGI(TAG, "Saved user config to NVS");
    }
    else
    {
        ESP_LOGE(TAG, "Failed to save user config to NVS");
    }
}
