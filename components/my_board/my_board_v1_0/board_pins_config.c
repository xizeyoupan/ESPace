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

#include "audio_error.h"
#include "audio_mem.h"
#include "board.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "soc/soc_caps.h"
#include <string.h>

#include "../../../main/user_define.h"

static const char* TAG = "MY_BOARD_V1_0";
extern user_config_t user_config;

esp_err_t get_i2c_pins(i2c_port_t port, i2c_config_t* i2c_config)
{
    AUDIO_NULL_CHECK(TAG, i2c_config, return ESP_FAIL);
    if (port == I2C_NUM_0) {
        i2c_config->sda_io_num = -1;
        i2c_config->scl_io_num = -1;
    } else {
        i2c_config->sda_io_num = -1;
        i2c_config->scl_io_num = -1;
        ESP_LOGE(TAG, "i2c port %d is not supported", port);
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t get_i2s_pins(int port, board_i2s_pin_t* i2s_config)
{
    AUDIO_NULL_CHECK(TAG, i2s_config, return ESP_FAIL);
    ESP_LOGI(TAG, "Getting i2s pins");

    if (port == 0) {
        i2s_config->mck_io_num = -1;
        i2s_config->data_in_num = -1;
        i2s_config->bck_io_num = user_config.i2s_bck_gpio_num;
        i2s_config->ws_io_num = user_config.i2s_ws_gpio_num;
        i2s_config->data_out_num = user_config.i2s_dout_gpio_num;
    } else if (port == 1) {
        i2s_config->bck_io_num = -1;
        i2s_config->ws_io_num = -1;
        i2s_config->data_out_num = -1;
        i2s_config->data_in_num = -1;
    } else {
        memset(i2s_config, -1, sizeof(board_i2s_pin_t));
        ESP_LOGE(TAG, "i2s port %d is not supported", port);
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t get_spi_pins(spi_bus_config_t* spi_config, spi_device_interface_config_t* spi_device_interface_config)
{
    AUDIO_NULL_CHECK(TAG, spi_config, return ESP_FAIL);
    AUDIO_NULL_CHECK(TAG, spi_device_interface_config, return ESP_FAIL);

    spi_config->mosi_io_num = -1;
    spi_config->miso_io_num = -1;
    spi_config->sclk_io_num = -1;
    spi_config->quadwp_io_num = -1;
    spi_config->quadhd_io_num = -1;

    spi_device_interface_config->spics_io_num = -1;

    ESP_LOGW(TAG, "SPI interface is not supported");
    return ESP_OK;
}

// sdcard detect gpio
int8_t get_sdcard_intr_gpio(void)
{
    return SDCARD_INTR_GPIO;
}

// max number of sdcard open file
int8_t get_sdcard_open_file_num_max(void)
{
    return SDCARD_OPEN_FILE_NUM_MAX;
}

// volume up button
int8_t get_input_volup_id(void)
{
    return BUTTON_VOLUP_ID;
}

// volume down button
int8_t get_input_voldown_id(void)
{
    return BUTTON_VOLDOWN_ID;
}

// pa enable
int8_t get_pa_enable_gpio(void)
{
    return PA_ENABLE_GPIO;
}

// mode button
int8_t get_input_mode_id(void)
{
    return BUTTON_MODE_ID;
}

// set button
int8_t get_input_set_id(void)
{
    return BUTTON_SET_ID;
}

// play button
int8_t get_input_play_id(void)
{
    return BUTTON_PLAY_ID;
}

// mute button
int8_t get_input_mute_id(void)
{
    return BUTTON_MUTE_ID;
}
