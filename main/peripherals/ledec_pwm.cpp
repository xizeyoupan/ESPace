#include "ledc_pwm.h"

static const char* TAG = "LEDC PWM";
extern user_config_t user_config;

ledc_timer_config_t ledc_timers[LEDC_TIMER_CHANNEL_NUM];
ledc_channel_config_t ledc_channels[LEDC_CHANNEL_NUM];

void get_ledc_timer_config_by_index(int index, ledc_timer_config_t* config)
{
    if (index < 0 || index >= LEDC_TIMER_CHANNEL_NUM) {
        ESP_LOGE(TAG, "Invalid LEDC timer index: %d", index);
        return;
    }
    *config = ledc_timers[index];
}

void get_ledc_channel_config_by_index(int index, ledc_channel_config_t* config)
{
    if (index < 0 || index >= LEDC_CHANNEL_NUM) {
        ESP_LOGE(TAG, "Invalid LEDC channel index: %d", index);
        return;
    }
    *config = ledc_channels[index];
}

esp_err_t set_ledc_timer_config_by_index(int index, ledc_mode_t speed, uint32_t freq_hz)
{
    if (index < 0 || index >= LEDC_TIMER_CHANNEL_NUM) {
        ESP_LOGE(TAG, "Invalid LEDC timer index: %d", index);
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret = ESP_OK;

    ledc_timers[index].timer_num = (ledc_timer_t)index;
    ledc_timers[index].speed_mode = speed;

    if (ledc_timers[index].clk_cfg != 0) {
        ledc_timers[index].deconfigure = true;
        ret = ledc_timer_pause(ledc_timers[index].speed_mode, ledc_timers[index].timer_num);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to pause LEDC timer: %s", esp_err_to_name(ret));
            // return ret;
        }
        ret = ledc_timer_config(&ledc_timers[index]);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to deconfigure LEDC timer: %s", esp_err_to_name(ret));
            // return ESP_FAIL;
        }
    }

    uint32_t duty_resolution = ledc_find_suitable_duty_resolution(80000000, freq_hz);
    ledc_timers[index].deconfigure = false;
    ledc_timers[index].duty_resolution = (ledc_timer_bit_t)duty_resolution;
    ledc_timers[index].freq_hz = freq_hz;
    ledc_timers[index].clk_cfg = LEDC_USE_APB_CLK;

    ret = ledc_timer_config(&ledc_timers[index]);
    if (ret != ESP_OK) {
        memset(&ledc_timers[index], 0, sizeof(ledc_timer_config_t));
        ESP_LOGE(TAG, "Failed to set LEDC timer config: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "LEDC timer %d configured successfully with frequency %lu Hz", index, freq_hz);
    }

    return ret;
}

esp_err_t set_ledc_channel_config_by_index(int index, int gpio_num, ledc_timer_t timer_sel, ledc_mode_t speed_mode, uint32_t duty, int hpoint)
{
    if (index < 0 || index >= LEDC_CHANNEL_NUM) {
        ESP_LOGE(TAG, "Invalid LEDC channel index: %d", index);
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret = ESP_OK;

    char io_holder[HOLDER_STRING_SIZE];
    int io_holder_channel;
    int io_of_channel = ledc_channels[index].gpio_num;

    io_mutex_get_status(io_of_channel, io_holder, &io_holder_channel);

    if (
        (io_of_channel == gpio_num)
        || (io_of_channel != gpio_num && (strlen(io_holder) == 0 || (strcmp(io_holder, LEDC_PWM_HOLDER) != 0 || io_holder_channel != index)))) {

        io_mutex_get_status(gpio_num, io_holder, &io_holder_channel);
        if (strlen(io_holder) == 0) {
            if (io_mutex_lock(gpio_num, LEDC_PWM_HOLDER, index, portMAX_DELAY) != pdTRUE) {
                ESP_LOGE(TAG, "Failed to lock GPIO %d for LEDC PWM", gpio_num);
                return ESP_ERR_INVALID_STATE;
            }
        } else if (strcmp(io_holder, LEDC_PWM_HOLDER) != 0 || io_holder_channel != index) {
            ESP_LOGE(TAG, "GPIO %d is already locked by %s on channel %d", gpio_num, io_holder, io_holder_channel);
            return ESP_ERR_INVALID_STATE;
        }

    } else if (io_of_channel != gpio_num) {
        ESP_LOGE(TAG, "Stop LEDC PWM on channel %d to release GPIO %d", index, io_of_channel);
        return ESP_ERR_INVALID_STATE;
    }

    ledc_channels[index].gpio_num = gpio_num;
    ledc_channels[index].speed_mode = speed_mode;
    ledc_channels[index].channel = (ledc_channel_t)index;
    ledc_channels[index].intr_type = LEDC_INTR_DISABLE;
    ledc_channels[index].timer_sel = timer_sel;
    ledc_channels[index].duty = duty;
    ledc_channels[index].hpoint = hpoint;

    ret = ledc_channel_config(&ledc_channels[index]);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set LEDC channel config: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "LEDC channel %d configured successfully", index);
    }
    return ret;
}

esp_err_t clear_ledc_channel_config_by_index(int index)
{
    if (index < 0 || index >= LEDC_CHANNEL_NUM) {
        ESP_LOGE(TAG, "Invalid LEDC channel index: %d", index);
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret = ESP_OK;

    ret = ledc_stop(ledc_channels[index].speed_mode, ledc_channels[index].channel, 0);
    io_mutex_unlock(ledc_channels[index].gpio_num, LEDC_PWM_HOLDER, index);

    memset(&ledc_channels[index], 0, sizeof(ledc_channel_config_t));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop LEDC channel %d: %s", index, esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "LEDC channel %d cleared successfully", index);
    }
    return ret;
}
