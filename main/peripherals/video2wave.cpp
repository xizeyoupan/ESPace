#include "video2wave.h"

#include "espace_define.h"

#include "driver/dac_continuous.h"

#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"

#include "stdint.h"
#include "string.h"

#include "service/io_lock_service.h"

static const char* TAG = "VIDOE TO WAVE";
extern user_config_t user_config;

dac_continuous_handle_t dac_v2w_handle;
dac_continuous_config_t dac_v2w_handle_config;

uint8_t video_to_wave_input_data[VIDEO_PIXEL_TOTAL_NUM / 8];
uint8_t video_to_wave_output_data[VIDEO_OUTPUT_DATA_NUM];
size_t output_addr = 0;

uint8_t get_color(int piece_index, int i, int j)
{
    int bitIndex = piece_index * VIDEO_PIXEL_NUM_PER_PIECE + i * VIDEO_PIXEL_NUM_PER_PIECE_HORIZONTAL + j; // 展平为一维索引
    int bytePos = bitIndex / 8; // 字节位置
    int bitPos = 7 - (bitIndex % 8); // 高位先存
    int black = (video_to_wave_input_data[bytePos] >> bitPos) & 0x01;
    return black;
}

void porcess_data()
{
    output_addr = 0;
    int upper, lower;

    for (int piece_index = 0; piece_index < VIDEO_PIECE_NUM; piece_index++) {

        memset(video_to_wave_output_data + output_addr, 0xff, 8);
        output_addr += 8;
        memset(video_to_wave_output_data + output_addr, 0, 8);
        output_addr += 8;

        for (size_t j = 0; j < VIDEO_WIDTH; j++) {
            if (piece_index < VIDEO_PIECE_NUM / 2) {
                upper = 0;
                lower = 0;
            } else {
                upper = VIDEO_HEGIHT;
                lower = VIDEO_HEGIHT;
            }

            for (size_t i = 0; i < VIDEO_PIXEL_NUM_PER_PIECE_VERTICAL; i++) {
                if (!get_color(piece_index, i, j)) {
                    upper = piece_index * VIDEO_PIXEL_NUM_PER_PIECE_VERTICAL + i;
                    break;
                }
            }
            for (int i = VIDEO_PIXEL_NUM_PER_PIECE_VERTICAL - 1; i >= 0; i--) {
                if (!get_color(piece_index, i, j)) {
                    lower = piece_index * VIDEO_PIXEL_NUM_PER_PIECE_VERTICAL + i;
                    break;
                }
            }

            for (size_t m = 0; m < 4; m++) {
                video_to_wave_output_data[output_addr++] = (1.0 - 1.0 * upper / VIDEO_HEGIHT) * 0xff;
                video_to_wave_output_data[output_addr++] = (1.0 - 1.0 * lower / VIDEO_HEGIHT) * 0xff;
            }
        }

        memset(video_to_wave_output_data + output_addr, 0, 48);
        output_addr += 48;
    }

    // for (int piece_index = 0; piece_index < VIDEO_PIECE_NUM; piece_index++) {
    //     // 按`W`形遍历

    //     memset(video_to_wave_output_data + output_addr, 0, 4);
    //     output_addr += 192;
    //     memset(video_to_wave_output_data + output_addr, 0xff, 4);
    //     output_addr += 4;
    //     memset(video_to_wave_output_data + output_addr, 0, 4);
    //     output_addr += 4;

    //     for (int j = 0; j < VIDEO_WIDTH; j += 2) {
    //         for (int i = 0; i < VIDEO_PIXEL_NUM_PER_PIECE_VERTICAL; i++) {
    //             fill_output_data(piece_index, i, j);
    //         }
    //         j++;
    //         for (int i = VIDEO_PIXEL_NUM_PER_PIECE_VERTICAL - 1; i >= 0; i--) {
    //             fill_output_data(piece_index, i, j);
    //         }
    //     }
    // }
}

void fill_output_data(int piece_index, int i, int j)
{
    int bitIndex = piece_index * VIDEO_PIXEL_NUM_PER_PIECE + i * VIDEO_PIXEL_NUM_PER_PIECE_HORIZONTAL + j; // 展平为一维索引
    int bytePos = bitIndex / 8; // 字节位置
    int bitPos = 7 - (bitIndex % 8); // 高位先存
    int black = (video_to_wave_input_data[bytePos] >> bitPos) & 0x01;

    if (!black) {
        float pos_y = piece_index * VIDEO_PIXEL_NUM_PER_PIECE_VERTICAL + i;
        float data = (1 - pos_y / VIDEO_HEGIHT) * 255;
        video_to_wave_output_data[output_addr++] = data;
    } else {
        if (piece_index < VIDEO_PIECE_NUM / 2) {
            video_to_wave_output_data[output_addr++] = 255;
        } else {
            video_to_wave_output_data[output_addr++] = 0;
        }
    }
}
