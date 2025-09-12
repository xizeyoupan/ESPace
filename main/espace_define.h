#ifndef __ESPACE_DEFINE_H__
#define __ESPACE_DEFINE_H__

#include "esp_err.h"

#define SW_VERSION "v0.0.1"
#define USER_CONFIG_NVS_NAMESPACE "user_config"
#define USER_CONFIG_NVS_KEY "config_data"
#define MODEL_DATASET_ID -1

#define WIFI_SSID_MAX_LEN 32
#define WIFI_PASS_MAX_LEN 64

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)
typedef struct
{
    int up_key_gpio_num;
    int down_key_gpio_num;

    int mpu_sda_gpio_num;
    int mpu_scl_gpio_num;

    int ws2812_gpio_num;

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

    int button_period_ms;

    int mpu_one_shot_max_sample_size;
    int mpu_buf_out_to_cnn_size;

    int tflite_arena_size;
    int tflite_model_size;

    int esplog_max_length;

    int periph_pwr_gpio_num;
    int i2s_bck_gpio_num;
    int i2s_ws_gpio_num;
    int i2s_dout_gpio_num;
    int ir_rx_gpio_num;
    int ir_tx_gpio_num;

} user_config_t;
#pragma pack(pop)

typedef struct
{
    esp_err_t esp_err;
    char err_msg[128];
} user_def_err_t;

#ifdef __cplusplus
}
#endif

#endif