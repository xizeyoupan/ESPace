#include "button.h"

struct Button button0;
struct Button button1;

static const char *TAG = "BTN TASK";
EventGroupHandle_t button_event_group;

uint8_t read_button_GPIO(uint8_t button_id)
{
    switch (button_id)
    {
    case 0:
        return gpio_get_level(BUTTON0_GPIO_NUM);
        break;
    case 1:
        return gpio_get_level(BUTTON1_GPIO_NUM);
        break;
    default:
        return 0;
        break;
    }
}

void btn_single_click_handler(void *btn)
{
    switch ((((Button *)btn)->button_id))
    {
    case 0:
        ESP_LOGI(TAG, "btn 0 single click");
        xEventGroupSetBits(button_event_group, BTN0_SINGLE_CLICK_BIT);
        break;
    case 1:
        ESP_LOGI(TAG, "btn 1 single click");
        xEventGroupSetBits(button_event_group, BTN1_SINGLE_CLICK_BIT);
        break;
    default:
        break;
    }
}

void btn_press_down_up_handler(void *btn)
{
    switch ((((Button *)btn)->button_id))
    {
    case 0:
        switch (((Button *)btn)->event)
        {
        case PRESS_DOWN:
            xEventGroupSetBits(button_event_group, BTN0_DOWN_BIT);
            ESP_LOGI(TAG, "btn 0 press down");
            break;
        case PRESS_UP:
            xEventGroupClearBits(button_event_group, BTN0_DOWN_BIT);
            ESP_LOGI(TAG, "btn 0 press up");
            break;
        default:
            break;
        }
        break;
    case 1:
        switch (((Button *)btn)->event)
        {
        case PRESS_DOWN:
            xEventGroupSetBits(button_event_group, BTN1_DOWN_BIT);
            ESP_LOGI(TAG, "btn 1 press down");
            break;
        case PRESS_UP:
            xEventGroupClearBits(button_event_group, BTN1_DOWN_BIT);
            ESP_LOGI(TAG, "btn 1 press up");
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
}

void scan_button_task(void *pvParameters)
{
    BaseType_t core_id = xPortGetCoreID(); // 返回当前任务所在的核心 ID
    ESP_LOGI(TAG, "Task is running on core %d.", core_id);

    button_event_group = xEventGroupCreate();

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;                                          // 禁用中断
    io_conf.pin_bit_mask = (1ULL << BUTTON0_GPIO_NUM) | (1ULL << BUTTON1_GPIO_NUM); // 设置 GPIO
    io_conf.mode = GPIO_MODE_INPUT;                                                 // 设置为输入模式
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;                                        // 启用上拉电阻
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;                                   // 禁用下拉电阻
    gpio_config(&io_conf);

    button_init(&button0, read_button_GPIO, 0, 0);
    button_init(&button1, read_button_GPIO, 0, 1);

    button_attach(&button0, SINGLE_CLICK, btn_single_click_handler);
    button_attach(&button1, SINGLE_CLICK, btn_single_click_handler);

    button_attach(&button0, PRESS_UP, btn_press_down_up_handler);
    button_attach(&button0, PRESS_DOWN, btn_press_down_up_handler);

    button_attach(&button1, PRESS_UP, btn_press_down_up_handler);
    button_attach(&button1, PRESS_DOWN, btn_press_down_up_handler);

    button_start(&button0);
    button_start(&button1);

    while (1)
    {
        button_ticks();
        vTaskDelay(pdMS_TO_TICKS(TICKS_INTERVAL));
    }

    vTaskDelete(NULL);
}
