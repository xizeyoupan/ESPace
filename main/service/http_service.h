#ifndef __HTTP_SERVICE_H__
#define __HTTP_SERVICE_H__

#include "user_config.h"

#define WIFI_SSID_MAX_LEN            32
#define WIFI_PASS_MAX_LEN            64

#define SEND_USER_CONFIG_DATA_PREFIX 0
#define SEND_WS_IMU_DATA_PREFIX      1
#define SEND_STAT_DATA_PREFIX        2
#define SEND_LOG_DATA_PREFIX         3
#define SEND_IMU_DATASET_DATA_PREFIX 4

#define FETCHED_GET_CONFIG           0
#define FETCHED_SET_CONFIG           1
#define FETCHED_RESET_CONFIG         2
#define FETCHED_RESET_IMU            3
#define FETCHED_READY_TO_SCAN        4

#ifdef __cplusplus
extern "C" {
#endif

httpd_handle_t start_webserver();

#ifdef __cplusplus
}
#endif

#endif