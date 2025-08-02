#include "cosine_wave.h"

static const char* TAG = "COSINE WAVE";
extern user_config_t user_config;

dac_cosine_handle_t dac_cosine_handles[DAC_COSINE_CHANNEL_NUM];
dac_cosine_config_t dac_cosine_configs[DAC_COSINE_CHANNEL_NUM];

void get_cosine_config_by_index(int index, dac_cosine_config_t* config)
{
    if (index >= 0 && index < DAC_COSINE_CHANNEL_NUM) {
        *config = dac_cosine_configs[index];
        config->phase = config->phase ? config->phase : DAC_COSINE_PHASE_0;
    }
}

esp_err_t start_cosine_by_index(int index, uint32_t freq_hz, dac_cosine_atten_t atten, dac_cosine_phase_t phase, int8_t offset)
{
    if (index < 0 || index >= DAC_COSINE_CHANNEL_NUM) {
        ESP_LOGE(TAG, "Invalid channel index: %d", index);
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = ESP_OK;

    char io_holder[HOLDER_STRING_SIZE] = { 0 };
    int io_holder_channel;
    int gpio_num = index + 25;

    io_mutex_get_status(gpio_num, io_holder, &io_holder_channel);

    if (strlen(io_holder) == 0) {
        if (io_mutex_lock(gpio_num, DAC_COSINE_HOLDER, index, portMAX_DELAY) != pdTRUE) {
            ESP_LOGE(TAG, "Failed to lock GPIO %d for DAC COSINE", gpio_num);
            return ESP_ERR_INVALID_STATE;
        }
    } else if (strcmp(io_holder, DAC_COSINE_HOLDER) == 0 && io_holder_channel == index) {
        ret = dac_cosine_stop(dac_cosine_handles[index]);
        ret = dac_cosine_del_channel(dac_cosine_handles[index]);
    } else {
        ESP_LOGE(TAG, "GPIO %d is already locked by %s on channel %d", gpio_num, io_holder, io_holder_channel);
        return ESP_ERR_INVALID_STATE;
    }

    dac_cosine_config_t cfg = {
        .chan_id = (dac_channel_t)index,
        .freq_hz = freq_hz,
        .clk_src = DAC_COSINE_CLK_SRC_DEFAULT,
        .atten = atten,
        .phase = phase,
        .offset = offset,
    };
    cfg.flags.force_set_freq = true;

    dac_cosine_configs[index] = cfg;

    ret = dac_cosine_new_channel(dac_cosine_configs + index, dac_cosine_handles + index);
    ret = dac_cosine_start(dac_cosine_handles[index]);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure DAC cosine channel %d", index);
        return ret;
    }

    ESP_LOGI(TAG, "Started cosine wave on channel %d (freq=%luHz, atten=%d, phase=%d, offset=%d)",
        index, freq_hz, atten, phase, offset);
    return ESP_OK;
}

esp_err_t stop_cosine_by_index(int index)
{
    if (index < 0 || index >= DAC_COSINE_CHANNEL_NUM) {
        ESP_LOGE(TAG, "Invalid channel index: %d", index);
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = dac_cosine_stop(dac_cosine_handles[index]);
    ret = dac_cosine_del_channel(dac_cosine_handles[index]);

    int gpio_num = index + 25;

    io_mutex_unlock(gpio_num, DAC_COSINE_HOLDER, index);
    memset(&dac_cosine_handles[index], 0, sizeof(dac_cosine_handle_t));
    memset(&dac_cosine_configs[index], 0, sizeof(dac_cosine_config_t));

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop DAC cosine channel %d", index);
        return ret;
    }

    ESP_LOGI(TAG, "Stopped cosine wave on channel %d", index);
    return ESP_OK;
}