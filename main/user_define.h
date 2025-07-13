#ifndef __USER_DEFINE_H__
#define __USER_DEFINE_H__

#define SW_VERSION "v0.0.1"
#define USER_CONFIG_NVS_NAMESPACE "user_config"
#define USER_CONFIG_NVS_KEY "config_data"
#define MODEL_DATASET_ID -1

#define WIFI_SSID_MAX_LEN 32
#define WIFI_PASS_MAX_LEN 64

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)
typedef struct
{
    gpio_num_t up_key_gpio_num;
    gpio_num_t down_key_gpio_num;

    gpio_num_t mpu_sda_gpio_num;
    gpio_num_t mpu_scl_gpio_num;

    gpio_num_t ws2812_gpio_num;

    char username[32];
    char password[32];
    char mdns_host_name[32];
    char wifi_ap_ssid[WIFI_SSID_MAX_LEN];
    char wifi_ap_pass[WIFI_PASS_MAX_LEN];
    char wifi_ssid[WIFI_SSID_MAX_LEN];
    char wifi_pass[WIFI_PASS_MAX_LEN];
    int wifi_scan_list_size;
    int wifi_connect_max_retry;
    int ws_recv_buf_size;
    int ws_send_buf_size;
    int msg_buf_recv_size;
    int msg_buf_send_size;

    uint16_t button_period_ms;

    int mpu_one_shot_max_sample_size;
    int mpu_buf_out_to_cnn_size;

    int tflite_arena_size;
    int tflite_model_size;

    int esplog_max_length;

    gpio_num_t periph_pwr_gpio_num;
    gpio_num_t i2s_bck_gpio_num;
    gpio_num_t i2s_ws_gpio_num;
    gpio_num_t i2s_dout_gpio_num;
    gpio_num_t ir_rx_gpio_num;
    gpio_num_t ir_tx_gpio_num;

} user_config_t;

#ifdef __cplusplus
}
#endif

#endif