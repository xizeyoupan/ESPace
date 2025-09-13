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
void malloc_all_buffer();

cJSON* toCjsonObj(const void* obj, const StructInfo* info);
void fromCjsonObj(void* obj, const StructInfo* info, const cJSON* json);

#ifdef __cplusplus
}
#endif

#endif