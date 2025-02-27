#ifndef __WS2812_H__
#define __WS2812_H__

#include "user_config.h"

typedef enum {
    WS2812_BLINK_ERROR,
} blink_type;

#ifdef __cplusplus
extern "C" {
#endif

void WS2812_ControllerTask(void* pvParameters);
void ws2812_blink(blink_type blink);

#ifdef __cplusplus
}
#endif

#endif