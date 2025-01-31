#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#include "driver/gpio.h"

typedef enum
{
    COLOR_RED = 0xff0000,
    COLOR_GREEN = 0x00ff00,
    COLOR_BLUE = 0x0000ff,
    COLOR_YELLOW = 0xFFFF00,
    COLOR_MAGENTA = 0xFF00FF,
    COLOR_CYAN = 0x00FFFF,
    COLOR_WHITE = 0xFFFFFF,
    COLOR_NONE = 0x000000,
} color_enum;

#define SW_VERSION "v0.0.1"
#define USER_CONFIG_NVS_NAMESPACE "user_config"
#define USER_CONFIG_WS2812_IO_NUM_KEY "ws2812_io_num"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        gpio_num_t ws2812_gpio_num;
    } user_config_t;

#ifdef __cplusplus
}
#endif

#endif