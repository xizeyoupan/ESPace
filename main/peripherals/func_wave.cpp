#include "func_wave.h"

#include "espace_define.h"

#include "driver/dac_continuous.h"

#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"

#include "stdint.h"
#include "string.h"

#include "service/io_lock_service.h"

static const char* TAG = "FUNC WAVE";
extern user_config_t user_config;

dac_cont_wav_t dac_cont_wav;
dac_continuous_handle_t dac_continuous_handle;
dac_continuous_config_t dac_continuous_config;
uint8_t enabled = 0;

void start_dac_continuous_by_dma(uint32_t freq, uint8_t chan, user_def_err_t* user_err)
{
    esp_err_t ret = ESP_OK;

    if (chan == 0) {
        try_to_lock_io(25, FUNC_GEN_HOLDER, 0, user_err);
        if (user_err && user_err->esp_err != ESP_OK) {
            goto err;
        }
        dac_continuous_config.chan_mask = DAC_CHANNEL_MASK_CH0;
        dac_continuous_config.chan_mode = DAC_CHANNEL_MODE_SIMUL;
    } else if (chan == 1) {
        try_to_lock_io(26, FUNC_GEN_HOLDER, 1, user_err);
        if (user_err && user_err->esp_err != ESP_OK) {
            goto err;
        }
        dac_continuous_config.chan_mask = DAC_CHANNEL_MASK_CH1;
        dac_continuous_config.chan_mode = DAC_CHANNEL_MODE_SIMUL;
    } else {
        try_to_lock_io(25, FUNC_GEN_HOLDER, 0, user_err);
        if (user_err && user_err->esp_err != ESP_OK) {
            goto err;
        }
        try_to_lock_io(26, FUNC_GEN_HOLDER, 1, user_err);
        if (user_err && user_err->esp_err != ESP_OK) {
            goto err;
        }
        dac_continuous_config.chan_mask = DAC_CHANNEL_MASK_ALL;
        dac_continuous_config.chan_mode = DAC_CHANNEL_MODE_ALTER;
    }

    dac_continuous_config.desc_num = 6;
    dac_continuous_config.buf_size = dac_cont_wav.length;
    dac_continuous_config.freq_hz = freq * dac_cont_wav.length;
    dac_continuous_config.offset = 0;
    dac_continuous_config.clk_src = DAC_DIGI_CLK_SRC_APLL;

    if (enabled) {
        ESP_GOTO_ON_ERROR(dac_continuous_disable(dac_continuous_handle), err, TAG, "");
        ESP_GOTO_ON_ERROR(dac_continuous_del_channels(dac_continuous_handle), err, TAG, "");
    }

    ESP_GOTO_ON_ERROR(dac_continuous_new_channels(&dac_continuous_config, &dac_continuous_handle), err, TAG, "");
    ESP_GOTO_ON_ERROR(dac_continuous_enable(dac_continuous_handle), err, TAG, "");
    ESP_GOTO_ON_ERROR(dac_continuous_write_cyclically(dac_continuous_handle, dac_cont_wav.data_wav, dac_cont_wav.length, NULL), err, TAG, "");
    enabled = 1;

err:
    if (user_err && user_err->esp_err == ESP_OK) {
        user_err->esp_err = ret;
    }
}

void stop_dac_continuous_by_dma(user_def_err_t* user_err)
{
    esp_err_t ret = ESP_OK;
    user_err->esp_err = ret;

    if (enabled == 0) {
        return;
    }

    ESP_GOTO_ON_ERROR(dac_continuous_disable(dac_continuous_handle), err, TAG, "");
    ESP_GOTO_ON_ERROR(dac_continuous_del_channels(dac_continuous_handle), err, TAG, "");
    enabled = 0;

    switch (dac_continuous_config.chan_mask) {
    case DAC_CHANNEL_MASK_CH0:
        if (io_mutex_unlock(25, FUNC_GEN_HOLDER, 0) == pdFALSE) {
            ret = ESP_ERR_INVALID_STATE;
        }
        break;
    case DAC_CHANNEL_MASK_CH1:
        if (io_mutex_unlock(26, FUNC_GEN_HOLDER, 1) == pdFALSE) {
            ret = ESP_ERR_INVALID_STATE;
        }
        break;
    case DAC_CHANNEL_MASK_ALL:
        if (io_mutex_unlock(25, FUNC_GEN_HOLDER, 0) == pdFALSE) {
            ret = ESP_ERR_INVALID_STATE;
        }
        if (io_mutex_unlock(26, FUNC_GEN_HOLDER, 1) == pdFALSE) {
            ret = ESP_ERR_INVALID_STATE;
        }
        break;
    default:
        break;
    }

err:
    if (user_err && user_err->esp_err == ESP_OK) {
        user_err->esp_err = ret;
    }
}
