#ifndef __DEVICE_SERVICE_H__
#define __DEVICE_SERVICE_H__

#include "cJSON.h"
#include "esp_http_server.h"
#include "nvs_util.h"
#include "user_config.h"
#include "wand_server_task.h"

#ifdef __cplusplus
extern "C" {
#endif

char* get_device_info(void);

#ifdef __cplusplus
}
#endif

#endif