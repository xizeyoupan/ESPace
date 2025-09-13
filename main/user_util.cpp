#include "user_util.h"

#include "espace_define.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/message_buffer.h"

#include "ctype.h"
#include "stdint.h"
#include "string.h"

#include "nvs_flash.h"

#include "cJSON.h"

#include "service/json_helper.h"

#include "task/mpu_task.h"

static const char* TAG = "NVS_UTIL";

esp_err_t save_to_namespace(char* user_namespace, const char* key, void* value, size_t size)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(user_namespace, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        goto save_to_namespace_end;
    }
    err = nvs_set_blob(nvs_handle, key, value, size);
    if (err != ESP_OK) {
        goto save_to_namespace_end;
    }
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        goto save_to_namespace_end;
    }
save_to_namespace_end:
    nvs_close(nvs_handle);
    return err;
}

esp_err_t load_from_namespace(char* user_namespace, const char* key, void* out_data, size_t size)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(user_namespace, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        goto load_from_namespace_end;
    }
    err = nvs_get_blob(nvs_handle, key, out_data, &size);
    if (err != ESP_OK) {
        goto load_from_namespace_end;
    }

load_from_namespace_end:
    nvs_close(nvs_handle);
    return err;
}

extern user_config_t user_config;

void reset_user_config()
{
    ESP_LOGI(TAG, "Reset user config");

    memset(&user_config, 0, sizeof(user_config_t));

    user_config.up_key_gpio_num = 4;
    user_config.down_key_gpio_num = 0;

    user_config.mpu_sda_gpio_num = 22;
    user_config.mpu_scl_gpio_num = 21;

    user_config.ws2812_gpio_num = 23;

    user_config.wifi_scan_list_size = 50;
    user_config.wifi_connect_max_retry = 10;
    strcpy(user_config.username, "murasame");
    strcpy(user_config.password, "0d00");
    strcpy(user_config.wifi_ap_ssid, "ESPace-AP");
    strcpy(user_config.wifi_ap_pass, "07210721");
    strcpy(user_config.mdns_host_name, "espace");

    user_config.ws_recv_buf_size = 1024 * 100;
    user_config.msg_buf_recv_size = 1024 * 400;

    user_config.ws_send_buf_size = 1024 * 100;
    user_config.msg_buf_send_size = 1024 * 400;

    user_config.button_period_ms = 3000;

    user_config.mpu_one_shot_max_sample_size = 1000;
    user_config.mpu_buf_out_to_cnn_size = 1024 * 100;

    user_config.tflite_arena_size = 1024 * 100;
    user_config.tflite_model_size = 1200 * 6 * 4;

    user_config.esplog_max_length = 512;

    user_config.periph_pwr_gpio_num = 33;
    user_config.i2s_bck_gpio_num = 14;
    user_config.i2s_ws_gpio_num = 13;
    user_config.i2s_dout_gpio_num = 27;
    user_config.ir_rx_gpio_num = 19;
    user_config.ir_tx_gpio_num = 18;
}

void load_user_config()
{
    if (load_from_namespace(USER_CONFIG_NVS_NAMESPACE, USER_CONFIG_NVS_KEY, &user_config, sizeof(user_config)) == ESP_OK) {
        ESP_LOGI(TAG, "Loaded user config from NVS");
    } else {
        ESP_LOGW(TAG, "Failed to load user config from NVS, using default values");
        reset_user_config();
    }

    cJSON* config = get_user_config_json();
    if (config == NULL) {
        ESP_LOGE(TAG, "Failed to get user config JSON");
        return;
    }

    char* config_str = cJSON_Print(config);
    if (config_str == NULL) {
        ESP_LOGE(TAG, "Failed to print user config JSON");
        cJSON_Delete(config);
        return;
    }
    ESP_LOGI(TAG, "User config JSON: %s", config_str);
    cJSON_Delete(config);
    free(config_str);
}

void save_user_config()
{
    if (save_to_namespace(USER_CONFIG_NVS_NAMESPACE, USER_CONFIG_NVS_KEY, &user_config, sizeof(user_config)) == ESP_OK) {
        ESP_LOGI(TAG, "Saved user config to NVS");
    } else {
        ESP_LOGE(TAG, "Failed to save user config to NVS");
    }
}

int get_model_type_by_name(char* name)
{
    if (strstr(name, "oneshot") != NULL) {
        return MODEL_TYPE_ONESHOT;
    }
    return MODEL_TYPE_PERIODIC;
}

size_t get_sample_tick_by_name(char* name)
{
    char* token = strtok(name, ".");
    while (token != NULL) {
        if (isdigit((unsigned char)token[0])) {
            ESP_LOGI(TAG, "Found sample tick: %s", token);
            return (size_t)atoi(token);
        }
        token = strtok(NULL, ".");
    }
    return 0;
}

