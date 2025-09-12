#include "ws2812_task.h"

#include "espace_define.h"

#include "esp_err.h"
#include "esp_log.h"

#include "stdint.h"
#include "string.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "service/ws2812_service.h"

static const char* TAG = "WS2812_TASK";
extern user_config_t user_config;
extern uint32_t target_color;
extern SemaphoreHandle_t ws2812_mutex;

EventGroupHandle_t s_ws2812_event_group;
uint8_t blink_cnt = 0;
uint8_t breath_cnt = 0;
uint8_t breath_dir = 0;
const double BREATHE_STEP = 100;

void ws2812_task(void* pvParameters)
{
    s_ws2812_event_group = xEventGroupCreate();

    ws2812_init();
    ws2812_set_static_color(COLOR_BOOTED);

    while (1) {
        EventBits_t bits = xEventGroupWaitBits(s_ws2812_event_group, WS2812_STATIC_BIT | WS2812_BLINK_BIT | WS2812_BREATHE_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

        if (bits & WS2812_STATIC_BIT) {
            xEventGroupClearBits(s_ws2812_event_group, WS2812_STATIC_BIT);
        } else if (bits & WS2812_BLINK_BIT) {
            blink_cnt = (blink_cnt + 1) % 2;
            xSemaphoreTake(ws2812_mutex, portMAX_DELAY);
            if (blink_cnt) {
                ws2812_send_pixel(target_color);
            } else {
                ws2812_send_pixel(COLOR_NONE);
            }
            xSemaphoreGive(ws2812_mutex);
            vTaskDelay(pdMS_TO_TICKS(400));
        } else if (bits & WS2812_BREATHE_BIT) {
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

            uint32_t color_r = ((target_color >> 16) & 0xff) / BREATHE_STEP * breath_cnt;
            uint32_t color_g = ((target_color >> 8) & 0xff) / BREATHE_STEP * breath_cnt;
            uint32_t color_b = (target_color & 0xff) / BREATHE_STEP * breath_cnt;
            uint32_t color = (color_r << 16) | (color_g << 8) | color_b;

            xSemaphoreTake(ws2812_mutex, portMAX_DELAY);
            ws2812_send_pixel(color);
            xSemaphoreGive(ws2812_mutex);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    vTaskDelete(NULL);
}
