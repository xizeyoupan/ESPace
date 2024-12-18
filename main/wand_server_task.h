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
#include "esp_http_server.h"
#include "nvs_flash.h"
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

    static void start_ap_mode();
    void wand_server_task(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif