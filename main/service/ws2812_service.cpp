#include "ws2812_service.h"

#include "espace_define.h"

#include "esp_err.h"
#include "esp_log.h"

#include "stdint.h"
#include "string.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "driver/rmt_tx.h"

// WS2812 时序定义（单位：纳秒）
#define T0H 300 // 逻辑 0 高电平时间
#define T0L 900 // 逻辑 0 低电平时间
#define T1H 900 // 逻辑 1 高电平时间
#define T1L 300 // 逻辑 1 低电平时间
#define RESET_TIME 50000 // 复位信号

static const char* TAG = "WS2812_SERVICE";
extern EventGroupHandle_t s_ws2812_event_group;

extern user_config_t user_config;

// LED状态栈全局变量
led_state_stack_t g_led_stack;
extern TaskHandle_t ws2812_task_handle;

rmt_encoder_handle_t ws2812_encoder;
rmt_encoder_handle_t ws2812_reset_encoder;
rmt_bytes_encoder_config_t bytes_encoder_config = {
    .bit0 = rmt_symbol_word_t { .duration0 = T0H / 100, .level0 = 1, .duration1 = T0L / 100, .level1 = 0 }, // 逻辑 0
    .bit1 = rmt_symbol_word_t { .duration0 = T1H / 100, .level0 = 1, .duration1 = T1L / 100, .level1 = 0 }, // 逻辑 1
    .flags = { 1 } // 高位先发
};

rmt_copy_encoder_config_t bytes_reset_encoder_config = {};

// 复位信号
rmt_symbol_word_t reset_symbol = {
    .duration0 = RESET_TIME / 100,
    .level0 = 0,
    .duration1 = 0,
    .level1 = 0
};
rmt_transmit_config_t tx_config = {
    .loop_count = 0 // 不循环
};

rmt_channel_handle_t led_chan = NULL;

void ws2812_send_pixel(uint32_t color)
{
    uint8_t pixel_data[3]; // WS2812 颜色顺序：GRB
    pixel_data[0] = (color >> 8) & 0xff;
    pixel_data[1] = (color >> 16) & 0xff;
    pixel_data[2] = (color >> 0) & 0xff;

    ESP_ERROR_CHECK(rmt_transmit(led_chan, ws2812_encoder, pixel_data, sizeof(pixel_data), &tx_config));
    ESP_ERROR_CHECK(rmt_transmit(led_chan, ws2812_reset_encoder, &reset_symbol, sizeof(reset_symbol), &tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));
}

void ws2812_init()
{

    // 初始化LED状态栈
    led_stack_init();

    rmt_tx_channel_config_t tx_chan_config = {};
    tx_chan_config.clk_src = RMT_CLK_SRC_DEFAULT;
    tx_chan_config.gpio_num = gpio_num_t(user_config.ws2812_gpio_num);
    tx_chan_config.mem_block_symbols = 64; // increase the block size can make the LED less flickering
    tx_chan_config.resolution_hz = 10 * 1000 * 1000; // 10 MHz（100 ns 分辨率）,
    tx_chan_config.trans_queue_depth = 4; // set the number of transactions that can be pending in the background

    ESP_LOGI(TAG, "Create RMT TX channel");

    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &led_chan));

    ESP_ERROR_CHECK(rmt_new_bytes_encoder(&bytes_encoder_config, &ws2812_encoder));
    ESP_ERROR_CHECK(rmt_new_copy_encoder(&bytes_reset_encoder_config, &ws2812_reset_encoder));

    ESP_LOGI(TAG, "Enable RMT TX channel");
    ESP_ERROR_CHECK(rmt_enable(led_chan));
}

void ws2812_set_static_color(uint32_t color)
{
    led_push_state(LED_MODE_STATIC, color, 0, 0, portMAX_DELAY); // 静态颜色，无限时长
}

void ws2812_blink(uint32_t color)
{
    led_push_state(LED_MODE_BLINK, color, 0, 400, portMAX_DELAY); // 400ms频率，无限时长
}

void ws2812_breathe(uint32_t color)
{
    led_push_state(LED_MODE_BREATHE, color, 0, 10, portMAX_DELAY); // 10ms频率，无限时长
}

void ws2812_gradient(uint32_t color1, uint32_t color2, uint32_t freq)
{
    led_push_state(LED_MODE_GRADIENT, color1, color2, freq, portMAX_DELAY); // 渐变模式，无限时长
}

// ================== LED状态栈管理API ==================

led_status_t led_stack_init()
{
    g_led_stack.mutex = xSemaphoreCreateMutex();
    configASSERT(g_led_stack.mutex);
    g_led_stack.top = -1; // 空栈

    ESP_LOGI(TAG, "LED state stack initialized");
    return LED_OK;
}

led_status_t led_push_state(led_mode_t mode, uint32_t color1, uint32_t color2, uint32_t freq, uint32_t duration_ms)
{
    if (xSemaphoreTake(g_led_stack.mutex, portMAX_DELAY) != pdTRUE) {
        return LED_INVALID_PARAM;
    }

    // 检查栈是否已满
    if (g_led_stack.top >= LED_STATE_STACK_MAX - 1) {
        xSemaphoreGive(g_led_stack.mutex);
        ESP_LOGE(TAG, "LED state stack full");
        return LED_STACK_FULL;
    }

    // 压栈新状态
    g_led_stack.top++;
    led_state_t* state = &g_led_stack.states[g_led_stack.top];

    // 初始化状态
    state->mode = mode;
    state->color1 = color1;
    state->color2 = color2;
    state->freq = freq;
    state->duration_ms = duration_ms;
    state->remaining_ms = duration_ms;
    state->start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    state->elapsed_ms = 0;

    xSemaphoreGive(g_led_stack.mutex);

    // 通知LED任务有新状态
    ws2812_notify_state_change();

    return LED_OK;
}

led_status_t led_pop_state()
{
    if (xSemaphoreTake(g_led_stack.mutex, portMAX_DELAY) != pdTRUE) {
        return LED_INVALID_PARAM;
    }

    // 检查栈是否为空
    if (g_led_stack.top < 0) {
        xSemaphoreGive(g_led_stack.mutex);
        return LED_STACK_EMPTY;
    }

    g_led_stack.top--;
    xSemaphoreGive(g_led_stack.mutex);

    ESP_LOGI(TAG, "LED state popped, new top: %d", g_led_stack.top);
    return LED_OK;
}

led_state_t* led_get_current_state()
{
    if (xSemaphoreTake(g_led_stack.mutex, portMAX_DELAY) != pdTRUE) {
        return NULL;
    }

    if (g_led_stack.top < 0) {
        xSemaphoreGive(g_led_stack.mutex);
        return NULL;
    }

    led_state_t* state = &g_led_stack.states[g_led_stack.top];
    xSemaphoreGive(g_led_stack.mutex);
    return state;
}

led_status_t led_clear_stack()
{
    if (xSemaphoreTake(g_led_stack.mutex, portMAX_DELAY) != pdTRUE) {
        return LED_INVALID_PARAM;
    }

    g_led_stack.top = -1;
    xSemaphoreGive(g_led_stack.mutex);

    ESP_LOGI(TAG, "LED state stack cleared");
    return LED_OK;
}

void ws2812_notify_state_change()
{
    // 通知LED任务状态改变
    if (ws2812_task_handle != NULL) {
        xTaskNotifyGive(ws2812_task_handle);
    }
}
