#include "ws2812_task.h"

#include "espace_define.h"

#include "esp_err.h"
#include "esp_log.h"

#include "stdint.h"
#include "string.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "service/ws2812_service.h"

static const char* TAG = "WS2812_TASK";
extern user_config_t user_config;
extern led_state_stack_t g_led_stack;

EventGroupHandle_t s_ws2812_event_group;
TaskHandle_t ws2812_task_handle = NULL;

// LED任务状态变量
uint8_t blink_cnt = 0;
uint8_t breath_cnt = 0;
uint8_t breath_dir = 0;
const double BREATHE_STEP = 100;

// 渐变状态变量
double gradient_step = 0.0;
uint8_t gradient_dir = 0; // 0: color1 -> color2, 1: color2 -> color1

// 时间管理相关
uint32_t last_update_time = 0;
uint32_t state_start_time = 0;

// 执行单个LED模式的函数
static void execute_led_mode(led_state_t* state)
{
    uint32_t color = 0;

    uint32_t color_r = 0;
    uint32_t color_g = 0;
    uint32_t color_b = 0;

    uint32_t r1 = 0;
    uint32_t g1 = 0;
    uint32_t b1 = 0;
    uint32_t r2 = 0;
    uint32_t g2 = 0;
    uint32_t b2 = 0;

    uint32_t r = 0;
    uint32_t g = 0;
    uint32_t b = 0;

    switch (state->mode) {
    case LED_MODE_STATIC:
        ws2812_send_pixel(state->color1);
        break;

    case LED_MODE_BLINK:
        blink_cnt = (blink_cnt + 1) % 2;
        if (blink_cnt) {
            ws2812_send_pixel(state->color1);
        } else {
            ws2812_send_pixel(COLOR_NONE);
        }
        break;

    case LED_MODE_BREATHE:
        if (breath_dir == 0) {
            breath_cnt++;
            if (breath_cnt >= BREATHE_STEP) {
                breath_dir = 1;
            }
        } else {
            breath_cnt--;
            if (breath_cnt == 0) {
                breath_dir = 0;
            }
        }

        color_r = ((state->color1 >> 16) & 0xff) / BREATHE_STEP * breath_cnt;
        color_g = ((state->color1 >> 8) & 0xff) / BREATHE_STEP * breath_cnt;
        color_b = (state->color1 & 0xff) / BREATHE_STEP * breath_cnt;
        color = (color_r << 16) | (color_g << 8) | color_b;
        ws2812_send_pixel(color);

        break;

    case LED_MODE_GRADIENT:
        if (gradient_dir == 0) {
            gradient_step += 1.0 / state->freq;
            if (gradient_step >= 1.0) {
                gradient_step = 1.0;
                gradient_dir = 1;
            }
        } else {
            gradient_step -= 1.0 / state->freq;
            if (gradient_step <= 0.0) {
                gradient_step = 0.0;
                gradient_dir = 0;
            }
        }

        r1 = (state->color1 >> 16) & 0xff;
        g1 = (state->color1 >> 8) & 0xff;
        b1 = state->color1 & 0xff;
        r2 = (state->color2 >> 16) & 0xff;
        g2 = (state->color2 >> 8) & 0xff;
        b2 = state->color2 & 0xff;

        r = (uint32_t)(r1 + (r2 - r1) * gradient_step);
        g = (uint32_t)(g1 + (g2 - g1) * gradient_step);
        b = (uint32_t)(b1 + (b2 - b1) * gradient_step);

        color = (r << 16) | (g << 8) | b;
        ws2812_send_pixel(color);

        break;

    default:
        ws2812_send_pixel(COLOR_NONE);
        break;
    }
}

void ws2812_task(void* pvParameters)
{
    ws2812_task_handle = xTaskGetCurrentTaskHandle();
    s_ws2812_event_group = xEventGroupCreate();

    ws2812_init();
    ws2812_set_static_color(COLOR_BOOTED);

    while (1) {
        // 获取当前栈顶状态
        led_state_t* current_state = led_get_current_state();

        if (current_state != NULL) {
            uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;

            // 检查状态是否超时
            if (current_state->duration_ms != portMAX_DELAY) {
                uint32_t elapsed = current_time - current_state->start_time;

                if (elapsed >= current_state->duration_ms) {
                    // 状态超时，出栈并处理下一个状态
                    ESP_LOGI(TAG, "LED state timeout, popping");
                    led_pop_state();
                    continue; // 重新获取栈顶状态
                }

                // 更新剩余时间
                current_state->elapsed_ms = elapsed;
                current_state->remaining_ms = current_state->duration_ms - elapsed;
            }

            // 执行当前状态的持续逻辑
            if (current_state->mode == LED_MODE_STATIC) {
                // 静态颜色：执行一次，然后等待通知
                ws2812_send_pixel(current_state->color1);

                // 等待状态变化通知
                ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            } else {
                // 动态模式：持续执行，但可以被通知打断
                execute_led_mode(current_state);

                // 根据模式设置适当的延迟
                uint32_t delay_ms = 0;
                switch (current_state->mode) {
                case LED_MODE_BLINK:
                    delay_ms = current_state->freq; // 闪烁频率
                    break;
                case LED_MODE_BREATHE:
                    delay_ms = current_state->freq; // 呼吸速度
                    break;
                case LED_MODE_GRADIENT:
                    delay_ms = 10; // 渐变固定10ms更新
                    break;
                default:
                    delay_ms = 100; // 默认延迟
                    break;
                }

                // 可中断的延迟
                if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(delay_ms)) > 0) {
                    // 有新状态，重新开始循环处理栈顶
                    ESP_LOGI(TAG, "New state detected, switching");
                    continue;
                }
            }
        } else {
            // 栈为空，关闭LED，等待新状态
            ws2812_send_pixel(COLOR_NONE);
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        }
    }

    vTaskDelete(NULL);
}
