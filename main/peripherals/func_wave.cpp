#include "func_wave.h"

#include "espace_define.h"

#include "driver/dac_continuous.h"

#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"

#include "stdint.h"
#include "string.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "service/io_lock_service.h"

static const char* TAG = "FUNC WAVE";
extern user_config_t user_config;

dac_cont_wav_t dac_cont_wav;
dac_continuous_handle_t dac_continuous_handle;
dac_continuous_config_t dac_continuous_config;
uint8_t enabled = 0;

void enable_dac_continuous_by_dma(uint32_t freq, uint32_t bufsize, uint8_t chan, user_def_err_t* user_err, const char* holder)
{
    esp_err_t ret = ESP_OK;

    if (chan == 0) {
        try_to_lock_io(25, holder, 0, user_err);
        if (user_err && user_err->esp_err != ESP_OK) {
            goto err;
        }
        dac_continuous_config.chan_mask = DAC_CHANNEL_MASK_CH0;
        dac_continuous_config.chan_mode = DAC_CHANNEL_MODE_SIMUL;
    } else if (chan == 1) {
        try_to_lock_io(26, holder, 1, user_err);
        if (user_err && user_err->esp_err != ESP_OK) {
            goto err;
        }
        dac_continuous_config.chan_mask = DAC_CHANNEL_MASK_CH1;
        dac_continuous_config.chan_mode = DAC_CHANNEL_MODE_SIMUL;
    } else {
        try_to_lock_io(25, holder, 0, user_err);
        if (user_err && user_err->esp_err != ESP_OK) {
            goto err;
        }
        try_to_lock_io(26, holder, 1, user_err);
        if (user_err && user_err->esp_err != ESP_OK) {
            goto err;
        }
        dac_continuous_config.chan_mask = DAC_CHANNEL_MASK_ALL;
        dac_continuous_config.chan_mode = DAC_CHANNEL_MODE_ALTER;
    }

    dac_continuous_config.desc_num = 6;
    dac_continuous_config.buf_size = bufsize;
    dac_continuous_config.freq_hz = freq * bufsize;
    dac_continuous_config.offset = 0;
    dac_continuous_config.clk_src = DAC_DIGI_CLK_SRC_APLL;

    if (enabled) {
        ESP_GOTO_ON_ERROR(dac_continuous_disable(dac_continuous_handle), err, TAG, "");
        ESP_GOTO_ON_ERROR(dac_continuous_del_channels(dac_continuous_handle), err, TAG, "");
    }

    ESP_GOTO_ON_ERROR(dac_continuous_new_channels(&dac_continuous_config, &dac_continuous_handle), err, TAG, "");
    ESP_GOTO_ON_ERROR(dac_continuous_enable(dac_continuous_handle), err, TAG, "");
    enabled = 1;

err:
    if (user_err && user_err->esp_err == ESP_OK) {
        user_err->esp_err = ret;
    }
}

void start_dac_continuous_by_dma(uint8_t* buf, uint32_t bufsize, user_def_err_t* user_err)
{
    esp_err_t ret = ESP_OK;
    ESP_GOTO_ON_ERROR(dac_continuous_write_cyclically(dac_continuous_handle, dac_cont_wav.data_wav, dac_cont_wav.length, NULL), err, TAG, "");

err:
    if (user_err && user_err->esp_err == ESP_OK) {
        user_err->esp_err = ret;
    }
}

static size_t wave_offset = 0;

static bool IRAM_ATTR dac_on_convert_done_callback(dac_continuous_handle_t handle, const dac_event_data_t* event, void* user_data)
{
    // ESP_LOGI(TAG, "IN CALLBACK");
    size_t written = 0;
    esp_err_t ret;

    dac_cont_wav_t dac_data = *((dac_cont_wav_t*)user_data);

    if (wave_offset < dac_data.length) {
        size_t remain = dac_data.length - wave_offset;
        size_t chunk = remain > 1024 ? 1024 : remain;

        ret = dac_continuous_write_asynchronously(handle, (uint8_t*)event->buf, event->buf_size, dac_data.data_wav + wave_offset, chunk, &written);
        if (ret == ESP_OK && written > 0) {
            wave_offset += written;
        }
        return true; // 继续运行
    } else {
        // // 已经播完，停止 DAC
        // dac_continuous_disable(handle);
        // return false; // 返回 false，表示不再触发回调
        // ESP_LOGI(TAG, "IN CALLBACK FINish");

        wave_offset = 0;
        return true;
    }
}

dac_cont_wav_t dac_data;
void start_dac_write_async_by_dma(uint8_t* buf, uint32_t bufsize, user_def_err_t* user_err)
{
    esp_err_t ret = ESP_OK;

    dac_event_callbacks_t cbs = {
        .on_convert_done = dac_on_convert_done_callback,
        .on_stop = NULL,
    };

    dac_data.data_wav = buf;
    dac_data.length = bufsize;
    /* Must register the callback if using asynchronous writing */
    ESP_ERROR_CHECK(dac_continuous_register_event_callback(dac_continuous_handle, &cbs, &dac_data));

    size_t written = 0;
    ESP_ERROR_CHECK(dac_continuous_start_async_writing(dac_continuous_handle));
    wave_offset += written;
    ESP_LOGI(TAG, "DAC started, initial written=%d\n", (int)written);

    // err:
    if (user_err && user_err->esp_err == ESP_OK) {
        user_err->esp_err = ret;
    }
}

void disable_dac_continuous_by_dma(user_def_err_t* user_err, const char* holder)
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
        if (io_mutex_unlock(25, holder, 0) == pdFALSE) {
            ret = ESP_ERR_INVALID_STATE;
        }
        break;
    case DAC_CHANNEL_MASK_CH1:
        if (io_mutex_unlock(26, holder, 1) == pdFALSE) {
            ret = ESP_ERR_INVALID_STATE;
        }
        break;
    case DAC_CHANNEL_MASK_ALL:
        if (io_mutex_unlock(25, holder, 0) == pdFALSE) {
            ret = ESP_ERR_INVALID_STATE;
        }
        if (io_mutex_unlock(26, holder, 1) == pdFALSE) {
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
