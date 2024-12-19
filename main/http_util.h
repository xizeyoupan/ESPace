#ifndef __HTTP_UTIL_H__
#define __HTTP_UTIL_H__

#include "cJSON.h"
#include "wand_server_task.h"
#include "esp_http_server.h"
#include "nvs_util.h"
#include "user_config.h"

#ifdef __cplusplus
extern "C"
{
#endif

    httpd_handle_t start_webserver();

#ifdef __cplusplus
}
#endif

#endif