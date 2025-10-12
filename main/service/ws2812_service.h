#ifndef __WS2812_SERVICE_H__
#define __WS2812_SERVICE_H__

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "stdint.h"

#define WS2812_STATIC_BIT BIT0
#define WS2812_BLINK_BIT BIT1
#define WS2812_BREATHE_BIT BIT2

// LED状态栈深度
#define LED_STATE_STACK_MAX 8

// LED模式枚举
typedef enum {
    LED_MODE_STATIC = 0,
    LED_MODE_BLINK = 1,
    LED_MODE_BREATHE = 2,
    LED_MODE_GRADIENT = 3,
} led_mode_t;

// LED状态结构
typedef struct {
    led_mode_t mode;          // LED模式
    uint32_t color1;          // 主颜色
    uint32_t color2;          // 副颜色（用于渐变等）
    uint32_t freq;            // 频率/速度参数
    uint32_t duration_ms;     // 总持续时间
    uint32_t remaining_ms;    // 剩余时间
    uint32_t start_time;      // 开始时间戳
    uint32_t elapsed_ms;      // 已执行时间
} led_state_t;

// LED状态栈
typedef struct {
    led_state_t states[LED_STATE_STACK_MAX];
    int8_t top;             // 栈顶指针，-1表示空栈
    SemaphoreHandle_t mutex; // 线程安全
} led_state_stack_t;

// 全局LED状态栈声明
extern led_state_stack_t g_led_stack;

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    COLOR_RED = 0xff0000,
    COLOR_ORANGE = 0xff7f00,
    COLOR_YELLOW = 0xFFFF00,
    COLOR_GREEN = 0x00ff00,
    COLOR_CYAN = 0x00ffff,
    COLOR_BLUE = 0x0000ff,
    COLOR_PURPLE = 0x7f00ff,
    COLOR_MAGENTA = 0xFF00FF,
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
} color_predefined_enum;

// 原有基础函数
void ws2812_init();
void ws2812_send_pixel(uint32_t color);

// LED状态栈管理API
typedef enum {
    LED_OK = 0,
    LED_STACK_FULL = -1,
    LED_STACK_EMPTY = -2,
    LED_INVALID_PARAM = -3,
} led_status_t;

// 栈操作函数
led_status_t led_stack_init();
led_status_t led_push_state(led_mode_t mode, uint32_t color1, uint32_t color2, uint32_t freq, uint32_t duration_ms);
led_status_t led_pop_state();
led_state_t* led_get_current_state();
led_status_t led_clear_stack();

// 兼容性函数（内部转换为栈操作）
void ws2812_blink(uint32_t color);
void ws2812_set_static_color(uint32_t color);
void ws2812_breathe(uint32_t color);

// 新增功能函数
void ws2812_gradient(uint32_t color1, uint32_t color2, uint32_t freq);

// 任务通知函数
void ws2812_notify_state_change();

#ifdef __cplusplus
}
#endif

#endif