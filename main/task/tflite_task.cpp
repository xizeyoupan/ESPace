#include "tflite_task.h"

static const char* TAG = "TFLITE_TASK";
extern MessageBufferHandle_t xMessageBufferMPUOut2CNN, xMessageBufferReqSend;
extern mpu_command_t received_command;

float output_buf[16];

void tflite_task(void* pvParameters)
{
    BaseType_t core_id = xPortGetCoreID(); // 返回当前任务所在的核心 ID
    ESP_LOGI(TAG, "Task is running on core %d.", core_id);

    while (1) {
        if (!received_command.sample_size) {
            vTaskDelay(pdTICKS_TO_MS(10));
            continue;
        }

        size_t byte_size = received_command.sample_size * 24;
        float* buffer = (float*)malloc(byte_size);
        int output_size;
        size_t msg_size = xMessageBufferReceive(xMessageBufferMPUOut2CNN, buffer, byte_size, portMAX_DELAY);
        ESP_LOGI(TAG, "Received %d bytes from message buffer. Expected byte_size: %d", msg_size, byte_size);

        tflite_invoke(buffer, received_command.sample_size * 6, output_buf, &output_size);
        free(buffer);

        if (received_command.send_to_ws) {
            cJSON* json2ws = cJSON_CreateObject();
            cJSON* payload = cJSON_CreateObject();
            cJSON* output = cJSON_CreateFloatArray(output_buf, output_size);

            cJSON_AddStringToObject(json2ws, "type", "predict");
            cJSON_AddItemToObject(payload, "data", output);
            cJSON_AddNumberToObject(payload, "output_size", output_size);
            cJSON_AddItemToObject(json2ws, "payload", payload);
            char* json2ws_str = cJSON_Print(json2ws);

            xMessageBufferSend(xMessageBufferReqSend, json2ws_str, strlen(json2ws_str), portMAX_DELAY);
            cJSON_Delete(json2ws);
            free(json2ws_str);
        }
    }
    vTaskDelete(NULL);
}
