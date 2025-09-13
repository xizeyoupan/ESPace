#include "espace_define.h"

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/task.h"

#include "nvs_flash.h"
#include "user_util.h"

#include "service/io_lock_service.h"
#include "service/json_helper.h"
#include "service/littlefs_service.h"
#include "service/tflite_service.h"

#include "task/button.h"
#include "task/handle_req_task.h"
#include "task/ir_task.h"
#include "task/mpu_task.h"
#include "task/play_mp3.h"
#include "task/tflite_task.h"
#include "task/wand_server_task.h"
#include "task/ws2812_task.h"
#include "task/ws_task.h"

#include <cstdarg>
#include <cstdint>
#include <cstring>

user_config_t user_config;

static const char* TAG = "MAIN";

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

    cjson_hook_init(NULL);
    littlefs_init();
    list_littlefs_files(NULL);
    load_user_config();
    io_mutex_init();
    malloc_all_buffer();
    tflite_init();

    enable_periph_pwr();
    vTaskDelay(pdMS_TO_TICKS(100));

    xTaskCreatePinnedToCore(&ws2812_task, "ws2812_task", 1024 * 3, NULL, 5, NULL, 0);

    xTaskCreatePinnedToCore(&wand_server_task, "wand_server", 1024 * 5, NULL, 15, NULL, 0);

    xTaskCreatePinnedToCore(&websocket_send_task, "websocket_send", 1024 * 5, NULL, 13, NULL, 1);

    xTaskCreatePinnedToCore(&handle_req_task, "handle_req", 1024 * 5, NULL, 12, NULL, 1);

    xTaskCreatePinnedToCore(&mpu_task, "mpu_task", 1024 * 5, NULL, 10, NULL, 1);

    xTaskCreatePinnedToCore(&tflite_task, "tflite_task", 1024 * 5, NULL, 9, NULL, 1);

    xTaskCreatePinnedToCore(&scan_button_task, "button_task", 1024 * 3, NULL, 5, NULL, 1);

    // xTaskCreatePinnedToCore(&play_mp3_task, "MP3", 1024 * 10, NULL, 5, NULL, 1);

    // xTaskCreatePinnedToCore(&ir_task, "ir_task", 1024 * 3, NULL, 5, NULL, 1);

    esp_log_set_vprintf(my_vprintf);
}
