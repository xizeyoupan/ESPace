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