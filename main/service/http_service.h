#ifndef __HTTP_SERVICE_H__
#define __HTTP_SERVICE_H__

#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

httpd_handle_t start_webserver();

#ifdef __cplusplus
}
#endif

#endif