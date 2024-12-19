#ifndef __NVS_UTIL_H__
#define __NVS_UTIL_H__

#include "nvs_flash.h"
#include "wand_server_task.h"

#ifdef __cplusplus
extern "C"
{
#endif

    esp_err_t save_wifi_config(char *wifi_ssid, char *wifi_pass);
    esp_err_t load_wifi_config(char *wifi_ssid, char *wifi_pass);

#ifdef __cplusplus
}
#endif

#endif