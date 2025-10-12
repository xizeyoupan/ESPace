#include "user_uart.h"

#include "peripherals/func_wave.h"
#include "peripherals/video2wave.h"

#include "service/io_lock_service.h"

#include "driver/uart.h"

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/message_buffer.h"

#include "string.h"

#define BUFFER_SIZE (1024)

static const char* TAG = "UART";
extern MessageBufferHandle_t xMessageBufferReqSend;

extern uint8_t video_to_wave_output_data[VIDEO_OUTPUT_DATA_NUM];

void uart0_init()
{
    ESP_LOGI(TAG, "Initialize UART0");
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_driver_install(UART_NUM_0, VIDEO_PIXEL_TOTAL_NUM, BUFFER_SIZE * 2, 0, NULL, 0);
    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    user_def_err_t user_ret;
    enable_dac_continuous_by_dma(500, 2048, 0, &user_ret, VIDEO2WAVE_HOLDER);
}

uint8_t flag = 0;
extern uint8_t video_to_wave_input_data[VIDEO_PIXEL_TOTAL_NUM / 8];

void test_uart(void* pvParameters)
{
    uint8_t uart0_data;
    size_t input_data_addr = 0;
    while (1) {
        int len = uart_read_bytes(UART_NUM_0, &uart0_data, 1, portMAX_DELAY);
        if (len <= 0)
            continue;

        if (flag) {
            video_to_wave_input_data[input_data_addr++] = uart0_data;
            // if (input_data_addr % 100 == 0) {
            //     ESP_LOGI(TAG, "get addr %d", input_data_addr);
            // }

            if (input_data_addr >= sizeof(video_to_wave_input_data)) {
                porcess_data();
                ESP_LOGI(TAG, "process 1 frame");
                start_dac_write_async_by_dma(video_to_wave_output_data, sizeof(video_to_wave_output_data), NULL);
                flag = 0;
            }
        }
        if (flag == 0 && uart0_data == 0xAA) {
            flag = 1;
            input_data_addr = 0;
            ESP_LOGI(TAG, "get flagggggggggggggg");
        }
    }
}
