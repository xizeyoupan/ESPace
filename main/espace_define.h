#ifndef __ESPACE_DEFINE_H__
#define __ESPACE_DEFINE_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

#include "esp_err.h"
#include "stdint.h"

#define SW_VERSION "v0.0.1"
#define USER_CONFIG_NVS_NAMESPACE "user_config"
#define USER_CONFIG_NVS_KEY "config_data"
#define MODEL_DATASET_ID -1

#define WIFI_SSID_MAX_LEN 32
#define WIFI_PASS_MAX_LEN 64

#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    INT,
    STRING,
    FLOAT,
    DOUBLE,
    FIXED_STRING
} FieldKind;

typedef struct {
    const char* type_name;
    const char* field_name;
    size_t offset;
    FieldKind kind;
    size_t array_size; // 对 FIXED_STRING 有效
} FieldInfo;

typedef struct {
    const char* struct_name;
    size_t size;
    const FieldInfo* fields;
    size_t field_count;
} StructInfo;

/* ---------- 字段定义与反射信息生成 ---------- */

// 普通字段：不是数组
#define DECL_FIELD(S, type, name, kind, size, is_array) \
    type name;

#define MAKE_FIELD_INFO(S, type, name, kind, size, is_array) \
    { #type, #name, offsetof(S, name), kind, size },

// 数组字段：专门处理 (用宏包裹数组长度)
#define DECL_FIELD_ARRAY(S, type, name, kind, size, is_array) \
    type name[size];

#define MAKE_FIELD_INFO_ARRAY(S, type, name, kind, size, is_array) \
    { #type, #name, offsetof(S, name), kind, size },

/* ---------- REFLECT_STRUCT 宏 ---------- */

#define REFLECT_STRUCT(name, FIELDS)                               \
    _Pragma("pack(push, 1)") typedef struct {                      \
        FIELDS(DECL_FIELD, DECL_FIELD_ARRAY, name)                 \
    } name;                                                        \
    _Pragma("pack(pop)")                                           \
                                                                   \
        static FieldInfo name##_fields[]                           \
        = {                                                        \
              FIELDS(MAKE_FIELD_INFO, MAKE_FIELD_INFO_ARRAY, name) \
          };                                                       \
                                                                   \
    static StructInfo name##_info = {                              \
        #name,                                                     \
        sizeof(name),                                              \
        name##_fields,                                             \
        sizeof(name##_fields) / sizeof(name##_fields[0])           \
    };

#define USER_CONFIG_FIELDS(X, XA, S)                              \
    X(S, int, up_key_gpio_num, INT, 0, 0)                         \
    X(S, int, down_key_gpio_num, INT, 0, 0)                       \
    X(S, int, mpu_sda_gpio_num, INT, 0, 0)                        \
    X(S, int, mpu_scl_gpio_num, INT, 0, 0)                        \
    X(S, int, ws2812_gpio_num, INT, 0, 0)                         \
                                                                  \
    XA(S, char, username, FIXED_STRING, 32, 1)                    \
    XA(S, char, password, FIXED_STRING, 32, 1)                    \
    XA(S, char, mdns_host_name, FIXED_STRING, 32, 1)              \
    XA(S, char, wifi_ap_ssid, FIXED_STRING, WIFI_SSID_MAX_LEN, 1) \
    XA(S, char, wifi_ap_pass, FIXED_STRING, WIFI_PASS_MAX_LEN, 1) \
    XA(S, char, wifi_ssid, FIXED_STRING, WIFI_SSID_MAX_LEN, 1)    \
    XA(S, char, wifi_pass, FIXED_STRING, WIFI_PASS_MAX_LEN, 1)    \
                                                                  \
    X(S, int, wifi_scan_list_size, INT, 0, 0)                     \
    X(S, int, wifi_connect_max_retry, INT, 0, 0)                  \
    X(S, int, ws_recv_buf_size, INT, 0, 0)                        \
    X(S, int, ws_send_buf_size, INT, 0, 0)                        \
    X(S, int, msg_buf_recv_size, INT, 0, 0)                       \
    X(S, int, msg_buf_send_size, INT, 0, 0)                       \
                                                                  \
    X(S, int, button_period_ms, INT, 0, 0)                        \
                                                                  \
    X(S, int, mpu_one_shot_max_sample_size, INT, 0, 0)            \
    X(S, int, mpu_buf_out_to_cnn_size, INT, 0, 0)                 \
                                                                  \
    X(S, int, tflite_arena_size, INT, 0, 0)                       \
    X(S, int, tflite_model_size, INT, 0, 0)                       \
                                                                  \
    X(S, int, esplog_max_length, INT, 0, 0)                       \
                                                                  \
    X(S, int, periph_pwr_gpio_num, INT, 0, 0)                     \
    X(S, int, i2s_bck_gpio_num, INT, 0, 0)                        \
    X(S, int, i2s_ws_gpio_num, INT, 0, 0)                         \
    X(S, int, i2s_dout_gpio_num, INT, 0, 0)                       \
    X(S, int, ir_rx_gpio_num, INT, 0, 0)                          \
    X(S, int, ir_tx_gpio_num, INT, 0, 0)

REFLECT_STRUCT(user_config_t, USER_CONFIG_FIELDS)

#define USER_DEF_ERR_MSG_LEN 128
typedef struct
{
    esp_err_t esp_err;
    char err_msg[USER_DEF_ERR_MSG_LEN];
} user_def_err_t;

#ifdef __cplusplus
}
#endif

#pragma GCC diagnostic pop

#endif