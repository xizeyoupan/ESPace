#ifndef __WIFI_SERVICE_H__
#define __WIFI_SERVICE_H__

#include "cJSON.h"
#include "esp_http_server.h"
#include "nvs_util.h"
#include "user_config.h"
#include "wand_server_task.h"

#ifdef __cplusplus
extern "C" {
#endif

char* get_wifi_list(void);

#ifdef __cplusplus
}
#endif

#endif
