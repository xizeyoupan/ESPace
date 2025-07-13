#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"
#include "esp_bt.h"
#include "esp_bt_device.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_log.h"
#include "esp_peripherals.h"

#include "a2dp_stream.h"
#include "audio_element.h"
#include "audio_event_iface.h"
#include "audio_pipeline.h"
#include "board.h"
#include "filter_resample.h"
#include "i2s_stream.h"
#include "http_stream.h"
#include "nvs_flash.h"
#include "periph_touch.h"
#include "mp3_decoder.h"

static const char* TAG = "BLUETOOTH_EXAMPLE";
static esp_periph_handle_t bt_periph = NULL;

void play_mp3_task(void* pvParameters)
{
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t bt_stream_reader, i2s_stream_writer;

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set(TAG, ESP_LOG_DEBUG);

    ESP_LOGI(TAG, "[ 1 ] Init Bluetooth");
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT));
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    esp_bt_dev_set_device_name("ESP_SINK_STREAM_DEMO");

    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);

    ESP_LOGI(TAG, "[ 2 ] Start codec chip");
    audio_board_handle_t board_handle = audio_board_init();
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);

    ESP_LOGI(TAG, "[ 3 ] Create audio pipeline for playback");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);

    ESP_LOGI(TAG, "[4] Create i2s stream to write data to codec chip");
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.type = AUDIO_STREAM_WRITER;
    ESP_LOGI(TAG, "[4.0.1] Create i2s stream to write data to codec chip");

    i2s_stream_writer = i2s_stream_init(&i2s_cfg);
    ESP_LOGI(TAG, "[4.0.2] Create i2s stream to write data to codec chip");

    ESP_LOGI(TAG, "[4.1] Get Bluetooth stream");
    a2dp_stream_config_t a2dp_config = {
        .type = AUDIO_STREAM_READER,
        .user_callback = { 0 },
        .audio_hal = board_handle->audio_hal,
    };
    bt_stream_reader = a2dp_stream_init(&a2dp_config);

    ESP_LOGI(TAG, "[4.2] Register all elements to audio pipeline");
    audio_pipeline_register(pipeline, bt_stream_reader, "bt");
    audio_pipeline_register(pipeline, i2s_stream_writer, "i2s");

    ESP_LOGI(TAG, "[4.3] Link it together [Bluetooth]-->bt_stream_reader-->i2s_stream_writer-->[codec_chip]");

    const char* link_tag[2] = { "bt", "i2s" };
    audio_pipeline_link(pipeline, &link_tag[0], 2);

    ESP_LOGI(TAG, "[ 5 ] Initialize peripherals");
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);
    audio_board_key_init(set);

    ESP_LOGI(TAG, "[5.2] Create Bluetooth peripheral");
    bt_periph = bt_create_periph();

    ESP_LOGI(TAG, "[5.3] Start all peripherals");
    esp_periph_start(set, bt_periph);

    ESP_LOGI(TAG, "[ 6 ] Set up  event listener");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

    ESP_LOGI(TAG, "[6.1] Listening event from all elements of pipeline");
    audio_pipeline_set_listener(pipeline, evt);

    ESP_LOGI(TAG, "[ 7 ] Start audio_pipeline");
    audio_pipeline_run(pipeline);

    ESP_LOGI(TAG, "[ 8 ] Listen for all pipeline events");

    uint8_t volume;
    audio_hal_get_volume(board_handle->audio_hal, &volume);
    ESP_LOGI(TAG, "volume: %u", volume);

    audio_hal_set_volume(board_handle->audio_hal, 10);
    while (1) {
        audio_event_iface_msg_t msg;
        esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "[ * ] Event interface error : %d", ret);
            continue;
        }

        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void*)bt_stream_reader
            && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
            audio_element_info_t music_info = { 0 };
            audio_element_getinfo(bt_stream_reader, &music_info);

            ESP_LOGI(TAG, "[ * ] Receive music info from Bluetooth, sample_rates=%d, bits=%d, ch=%d",
                music_info.sample_rates, music_info.bits, music_info.channels);

            audio_element_set_music_info(i2s_stream_writer, music_info.sample_rates, music_info.channels, music_info.bits);
            i2s_stream_set_clk(i2s_stream_writer, music_info.sample_rates, music_info.bits, music_info.channels);
            continue;
        }

        /* Stop when the last pipeline element (i2s_stream_writer in this case) receives stop event */
        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void*)i2s_stream_writer
            && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
            && (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED))) {
            ESP_LOGW(TAG, "[ * ] Stop event received");
            break;
        }
    }

    ESP_LOGI(TAG, "[ 9 ] Stop audio_pipeline");
    audio_pipeline_stop(pipeline);
    audio_pipeline_wait_for_stop(pipeline);
    audio_pipeline_terminate(pipeline);

    /* Terminate the pipeline before removing the listener */
    audio_pipeline_remove_listener(pipeline);

    /* Stop all periph before removing the listener */
    esp_periph_set_stop_all(set);
    audio_event_iface_remove_listener(esp_periph_set_get_event_iface(set), evt);

    /* Make sure audio_pipeline_remove_listener & audio_event_iface_remove_listener are called before destroying event_iface */
    audio_event_iface_destroy(evt);

    /* Release all resources */
    audio_pipeline_unregister(pipeline, bt_stream_reader);
    audio_pipeline_unregister(pipeline, i2s_stream_writer);
    audio_pipeline_deinit(pipeline);
    audio_element_deinit(bt_stream_reader);
    audio_element_deinit(i2s_stream_writer);
    esp_periph_set_destroy(set);
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();
    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);

    // audio_pipeline_handle_t pipeline;
    // audio_element_handle_t http_stream_reader, i2s_stream_writer, mp3_decoder;

    // esp_log_level_set("*", ESP_LOG_WARN);
    // esp_log_level_set(TAG, ESP_LOG_DEBUG);

    // ESP_LOGI(TAG, "[ 1 ] Start audio codec chip");
    // audio_board_handle_t board_handle = audio_board_init();
    // audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);

    // ESP_LOGI(TAG, "[2.0] Create audio pipeline for playback");
    // audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    // pipeline = audio_pipeline_init(&pipeline_cfg);
    // mem_assert(pipeline);

    // ESP_LOGI(TAG, "[2.1] Create http stream to read data");
    // http_stream_cfg_t http_cfg = HTTP_STREAM_CFG_DEFAULT();
    // http_stream_reader = http_stream_init(&http_cfg);

    // ESP_LOGI(TAG, "[2.2] Create i2s stream to write data to codec chip");
    // i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    // i2s_cfg.type = AUDIO_STREAM_WRITER;
    // i2s_stream_writer = i2s_stream_init(&i2s_cfg);

    // ESP_LOGI(TAG, "[2.3] Create mp3 decoder to decode mp3 file");
    // mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
    // mp3_decoder = mp3_decoder_init(&mp3_cfg);

    // ESP_LOGI(TAG, "[2.4] Register all elements to audio pipeline");
    // audio_pipeline_register(pipeline, http_stream_reader, "http");
    // audio_pipeline_register(pipeline, mp3_decoder, "mp3");
    // audio_pipeline_register(pipeline, i2s_stream_writer, "i2s");

    // ESP_LOGI(TAG, "[2.5] Link it together http_stream-->mp3_decoder-->i2s_stream-->[codec_chip]");
    // const char* link_tag[3] = { "http", "mp3", "i2s" };
    // audio_pipeline_link(pipeline, &link_tag[0], 3);

    // ESP_LOGI(TAG, "[2.6] Set up  uri (http as http_stream, mp3 as mp3 decoder, and default output is i2s)");
    // audio_element_set_uri(http_stream_reader, "http://music.163.com/song/media/outer/url?id=2628344945");

    // ESP_LOGI(TAG, "[ 3 ] Start and wait for Wi-Fi network");
    // esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    // esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);
    // vTaskDelay(3000);

    // // Example of using an audio event -- START
    // ESP_LOGI(TAG, "[ 4 ] Set up  event listener");
    // audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    // audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

    // ESP_LOGI(TAG, "[4.1] Listening event from all elements of pipeline");
    // audio_pipeline_set_listener(pipeline, evt);

    // ESP_LOGI(TAG, "[4.2] Listening event from peripherals");
    // audio_event_iface_set_listener(esp_periph_set_get_event_iface(set), evt);

    // ESP_LOGI(TAG, "[ 5 ] Start audio_pipeline");
    // audio_pipeline_run(pipeline);

    // while (1) {
    //     audio_event_iface_msg_t msg;
    //     esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
    //     if (ret != ESP_OK) {
    //         ESP_LOGE(TAG, "[ * ] Event interface error : %d", ret);
    //         continue;
    //     }

    //     if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
    //         && msg.source == (void*)mp3_decoder
    //         && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
    //         audio_element_info_t music_info = { 0 };
    //         audio_element_getinfo(mp3_decoder, &music_info);

    //         ESP_LOGI(TAG, "[ * ] Receive music info from mp3 decoder, sample_rates=%d, bits=%d, ch=%d",
    //             music_info.sample_rates, music_info.bits, music_info.channels);

    //         i2s_stream_set_clk(i2s_stream_writer, music_info.sample_rates, music_info.bits, music_info.channels);
    //         continue;
    //     }

    //     /* Stop when the last pipeline element (i2s_stream_writer in this case) receives stop event */
    //     if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void*)i2s_stream_writer
    //         && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
    //         && (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED))) {
    //         ESP_LOGW(TAG, "[ * ] Stop event received");
    //         break;
    //     }
    // }
    // // Example of using an audio event -- END

    // ESP_LOGI(TAG, "[ 6 ] Stop audio_pipeline");
    // audio_pipeline_stop(pipeline);
    // audio_pipeline_wait_for_stop(pipeline);
    // audio_pipeline_terminate(pipeline);

    // /* Terminate the pipeline before removing the listener */
    // audio_pipeline_unregister(pipeline, http_stream_reader);
    // audio_pipeline_unregister(pipeline, i2s_stream_writer);
    // audio_pipeline_unregister(pipeline, mp3_decoder);

    // audio_pipeline_remove_listener(pipeline);

    // /* Stop all peripherals before removing the listener */
    // esp_periph_set_stop_all(set);
    // audio_event_iface_remove_listener(esp_periph_set_get_event_iface(set), evt);

    // /* Make sure audio_pipeline_remove_listener & audio_event_iface_remove_listener are called before destroying event_iface */
    // audio_event_iface_destroy(evt);

    // /* Release all resources */
    // audio_pipeline_deinit(pipeline);
    // audio_element_deinit(http_stream_reader);
    // audio_element_deinit(i2s_stream_writer);
    // audio_element_deinit(mp3_decoder);
    // esp_periph_set_destroy(set);
}