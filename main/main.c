#include "user_config.h"
user_config_t user_config;

static const char *TAG = "MAIN";

#define LOG_BUFFER_LEN (1023)
extern MessageBufferHandle_t xMessageBufferToClient;

char log_buffer[LOG_BUFFER_LEN + 1];
int ws_vprintf(const char *_Format, va_list _ArgList)
{
    if (user_config.enable_ws_log) {
        log_buffer[0] = SEND_LOG_DATA_PREFIX;
        size_t len    = vsnprintf(log_buffer + 1, LOG_BUFFER_LEN, _Format, _ArgList);
        if (len > LOG_BUFFER_LEN - 1) {
            log_buffer[LOG_BUFFER_LEN] = 0;
        }
        xMessageBufferSend(xMessageBufferToClient, log_buffer, len, 0);
    }

    return vprintf(_Format, _ArgList);
}

void app_main(void)
{

    // Initialize NVS
    ESP_LOGI(TAG, "Initialize NVS");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    load_user_config();
    init_control_block();
    ws2812_init();
    set_bg_color(COLOR_YELLOW);

    // Start wifi and web server
    xTaskCreatePinnedToCore(&wand_server_task, "WIFI_LOOP", 1024 * 5, NULL, 15, NULL, 0);

    // Start ws
    xTaskCreatePinnedToCore(&websocket_send_task, "WS", 1024 * 5, NULL, 10, NULL, 0);

    vTaskDelay(pdMS_TO_TICKS(1000));
    configASSERT(xMessageBufferToClient);

    // Start imu task
    xTaskCreatePinnedToCore(&mpu6050, "IMU", 1024 * 8, NULL, 9, NULL, 1);

    xTaskCreatePinnedToCore(&status_task, "STAT", 1024 * 5, NULL, 4, NULL, 1);

    xTaskCreatePinnedToCore(&CNN_task, "CNN", 1024 * 20, NULL, 10, NULL, 1);

    xTaskCreatePinnedToCore(&scan_button_task, "BUTTON", 1024 * 5, NULL, 5, NULL, 1);

    esp_log_set_vprintf(ws_vprintf);

    // xTaskCreate(&play_mp3_task, "MP3", 1024 * 10, NULL, 5, NULL);

    // xTaskCreate(&start_test, "tesst", 1024 * 20, NULL, 5, NULL);
}
