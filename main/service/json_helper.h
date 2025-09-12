#ifndef __JSON_HELPER_H__
#define __JSON_HELPER_H__

#include "cJSON.h"
#include "espace_define.h"

#ifdef __cplusplus
extern "C" {
#endif

cJSON* get_device_info(void);
cJSON* get_user_config_json(void);
cJSON* get_task_state(void);
void assign_user_config_from_json(const cJSON* data);
cJSON* get_ledc_timer_config_json(int index);
cJSON* get_ledc_channel_config_json(int index);
cJSON* get_imu_data_json();
cJSON* get_dac_cosine_config_json(int index);
void load_dac_cont_data_from_json(const cJSON* data);
void cjson_hook_init(user_def_err_t* user_err);

#ifdef __cplusplus
}
#endif

#endif