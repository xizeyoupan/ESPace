#include "CNN.h"
#include "user_config.h"
user_config_t user_config;

static const char* TAG = "MAIN";

extern MessageBufferHandle_t xMessageBufferReqSend;
extern MessageBufferHandle_t xMessageBufferReqRecv;

int my_vprintf(const char* _Format, va_list _ArgList)
{
    va_list args_copy;
    va_copy(args_copy, _ArgList);
    int len = vsnprintf(NULL, 0, _Format, args_copy);
    va_end(args_copy);

    if (len > 99999) {
        return printf("str len = %d, ignored.\n", len);
    } else {
        return vprintf(_Format, _ArgList);
    }

    // return 0;
}

extern "C" void app_main(void)
{
    // Initialize NVS
    ESP_LOGI(TAG, "Initialize NVS");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    littlefs_init();
    list_littlefs_files(NULL);
    load_user_config();
    tflite_init();

    xTaskCreatePinnedToCore(&ws2812_task, "ws2812_task", 1024 * 3, NULL, 5, NULL, 0);

    // Start wifi and web server
    xTaskCreatePinnedToCore(&wand_server_task, "wand_server", 1024 * 5, NULL, 15, NULL, 0);

    // Start ws
    xTaskCreatePinnedToCore(&websocket_send_task, "websocket_send", 1024 * 5, NULL, 13, NULL, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    configASSERT(xMessageBufferReqSend);

    xTaskCreatePinnedToCore(&handle_req_task, "handle_req", 1024 * 5, NULL, 12, NULL, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    configASSERT(xMessageBufferReqRecv);

    xTaskCreatePinnedToCore(&mpu_task, "mpu_task", 1024 * 5, NULL, 10, NULL, 1);

    // xTaskCreatePinnedToCore(&CNN_task, "CNN", 1024 * 20, NULL, 10, NULL, 1);

    xTaskCreatePinnedToCore(&scan_button_task, "button_task", 1024 * 3, NULL, 5, NULL, 1);

    // xTaskCreate(&play_mp3_task, "MP3", 1024 * 10, NULL, 5, NULL);

    // xTaskCreate(&start_test, "tesst", 1024 * 20, NULL, 5, NULL);

    esp_log_set_vprintf(my_vprintf);
}
