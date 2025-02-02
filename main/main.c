#include "user_config.h"
extern QueueHandle_t xWS2812Queue;
user_config_t user_config;

static const char *TAG = "MAIN";

void app_main(void)
{

    // Initialize NVS
    ESP_LOGI(TAG, "Initialize NVS");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    load_user_config();

    // Start ws2812
    xTaskCreate(&WS2812_ControllerTask, "WS2812_ControllerTask", 1024 * 5, NULL, 5, NULL);
    vTaskDelay(pdMS_TO_TICKS(10));
    uint32_t color = COLOR_YELLOW;
    xQueueSend(xWS2812Queue, &color, portMAX_DELAY);

    // Start web server
    xTaskCreate(&wand_server_task, "SERVER", 1024 * 5, NULL, 5, NULL);

    // Start ws
    xTaskCreate(&websocket_send_task, "WEBSOCKET", 1024 * 5, NULL, 5, NULL);

    // Start imu task
    xTaskCreate(&mpu6050, "IMU", 1024 * 8, NULL, 5, NULL);

    // xTaskCreate(&play_mp3_task, "MP3", 1024 * 10, NULL, 5, NULL);

    // xTaskCreate(&start_test, "tesst", 1024 * 20, NULL, 5, NULL);
}
