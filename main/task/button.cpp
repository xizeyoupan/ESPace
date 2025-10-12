#include "button.h"
#include "user_util.h"

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

#include "task/mpu_task.h"

struct Button button0;
struct Button button1;

static const char* TAG = "BTN TASK";
extern user_config_t user_config;

extern mpu_state_t mpu_state;
extern EventGroupHandle_t x_mpu_event_group;

// 从 button_service 移植过来的 GPIO 读取函数
uint8_t read_button_GPIO(uint8_t button_id)
{
    switch (button_id) {
    case 0:
        return gpio_get_level(gpio_num_t(user_config.up_key_gpio_num));
        break;
    case 1:
        return gpio_get_level(gpio_num_t(user_config.down_key_gpio_num));
        break;
    default:
        return 0;
        break;
    }
}

// 定义按钮动作描述符
typedef struct {
    device_state_t state;
    uint8_t button_id;
    uint8_t button_event;
    void (*handler)(void);
    const char* description;
} button_action_descriptor_t;

static void handle_sampling_btn0_down(void)
{
    xEventGroupSetBits(x_mpu_event_group, MPU_SAMPLING_START_BIT);
}

static void handle_sampling_btn0_up(void)
{
    xEventGroupSetBits(x_mpu_event_group, MPU_SAMPLING_STOP_BIT);
}

// 动作查找表
static const button_action_descriptor_t action_descriptors[] = {

    // SAMPLING 状态
    { DEVICE_STATE_DATASET_SAMPLING, 0, PRESS_DOWN, handle_sampling_btn0_down, "开始采样" },
    { DEVICE_STATE_DATASET_SAMPLING, 0, PRESS_UP, handle_sampling_btn0_up, "停止采样" },

};

// 状态驱动的按钮动作处理函数
void handle_button_action(uint8_t button_id, uint8_t button_event)
{
    device_state_t device_state = get_device_state();
    ESP_LOGI(TAG, "Button action: btn=%d, event=%d, state=%d", button_id, button_event, device_state);

    const button_action_descriptor_t* desc = action_descriptors;
    size_t num_actions = sizeof(action_descriptors) / sizeof(action_descriptors[0]);

    for (size_t i = 0; i < num_actions; i++, desc++) {
        if (desc->state == device_state && desc->button_id == button_id && desc->button_event == button_event) {
            ESP_LOGI(TAG, "Executing: %s", desc->description);
            desc->handler();
            return;
        }
    }

    ESP_LOGW(TAG, "No handler found for state=%d, btn=%d, event=%d",
        device_state, button_id, button_event);
}

static void periodic_timer_callback(void* arg)
{
    button_ticks();
}

// 新的统一按钮回调函数
void button_callback_handler(void* btn)
{
    uint8_t button_id = ((Button*)btn)->button_id;
    uint8_t event = ((Button*)btn)->event;

    // 特殊处理睡眠功能（双击按钮1）
    if (button_id == 1 && event == DOUBLE_CLICK) {
        gpio_set_level(gpio_num_t(user_config.periph_pwr_gpio_num), 1);
        gpio_pullup_en(gpio_num_t(user_config.periph_pwr_gpio_num));
        ESP_LOGW(TAG, "Into sleep");
        set_device_state(DEVICE_STATE_SLEEP);
        vTaskDelay(pdMS_TO_TICKS(100));
        esp_deep_sleep_start();
        return;
    }

    // 使用状态驱动的处理方式
    handle_button_action(button_id, event);
}

void scan_button_task(void* pvParameters)
{
    BaseType_t core_id = xPortGetCoreID(); // 返回当前任务所在的核心 ID
    ESP_LOGI(TAG, "Task is running on core %d.", core_id);

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE; // 禁用中断
    io_conf.pin_bit_mask = (1ULL << user_config.up_key_gpio_num) | (1ULL << user_config.down_key_gpio_num); // 设置 GPIO
    io_conf.mode = GPIO_MODE_INPUT; // 设置为输入模式
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE; // 启用上拉电阻
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; // 禁用下拉电阻
    gpio_config(&io_conf);

    button_init(&button0, read_button_GPIO, 0, 0);
    button_init(&button1, read_button_GPIO, 0, 1);

    // 使用统一的回调函数处理所有按钮事件
    button_attach(&button0, SINGLE_CLICK, button_callback_handler);
    button_attach(&button0, PRESS_DOWN, button_callback_handler);
    button_attach(&button0, PRESS_UP, button_callback_handler);
    button_attach(&button0, LONG_PRESS_HOLD, button_callback_handler);

    button_attach(&button1, SINGLE_CLICK, button_callback_handler);
    button_attach(&button1, PRESS_DOWN, button_callback_handler);
    button_attach(&button1, PRESS_UP, button_callback_handler);
    button_attach(&button1, DOUBLE_CLICK, button_callback_handler);
    button_attach(&button1, LONG_PRESS_HOLD, button_callback_handler);

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
