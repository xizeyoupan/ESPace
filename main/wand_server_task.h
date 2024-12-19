#ifndef __WAND_SERVER_TASK_H__
#define __WAND_SERVER_TASK_H__

#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "mdns.h"

#include <string.h>
#include "nvs_util.h"
#include "http_util.h"

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