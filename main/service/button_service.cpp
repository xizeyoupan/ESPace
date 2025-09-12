#include "button_service.h"

#include "espace_define.h"

#include "driver/gpio.h"

#include "esp_err.h"
#include "esp_log.h"

#include "stdint.h"
#include "string.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "task/mpu_task.h"

#include "multi_button.h"

extern struct Button button0;
extern struct Button button1;

static const char* TAG = "BTN SERVICE";
extern EventGroupHandle_t button_event_group;
extern user_config_t user_config;

extern mpu_command_t received_command;
extern EventGroupHandle_t x_mpu_event_group;

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

void btn_single_click_handler(void* btn)
{
    switch ((((Button*)btn)->button_id)) {
    case 0:
        // ESP_LOGI(TAG, "btn 0 single click");
        xEventGroupSetBits(button_event_group, BTN0_SINGLE_CLICK_BIT);
        break;
    case 1:
        // ESP_LOGI(TAG, "btn 1 single click");
        xEventGroupSetBits(button_event_group, BTN1_SINGLE_CLICK_BIT);
        break;
    default:
        break;
    }
}

void btn_press_down_up_handler(void* btn)
{
    switch ((((Button*)btn)->button_id)) {
    case 0:
        switch (((Button*)btn)->event) {
        case PRESS_DOWN:
            xEventGroupSetBits(button_event_group, BTN0_DOWN_BIT);
            xEventGroupSetBits(x_mpu_event_group, MPU_SAMPLING_START_BIT);
            // ESP_LOGI(TAG, "btn 0 press down");
            break;
        case PRESS_UP:
            xEventGroupClearBits(button_event_group, BTN0_DOWN_BIT);
            xEventGroupSetBits(x_mpu_event_group, MPU_SAMPLING_STOP_BIT);
            // ESP_LOGI(TAG, "btn 0 press up");
            break;
        default:
            break;
        }
        break;
    case 1:
        switch (((Button*)btn)->event) {
        case PRESS_DOWN:
            xEventGroupSetBits(button_event_group, BTN1_DOWN_BIT);
            // ESP_LOGI(TAG, "btn 1 press down");
            break;
        case PRESS_UP:
            xEventGroupClearBits(button_event_group, BTN1_DOWN_BIT);
            // ESP_LOGI(TAG, "btn 1 press up");
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
}
