#ifndef __WS2812_TASK_H__
#define __WS2812_TASK_H__

#include "user_config.h"

#define WS2812_STATIC_BIT BIT0
#define WS2812_BLINK_BIT BIT1
#define WS2812_BREATHE_BIT BIT2

#ifdef __cplusplus
extern "C" {
#endif

void ws2812_task(void* pvParameters);

#ifdef __cplusplus
}
#endif

#endif