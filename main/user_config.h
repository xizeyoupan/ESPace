#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#include "math.h"
#include "string.h"

#include "cJSON.h"
#include "mdns.h"

#include "esp_clk_tree.h"
#include "esp_event.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_wifi.h"

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/rmt_tx.h"

#include "freertos/event_groups.h"
#include "freertos/FreeRTOS.h"
#include "freertos/message_buffer.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "hal/efuse_ll.h"
#include "hal/efuse_hal.h"

#include "peripherals/ws2812.h"
#include "peripherals/mpu_device.h"

#include "service/device_service.h"
#include "service/wifi_service.h"

#include "nvs_flash.h"
#include "nvs_util.h"
#include "http_controller.h"
#include "wand_server_task.h"

#include "tftest.h"

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
#define USER_CONFIG_MPU_SDA_IO_NUM_KEY "mpu_sda_io_num"
#define USER_CONFIG_MPU_SCL_IO_NUM_KEY "mpu_scl_io_num"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        gpio_num_t ws2812_gpio_num;
        gpio_num_t mpu_sda_gpio_num;
        gpio_num_t mpu_scl_gpio_num;
    } user_config_t;

#ifdef __cplusplus
}
#endif

#endif