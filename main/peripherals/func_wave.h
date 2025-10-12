#ifndef __FUNC_WAVE_H__
#define __FUNC_WAVE_H__

#include "espace_define.h"

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DAC_CONT_CHANNEL_NUM 2

typedef struct {
    uint8_t* data_wav;
    uint16_t length;
} dac_cont_wav_t;

void enable_dac_continuous_by_dma(uint32_t freq, uint32_t bufsize, uint8_t chan, user_def_err_t* user_err, const char* holder);
void disable_dac_continuous_by_dma(user_def_err_t* user_err, const char* holder);
void start_dac_continuous_by_dma(uint8_t* buf, uint32_t bufsize, user_def_err_t* user_err);
void start_dac_write_async_by_dma(uint8_t* buf, uint32_t bufsize, user_def_err_t* user_err);

#ifdef __cplusplus
}
#endif

#endif