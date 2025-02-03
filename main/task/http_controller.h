#ifndef __HTTP_CONTROLLER_H__
#define __HTTP_CONTROLLER_H__

#include "user_config.h"

#define USER_CONFIG_DATA_PREFIX 0
#define WS_IMU_DATA_PREFIX 1
#define STAT_DATA_PREFIX 2

#ifdef __cplusplus
extern "C"
{
#endif

    httpd_handle_t start_webserver();
    void websocket_send_task(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif