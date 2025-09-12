#ifndef __FUNC_WAVE_H__
#define __FUNC_WAVE_H__

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DAC_CONT_CHANNEL_NUM 2

typedef struct {
    uint8_t* data_wav;
    uint16_t length;
} dac_cont_wav_t;

void dac_continuous_by_dma(uint32_t freq);

#ifdef __cplusplus
}
#endif

#endif