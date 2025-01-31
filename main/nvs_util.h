#ifndef __NVS_UTIL_H__
#define __NVS_UTIL_H__

#include "user_config.h"

#ifdef __cplusplus
extern "C"
{
#endif

    esp_err_t save_wifi_config(char *wifi_ssid, char *wifi_pass);
    esp_err_t load_wifi_config(char *wifi_ssid, char *wifi_pass);
    esp_err_t save_to_namespace(char *user_namespace, char *key, char *value);
    esp_err_t load_from_namespace(char *user_namespace, char *key, char *value);
    void load_user_config();

#ifdef __cplusplus
}
#endif

#endif