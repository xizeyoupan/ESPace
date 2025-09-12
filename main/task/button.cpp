#include "button.h"
#include "service/button_service.h"

#include "espace_define.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"

#include "stdint.h"
#include "string.h"

#include "multi_button.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "driver/gpio.h"

struct Button button0;
struct Button button1;

static const char* TAG = "BTN TASK";
EventGroupHandle_t button_event_group;
extern user_config_t user_config;

static void periodic_timer_callback(void* arg)
{
    button_ticks();
}

void into_sleep(void* btn)
{
    gpio_set_level(gpio_num_t(user_config.periph_pwr_gpio_num), 1);
    ESP_LOGW(TAG, "Into sleep");
    vTaskDelay(pdMS_TO_TICKS(100));
    esp_deep_sleep_start();
}

void scan_button_task(void* pvParameters)
{
    BaseType_t core_id = xPortGetCoreID(); // 返回当前任务所在的核心 ID
    ESP_LOGI(TAG, "Task is running on core %d.", core_id);

    button_event_group = xEventGroupCreate();

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE; // 禁用中断
    io_conf.pin_bit_mask = (1ULL << user_config.up_key_gpio_num) | (1ULL << user_config.down_key_gpio_num); // 设置 GPIO
    io_conf.mode = GPIO_MODE_INPUT; // 设置为输入模式
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE; // 启用上拉电阻
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; // 禁用下拉电阻
    gpio_config(&io_conf);

    button_init(&button0, read_button_GPIO, 0, 0);
    button_init(&button1, read_button_GPIO, 0, 1);

    button_attach(&button0, SINGLE_CLICK, btn_single_click_handler);
    button_attach(&button1, SINGLE_CLICK, btn_single_click_handler);

    button_attach(&button0, PRESS_UP, btn_press_down_up_handler);
    button_attach(&button0, PRESS_DOWN, btn_press_down_up_handler);

    button_attach(&button1, PRESS_UP, btn_press_down_up_handler);
    button_attach(&button1, PRESS_DOWN, btn_press_down_up_handler);
    button_attach(&button1, DOUBLE_CLICK, into_sleep);

    button_start(&button0);
    button_start(&button1);

    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &periodic_timer_callback,
        .name = "btn periodic"
    };

    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, TICKS_INTERVAL * 1000));
    ESP_LOGI(TAG, "Started button scan.");

    vTaskDelete(NULL);
}
