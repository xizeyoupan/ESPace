#include "nvs_util.h"

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

esp_err_t save_to_namespace(char *user_namespace, char *key, char *value)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(user_namespace, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK)
    {
        goto save_to_namespace_end;
    }
    err = nvs_set_str(nvs_handle, key, value);
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

esp_err_t load_from_namespace(char *user_namespace, char *key, char *value)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(user_namespace, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK)
    {
        goto load_from_namespace_end;
    }
    size_t str_len = 128;
    err = nvs_get_str(nvs_handle, key, value, &str_len);
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
    char temp[128];
    if (load_from_namespace(USER_CONFIG_NVS_NAMESPACE, USER_CONFIG_WS2812_IO_NUM_KEY, temp) == ESP_OK)
    {
        user_config.ws2812_gpio_num = atoi(temp);
    }
    else
    {
        user_config.ws2812_gpio_num = GPIO_NUM_22;
    }

    if (load_from_namespace(USER_CONFIG_NVS_NAMESPACE, USER_CONFIG_MPU_SDA_IO_NUM_KEY, temp) == ESP_OK)
    {
        user_config.mpu_sda_gpio_num = atoi(temp);
    }
    else
    {
        user_config.mpu_sda_gpio_num = GPIO_NUM_10;
    }

    if (load_from_namespace(USER_CONFIG_NVS_NAMESPACE, USER_CONFIG_MPU_SCL_IO_NUM_KEY, temp) == ESP_OK)
    {
        user_config.mpu_scl_gpio_num = atoi(temp);
    }
    else
    {
        user_config.mpu_scl_gpio_num = GPIO_NUM_5;
    }
}
