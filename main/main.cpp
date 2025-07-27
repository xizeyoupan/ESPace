#include "user_config.h"
user_config_t user_config;

static const char* TAG = "MAIN";

int my_vprintf(const char* _Format, va_list _ArgList)
{
    va_list args_copy;
    va_copy(args_copy, _ArgList);
    int len = vsnprintf(NULL, 0, _Format, args_copy);
    va_end(args_copy);

    if (user_config.esplog_max_length == 0) {
        return 0;
    } else if (len > user_config.esplog_max_length) {
        return printf("log str len = %d, ignored.\n", len);
    } else {
        return vprintf(_Format, _ArgList);
    }
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

    xTaskCreatePinnedToCore(&play_mp3_task, "MP3", 1024 * 10, NULL, 5, NULL, 1);

    // xTaskCreatePinnedToCore(&ir_task, "ir_task", 1024 * 3, NULL, 5, NULL, 1);

    esp_log_set_vprintf(my_vprintf);

    // ledc_timer_config_t ledc_timer = {
    //     .speed_mode = LEDC_LOW_SPEED_MODE,
    //     .duty_resolution = LEDC_TIMER_1_BIT,
    //     .timer_num = LEDC_TIMER_0,
    //     .freq_hz = 13560000,
    //     .clk_cfg = LEDC_AUTO_CLK
    // };
    // ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // // Prepare and then apply the LEDC PWM channel configuration
    // ledc_channel_config_t ledc_channel = {
    //     .gpio_num = 5,
    //     .speed_mode = LEDC_LOW_SPEED_MODE,
    //     .channel = LEDC_CHANNEL_0,
    //     .intr_type = LEDC_INTR_DISABLE,
    //     .timer_sel = LEDC_TIMER_0,
    //     .duty = 0, // Set duty to 0%
    //     .hpoint = 0
    // };
    // ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    // ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 1));
    // // Update duty to apply the new value
    // ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));

    dac_cosine_handle_t chan0_handle;

    dac_cosine_config_t cos0_cfg = {
        .chan_id = DAC_CHAN_0,
        .freq_hz = 75000, // It will be covered by 8000 in the latter configuration
        .clk_src = DAC_COSINE_CLK_SRC_DEFAULT,
        .atten = DAC_COSINE_ATTEN_DEFAULT,
        .phase = DAC_COSINE_PHASE_0,
        .offset = 0,
    };

    ESP_ERROR_CHECK(dac_cosine_new_channel(&cos0_cfg, &chan0_handle));
    ESP_ERROR_CHECK(dac_cosine_start(chan0_handle));
}
