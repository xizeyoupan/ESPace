#include "func_wave.h"

#include "espace_define.h"

#include "driver/dac_continuous.h"

#include "esp_err.h"
#include "esp_log.h"

#include "stdint.h"
#include "string.h"

#include "service/io_lock_service.h"

static const char* TAG = "FUNC WAVE";
extern user_config_t user_config;

#define WAVE_FREQ_HZ 100

dac_cont_wav_t dac_cont_wav;
dac_continuous_handle_t dac_continuous_handle;
dac_continuous_config_t dac_continuous_config;
uint8_t enabled = 0;

void dac_continuous_by_dma(uint32_t freq)
{
    dac_continuous_config.chan_mask = DAC_CHANNEL_MASK_CH1;
    dac_continuous_config.desc_num = 6;
    dac_continuous_config.buf_size = dac_cont_wav.length;
    dac_continuous_config.freq_hz = freq * dac_cont_wav.length;
    dac_continuous_config.offset = 0;
    dac_continuous_config.clk_src = DAC_DIGI_CLK_SRC_APLL;
    dac_continuous_config.chan_mode = DAC_CHANNEL_MODE_ALTER;

    esp_err_t err;
    if (enabled) {
        dac_continuous_disable(dac_continuous_handle);
        dac_continuous_del_channels(dac_continuous_handle);
    }

    err = dac_continuous_new_channels(&dac_continuous_config, &dac_continuous_handle);
    err = dac_continuous_enable(dac_continuous_handle);
    err = dac_continuous_write_cyclically(dac_continuous_handle, dac_cont_wav.data_wav, dac_cont_wav.length, NULL);
    enabled = 1;
}
