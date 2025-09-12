#ifndef __COSINE_WAVE_H__
#define __COSINE_WAVE_H__

#include "driver/dac_cosine.h"
#include "esp_err.h"
#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DAC_COSINE_CHANNEL_NUM 2

void get_cosine_config_by_index(int index, dac_cosine_config_t* config);
esp_err_t start_cosine_by_index(int index, uint32_t freq_hz, dac_cosine_atten_t atten, dac_cosine_phase_t phase, int8_t offset);
esp_err_t stop_cosine_by_index(int index);

#ifdef __cplusplus
}
#endif

#endif