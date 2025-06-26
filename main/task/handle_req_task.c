#include "handle_req_task.h"

static const char* TAG = "HANDLE_REQ_TASK";

extern MessageBufferHandle_t xMessageBufferReqSend;
extern user_config_t user_config;
extern mpu_command_t received_command;
extern EventGroupHandle_t x_mpu_event_group;
MessageBufferHandle_t xMessageBufferReqRecv;

#define FILE_LIST_STR_LEN 4096

char file_list_str[FILE_LIST_STR_LEN];

void handle_req_task(void* pvParameters)
{
    BaseType_t core_id = xPortGetCoreID(); // 返回当前任务所在的核心 ID
    ESP_LOGI(TAG, "Task is running on core %d.", core_id);

    xMessageBufferReqRecv = xMessageBufferCreate(user_config.msg_buf_recv_size);
    configASSERT(xMessageBufferReqRecv);

    uint8_t* buffer = malloc(user_config.ws_recv_buf_size);

    cJSON* monitor_json = NULL;

    while (1) {
        size_t msg_size = xMessageBufferReceive(xMessageBufferReqRecv, buffer, user_config.ws_recv_buf_size, portMAX_DELAY); // 读取完整消息
        buffer[msg_size] = 0;
        ESP_LOGI(TAG, "Receive ws size: %d, data: %s", msg_size, buffer);
        // 处理 WebSocket 数据帧

        char* resp_string = NULL;
        cJSON* resp_json = NULL;
        cJSON* resp_payload = NULL;
        const cJSON* type = NULL;
        const cJSON* requestId = NULL;
        const cJSON* payload = NULL;

        monitor_json = cJSON_Parse((char*)buffer);
        if (monitor_json == NULL) {
            goto json_parse_end;
        }

        type = cJSON_GetObjectItem(monitor_json, "type");
        if (type == NULL || !cJSON_IsString(type) || (type->valuestring == NULL)) {
            goto json_parse_end;
        }

        requestId = cJSON_GetObjectItem(monitor_json, "requestId");
        if (requestId == NULL || !cJSON_IsString(requestId) || (requestId->valuestring == NULL)) {
            goto json_parse_end;
        }

        payload = cJSON_GetObjectItem(monitor_json, "payload");
        if (payload == NULL || !cJSON_IsObject(payload)) {
            goto json_parse_end;
        }

        resp_json = cJSON_CreateObject();
        if (resp_json == NULL) {
            goto json_parse_end;
        }

        resp_payload = cJSON_CreateObject();
        if (resp_payload == NULL) {
            goto json_parse_end;
        }

        char response_type[64];
        snprintf(response_type, sizeof(response_type), "%s_response", type->valuestring);
        cJSON_AddStringToObject(resp_json, "type", response_type);
        cJSON_AddStringToObject(resp_json, "requestId", requestId->valuestring);
        cJSON_AddItemToObject(resp_json, "payload", resp_payload);
        cJSON_AddNullToObject(resp_json, "error");

        if (strcmp(type->valuestring, "get_device_info") == 0) {
            cJSON* device_info = get_device_info();
            if (device_info == NULL) {
                goto json_parse_end;
            }
            cJSON_AddItemToObject(resp_payload, "data", device_info);
        } else if (strcmp(type->valuestring, "get_user_config") == 0) {
            cJSON* user_config_json = get_user_config_json();
            if (user_config_json == NULL) {
                goto json_parse_end;
            }
            cJSON_AddItemToObject(resp_payload, "data", user_config_json);
        } else if (strcmp(type->valuestring, "get_wifi_info") == 0) {
            cJSON* wifi_info = get_wifi_info();
            if (wifi_info == NULL) {
                goto json_parse_end;
            }
            cJSON_AddItemToObject(resp_payload, "data", wifi_info);
        } else if (strcmp(type->valuestring, "get_wifi_list") == 0) {
            cJSON* wifi_list = get_wifi_list();
            if (wifi_list == NULL) {
                goto json_parse_end;
            }
            cJSON_AddItemToObject(resp_payload, "data", wifi_list);
        } else if (strcmp(type->valuestring, "get_state_info") == 0) {
            cJSON* task_state = get_task_state();
            if (task_state == NULL) {
                goto json_parse_end;
            }
            cJSON_AddItemToObject(resp_payload, "data", task_state);

        } else if (strcmp(type->valuestring, "connect_wifi") == 0) {
            const cJSON* data = cJSON_GetObjectItem(payload, "data");
            const cJSON* ssid = cJSON_GetObjectItem(data, "ssid");
            const cJSON* password = cJSON_GetObjectItem(data, "password");
            strcpy(user_config.wifi_ssid, ssid->valuestring);
            strcpy(user_config.wifi_pass, password->valuestring);

            save_user_config();
            start_sta_mode();

        } else if (strcmp(type->valuestring, "update_user_config") == 0) {
            const cJSON* data = cJSON_GetObjectItem(payload, "data");
            assign_user_config_from_json(data);
            save_user_config();
            cJSON* user_config_json = get_user_config_json();
            cJSON_AddItemToObject(resp_payload, "data", user_config_json);
        } else if (strcmp(type->valuestring, "reset_user_config") == 0) {
            reset_user_config();
            save_user_config();
            cJSON* user_config_json = get_user_config_json();
            cJSON_AddItemToObject(resp_payload, "data", user_config_json);
        } else if (strcmp(type->valuestring, "reboot") == 0) {
            esp_restart();
        } else if (strcmp(type->valuestring, "get_imu_data") == 0) {
            double roll, pitch, ax, ay, az, gx, gy, gz;
            get_angle_and_m6(&roll, &pitch, &ax, &ay, &az, &gx, &gy, &gz);
            cJSON* imu_data = cJSON_CreateObject();
            cJSON_AddNumberToObject(imu_data, "roll", roll);
            cJSON_AddNumberToObject(imu_data, "pitch", pitch);
            cJSON_AddNumberToObject(imu_data, "ax", ax);
            cJSON_AddNumberToObject(imu_data, "ay", ay);
            cJSON_AddNumberToObject(imu_data, "az", az);
            cJSON_AddNumberToObject(imu_data, "gx", gx);
            cJSON_AddNumberToObject(imu_data, "gy", gy);
            cJSON_AddNumberToObject(imu_data, "gz", gz);
            cJSON_AddItemToObject(resp_payload, "data", imu_data);
        } else if (strcmp(type->valuestring, "reset_imu") == 0) {
            reset_imu();
        } else if (strcmp(type->valuestring, "get_mpu_data_row") == 0) {
            const cJSON* data = cJSON_GetObjectItem(payload, "data");
            const cJSON* sample_size = cJSON_GetObjectItem(data, "sample_size");
            const cJSON* sample_tick = cJSON_GetObjectItem(data, "sample_tick");
            const cJSON* model_type = cJSON_GetObjectItem(data, "type");

            memset(&received_command, 0, sizeof(received_command));
            received_command.type = MPU_COMMAND_TYPE_GET_ROW;
            received_command.model_type = model_type->valuedouble;
            received_command.sample_size = sample_size->valuedouble;
            received_command.sample_tick = sample_tick->valuedouble;
            received_command.need_predict = 0;

            xEventGroupSetBits(x_mpu_event_group, MPU_SAMPLING_READY_BIT);
        } else if (strcmp(type->valuestring, "get_mpu_data_row_stop") == 0) {
            xEventGroupSetBits(x_mpu_event_group, MPU_SAMPLING_UNREADY_BIT);
        } else if (strcmp(type->valuestring, "get_file_list") == 0) {
            memset(file_list_str, 0, FILE_LIST_STR_LEN);
            list_littlefs_files(file_list_str);
            cJSON_AddStringToObject(resp_payload, "data", file_list_str);
        } else if (strcmp(type->valuestring, "modify_model") == 0) {
            const cJSON* data = cJSON_GetObjectItem(payload, "data");
            const cJSON* new_val = cJSON_GetObjectItem(data, "new");
            const cJSON* old_val = cJSON_GetObjectItem(data, "old");
            const cJSON* del_val = cJSON_GetObjectItem(data, "del");

            if (new_val && cJSON_IsString(new_val) && new_val->valuestring && strlen(new_val->valuestring)) {
                rename_file(old_val->valuestring, new_val->valuestring);
            } else if (del_val && cJSON_IsBool(del_val) && cJSON_IsTrue(del_val)) {
                unlink_file(old_val->valuestring);
            }
        } else if (strcmp(type->valuestring, "start_predict") == 0) {
            const cJSON* data = cJSON_GetObjectItem(payload, "data");
            const cJSON* model_name = cJSON_GetObjectItem(data, "model");

            
        }

        resp_string = cJSON_Print(resp_json);
        if (resp_string == NULL) {
            goto json_parse_end;
        }

    json_parse_end:
        if (resp_string != NULL) {
            ESP_LOGI(TAG, "Sending ws size: %d, data: %s", strlen(resp_string), resp_string);
            xMessageBufferSend(xMessageBufferReqSend, resp_string, strlen(resp_string), portMAX_DELAY);
        }

        cJSON_Delete(resp_json);
        free(resp_string);
        cJSON_Delete(monitor_json);
    }

    free(buffer);
    vTaskDelete(NULL);
}
