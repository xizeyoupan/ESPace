#ifndef __USER_UTIL_H__
#define __USER_UTIL_H__

#include "esp_err.h"
#include "stdint.h"

#include "cJSON.h"

#include "espace_define.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t save_wifi_config(char* wifi_ssid, char* wifi_pass);
esp_err_t load_wifi_config(char* wifi_ssid, char* wifi_pass);
void load_user_config();
void save_user_config();
void reset_user_config();
int get_model_type_by_name(char* name);
size_t get_sample_tick_by_name(char* name);
uint32_t get_color_by_name(char* name);
void enable_periph_pwr();
void disable_periph_pwr();
void malloc_all_buffer();

// 设备状态管理函数
device_state_t get_device_state(void);
void set_device_state(device_state_t new_state);

cJSON* toCjsonObj(const void* obj, const StructInfo* info);
void fromCjsonObj(void* obj, const StructInfo* info, const cJSON* json);
int my_vprintf(const char* _Format, va_list _ArgList);

#ifdef __cplusplus
}
#endif

#endif