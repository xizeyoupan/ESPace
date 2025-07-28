#ifndef __LEDC_PWM_H__
#define __LEDC_PWM_H__

#include "user_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LEDC_TIMER_CHANNEL_NUM 4
#define LEDC_CHANNEL_NUM LEDC_TIMER_CHANNEL_NUM

void get_ledc_timer_config_by_index(int index, ledc_timer_config_t* config);
esp_err_t set_ledc_timer_config_by_index(int index, ledc_mode_t speed, uint32_t freq_hz);
void get_ledc_channel_config_by_index(int index, ledc_channel_config_t* config);
esp_err_t set_ledc_channel_config_by_index(int index, int gpio_num, ledc_timer_t timer_sel, ledc_mode_t speed_mode, uint32_t duty, int hpoint);
esp_err_t clear_ledc_channel_config_by_index(int index);


#ifdef __cplusplus
}
#endif

#endif