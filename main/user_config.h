#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#include "math.h"
#include "string.h"

#include "cJSON.h"
#include "mdns.h"
#include "multi_button.h"

#include "esp_clk_tree.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_heap_caps.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_wifi.h"

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/rmt_tx.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/message_buffer.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "hal/efuse_hal.h"
#include "hal/efuse_ll.h"

#include "peripherals/mpu_device.h"
#include "peripherals/ws2812.h"

#include "service/device_service.h"
#include "service/wifi_service.h"

#include "task/button.h"
#include "task/http_controller.h"
#include "task/play_mp3.h"
#include "task/status_task.h"
#include "task/wand_server_task.h"

#include "nvs_flash.h"
#include "nvs_util.h"
#include "tftest.h"

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

#define SW_VERSION "v0.0.1"
#define USER_CONFIG_NVS_NAMESPACE "user_config"
#define USER_CONFIG_NVS_KEY "config_data"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    COMMAND_MODEL = 0,
    COMMAND_MODEL = 1,
} model_type;

typedef struct
{
    model_type type;
    uint16_t sample_tick;
    uint16_t sample_size;
    uint8_t* data_p;
} model_t;

#pragma pack(1)
typedef struct
{
    gpio_num_t ws2812_gpio_num;
    gpio_num_t mpu_sda_gpio_num;
    gpio_num_t mpu_scl_gpio_num;
    uint8_t enable_imu_det;
    uint8_t enable_ws_log;
} user_config_t;

#ifdef __cplusplus
}
#endif

#endif