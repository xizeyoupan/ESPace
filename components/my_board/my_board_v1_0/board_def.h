/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2020 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef _AUDIO_BOARD_DEFINITION_H_
#define _AUDIO_BOARD_DEFINITION_H_

#define BUTTON_VOLUP_ID -1      /* You need to define the GPIO pins of your board */
#define BUTTON_VOLDOWN_ID -1    /* You need to define the GPIO pins of your board */
#define BUTTON_MUTE_ID -1       /* You need to define the GPIO pins of your board */
#define BUTTON_SET_ID -1        /* You need to define the GPIO pins of your board */
#define BUTTON_MODE_ID -1       /* You need to define the GPIO pins of your board */
#define BUTTON_PLAY_ID -1       /* You need to define the GPIO pins of your board */
#define PA_ENABLE_GPIO -1      /* You need to define the GPIO pins of your board */
#define ADC_DETECT_GPIO -1     /* You need to define the GPIO pins of your board */
#define BATTERY_DETECT_GPIO -1 /* You need to define the GPIO pins of your board */
#define SDCARD_INTR_GPIO -1    /* You need to define the GPIO pins of your board */

#define SDCARD_OPEN_FILE_NUM_MAX 5

#define BOARD_PA_GAIN (10) /* Power amplifier gain defined by board (dB) */

#define SDCARD_PWR_CTRL -1
#define ESP_SD_PIN_CLK -1
#define ESP_SD_PIN_CMD -1
#define ESP_SD_PIN_D0 -1
#define ESP_SD_PIN_D1 -1
#define ESP_SD_PIN_D2 -1
#define ESP_SD_PIN_D3 -1
#define ESP_SD_PIN_D4 -1
#define ESP_SD_PIN_D5 -1
#define ESP_SD_PIN_D6 -1
#define ESP_SD_PIN_D7 -1
#define ESP_SD_PIN_CD -1
#define ESP_SD_PIN_WP -1

#if (defined(CONFIG_IDF_TARGET_ESP32))
#define I2S_BCK_GPIO GPIO_NUM_17
#define I2S_WS_GPIO GPIO_NUM_9
#define I2S_DATA_OUT_GPIO GPIO_NUM_16
#define AUDIO_VOLUE_DOWM_GPIO GPIO_NUM_0
#elif (defined(CONFIG_IDF_TARGET_ESP32C3))
#define I2S_BCK_GPIO GPIO_NUM_0
#define I2S_WS_GPIO GPIO_NUM_18
#define I2S_DATA_OUT_GPIO GPIO_NUM_3
#define AUDIO_VOLUE_DOWM_GPIO GPIO_NUM_9
#else
#define I2S_BCK_GPIO GPIO_NUM_NC
#define I2S_WS_GPIO GPIO_NUM_NC
#define I2S_DATA_OUT_GPIO GPIO_NUM_NC
#define AUDIO_VOLUE_DOWM_GPIO GPIO_NUM_NC
#endif // (defined (CONFIG_IDF_TARGET_ESP32))

extern audio_hal_func_t AUDIO_NEW_CODEC_DEFAULT_HANDLE;

#define AUDIO_CODEC_DEFAULT_CONFIG() {       \
    .adc_input = AUDIO_HAL_ADC_INPUT_LINE1,  \
    .dac_output = AUDIO_HAL_DAC_OUTPUT_ALL,  \
    .codec_mode = AUDIO_HAL_CODEC_MODE_BOTH, \
    .i2s_iface = {                           \
        .mode = AUDIO_HAL_MODE_SLAVE,        \
        .fmt = AUDIO_HAL_I2S_NORMAL,         \
        .samples = AUDIO_HAL_48K_SAMPLES,    \
        .bits = AUDIO_HAL_BIT_LENGTH_16BITS, \
    },                                       \
};

#define INPUT_KEY_NUM 4 /* You need to define the number of input buttons on your board */

#define INPUT_KEY_DEFAULT_INFO() {            \
    {                                         \
        .type = PERIPH_ID_ADC_BTN,            \
        .user_id = INPUT_KEY_USER_ID_VOLUP,   \
        .act_id = BUTTON_VOLUP_ID,            \
    },                                        \
    {                                         \
        .type = PERIPH_ID_ADC_BTN,            \
        .user_id = INPUT_KEY_USER_ID_VOLDOWN, \
        .act_id = BUTTON_VOLDOWN_ID,          \
    },                                        \
    {                                         \
        .type = PERIPH_ID_ADC_BTN,            \
        .user_id = INPUT_KEY_USER_ID_MUTE,    \
        .act_id = BUTTON_MUTE_ID,             \
    },                                        \
    {                                         \
        .type = PERIPH_ID_ADC_BTN,            \
        .user_id = INPUT_KEY_USER_ID_SET,     \
        .act_id = BUTTON_SET_ID,              \
    },                                        \
}

#endif
