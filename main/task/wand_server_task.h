#ifndef __WAND_SERVER_TASK_H__
#define __WAND_SERVER_TASK_H__

#include "user_config.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define WIFI_SSID_MAX_LEN 32
#define WIFI_PASS_MAX_LEN 64

    void start_ap_mode();
    void start_sta_mode();
    void wand_server_task(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif