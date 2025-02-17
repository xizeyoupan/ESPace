#include "button.h"

struct Button button0;
struct Button button1;

uint8_t read_button_GPIO(uint8_t button_id)
{
    switch (button_id)
    {
    case 0:
        return gpio_get_level(BUTTON0_GPIO_NUM);
        break;
    case 1:
        return gpio_get_level(BUTTON0_GPIO_NUM);
        break;
    default:
        return 0;
        break;
    }
}

void scan_button_task(void *pvParameters)
{
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;                                          // 禁用中断
    io_conf.pin_bit_mask = (1ULL << BUTTON0_GPIO_NUM) | (1ULL << BUTTON1_GPIO_NUM); // 设置 GPIO
    io_conf.mode = GPIO_MODE_INPUT;                                                 // 设置为输入模式
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;                                        // 启用上拉电阻
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;                                   // 禁用下拉电阻
    gpio_config(&io_conf);

    button_init(&button0, gpio_get_level, 0, 0);
    button_init(&button1, gpio_get_level, 0, 1);

    while (1)
    {
        vTaskDelay(10);
    }

    vTaskDelete(NULL);
}
