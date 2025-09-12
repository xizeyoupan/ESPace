#ifndef __WS2812_SERVICE_H__
#define __WS2812_SERVICE_H__

#include "stdint.h"

#define WS2812_STATIC_BIT BIT0
#define WS2812_BLINK_BIT BIT1
#define WS2812_BREATHE_BIT BIT2

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    COLOR_RED = 0xff0000,
    COLOR_GREEN = 0x00ff00,
    COLOR_BLUE = 0x0000ff,
    COLOR_YELLOW = 0xFFFF00,
    COLOR_MAGENTA = 0xFF00FF,
    COLOR_CYAN = 0x00FFFF,
    COLOR_WHITE = 0xFFFFFF,
    COLOR_NONE = 0x000000,
} color_enum;

typedef enum {
    COLOR_BOOTED = COLOR_YELLOW,
    COLOR_WIFI_CONNECTING = COLOR_MAGENTA,
    COLOR_WIFI_CONNECTED = COLOR_GREEN,
    COLOR_WIFI_ERROR = COLOR_RED,
    COLOR_MPU_SAMPLING = COLOR_BLUE,
    COLOR_MPU_PREDICTING = COLOR_GREEN,
    COLOR_MPU_SAMPLING_STOP = COLOR_GREEN,
} color_predefined_enum;

void ws2812_init();
void ws2812_send_pixel(uint32_t color);
void ws2812_blink(uint32_t color);
void ws2812_set_static_color(uint32_t color);
void ws2812_breathe(uint32_t color);

#ifdef __cplusplus
}
#endif

#endif