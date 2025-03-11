#ifndef __WS2812_SERVICE_H__
#define __WS2812_SERVICE_H__

#include "user_config.h"

#ifdef __cplusplus
extern "C" {
#endif

void ws2812_blink(uint32_t color);
void set_bg_color(uint32_t color);
void ws2812_init();

#ifdef __cplusplus
}
#endif

#endif