uint32_t get_color_by_name(char* name)
{
    // Make a copy to avoid modifying the original string
    char* name_copy = strdup(name);
    if (!name_copy)
        return 0;

    char* p = strrchr(name_copy, '.');
    if (!p) {
        free(name_copy);
        return 0;
    }
    *p = '\0';
    p = strrchr(name_copy, '.');
    if (!p) {
        free(name_copy);
        return 0;
    }
    uint32_t color = (uint32_t)strtoul(p + 1, NULL, 16);
    free(name_copy);
    return color;
}

void enable_periph_pwr()
{
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE; // 禁用中断
    io_conf.pin_bit_mask = (1ULL << user_config.periph_pwr_gpio_num); // 设置 GPIO
    io_conf.mode = GPIO_MODE_OUTPUT; // 设置为输入模式
    gpio_config(&io_conf);

    gpio_set_level(gpio_num_t(user_config.periph_pwr_gpio_num), 0);
}

MessageBufferHandle_t xMessageBufferReqSend;
MessageBufferHandle_t xMessageBufferReqRecv;
MessageBufferHandle_t xMessageBufferMPUOut2CNN;
void malloc_all_buffer()
{
    uint64_t start = esp_timer_get_time();

    UBaseType_t capType = MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT;
    xMessageBufferReqSend = xMessageBufferCreateWithCaps(user_config.msg_buf_send_size, capType);
    configASSERT(xMessageBufferReqSend);

    xMessageBufferReqRecv = xMessageBufferCreateWithCaps(user_config.msg_buf_recv_size, capType);
    configASSERT(xMessageBufferReqRecv);

    xMessageBufferMPUOut2CNN = xMessageBufferCreateWithCaps(user_config.mpu_buf_out_to_cnn_size, capType);
    configASSERT(xMessageBufferMPUOut2CNN);

    uint64_t end = esp_timer_get_time();
    ESP_LOGI(TAG, "Took %llu milliseconds to malloc all buffer", (end - start) / 1000);
}

/*
#define PERSON_FIELDS(X, XA, S)             \
    X (S, int,   age,    INT,  0, 0)        \
    XA(S, char,  name,   FIXED_STRING, 32, 1) \
    X (S, float, height, FLOAT, 0, 0)

REFLECT_STRUCT(Person, PERSON_FIELDS)

Person p;
cJSON* obj = toCjsonObj(&p, &Person_info);
*/
cJSON* toCjsonObj(const void* obj, const StructInfo* info)
{
    cJSON* root = cJSON_CreateObject();
    for (size_t i = 0; i < info->field_count; i++) {
        const FieldInfo* f = &info->fields[i];
        const char* base = (const char*)obj;
        const void* ptr = base + f->offset;

        switch (f->kind) {
        case INT:
            cJSON_AddNumberToObject(root, f->field_name, *(int*)ptr);
            break;
        case FLOAT:
            cJSON_AddNumberToObject(root, f->field_name, *(float*)ptr);
            break;
        case DOUBLE:
            cJSON_AddNumberToObject(root, f->field_name, *(double*)ptr);
            break;
        case STRING:
            cJSON_AddStringToObject(root, f->field_name, *(char**)ptr);
            break;
        case FIXED_STRING:
            cJSON_AddStringToObject(root, f->field_name, (const char*)ptr);
            break;
        }
    }
    return root;
}

/*
#define PERSON_FIELDS(X, XA, S)             \
    X (S, int,   age,    INT,  0, 0)        \
    XA(S, char,  name,   FIXED_STRING, 32, 1) \
    X (S, float, height, FLOAT, 0, 0)
REFLECT_STRUCT(Person, PERSON_FIELDS)

Person p;
const char* json_text = "{\"age\":30,\"name\":\"Bob\",\"height\":1.75}";
cJSON* json = cJSON_Parse(json_text);
Person p;
memset(&p, 0, sizeof(p));
fromCjsonObj(&p, &Person_info, json);
*/
void fromCjsonObj(void* obj, const StructInfo* info, const cJSON* json)
{
    for (size_t i = 0; i < info->field_count; i++) {
        const FieldInfo* f = &info->fields[i];
        const char* base = (const char*)obj;
        void* ptr = (void*)(base + f->offset);

        const cJSON* item = cJSON_GetObjectItemCaseSensitive(json, f->field_name);
        if (!item)
            continue;

        switch (f->kind) {
        case INT:
            if (cJSON_IsNumber(item))
                *(int*)ptr = item->valueint;
            break;
        case FLOAT:
            if (cJSON_IsNumber(item))
                *(float*)ptr = (float)item->valuedouble;
            break;
        case DOUBLE:
            if (cJSON_IsNumber(item))
                *(double*)ptr = item->valuedouble;
            break;
        case STRING:
            if (cJSON_IsString(item) && item->valuestring)
                *(char**)ptr = strdup(item->valuestring);
            break;
        case FIXED_STRING:
            if (cJSON_IsString(item) && item->valuestring)
                strncpy((char*)ptr, item->valuestring, f->array_size - 1);
            ((char*)ptr)[f->array_size - 1] = '\0';
            break;
        }
    }
}

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