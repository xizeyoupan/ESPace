#ifndef __HTTP_CONTROLLER_H__
#define __HTTP_CONTROLLER_H__

#include "cJSON.h"
#include "device_service.h"
#include "esp_http_server.h"
#include "nvs_util.h"
#include "user_config.h"
#include "wand_server_task.h"
#include "wifi_service.h"

#ifdef __cplusplus
extern "C" {
#endif

httpd_handle_t start_webserver();

#ifdef __cplusplus
}
#endif

#endif