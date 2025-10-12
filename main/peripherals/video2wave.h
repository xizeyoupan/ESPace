#ifndef __VIDEO2WAVE_H__
#define __VIDEO2WAVE_H__

#include "esp_err.h"
#include "stdint.h"

#define VIDEO_WIDTH (200)
#define VIDEO_HEGIHT (150)
#define VIDEO_PIXEL_TOTAL_NUM (VIDEO_WIDTH * VIDEO_HEGIHT)
#define VIDEO_PIECE_NUM (15)
#define VIDEO_PIXEL_NUM_PER_PIECE (VIDEO_WIDTH * VIDEO_HEGIHT / VIDEO_PIECE_NUM)
#define VIDEO_PIXEL_NUM_PER_PIECE_HORIZONTAL VIDEO_WIDTH
#define VIDEO_PIXEL_NUM_PER_PIECE_VERTICAL (VIDEO_HEGIHT / VIDEO_PIECE_NUM)
#define VIDEO_OUTPUT_DATA_NUM (64 * VIDEO_PIECE_NUM + VIDEO_PIXEL_TOTAL_NUM)

#ifdef __cplusplus
extern "C" {
#endif

void fill_output_data(int piece_index, int i, int j);
void porcess_data();

#ifdef __cplusplus
}
#endif

#endif