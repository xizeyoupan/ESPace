#include "tflite_task.h"

static const char* TAG = "TFLITE_TASK";
extern MessageBufferHandle_t xMessageBufferMPUOut2CNN;
extern mpu_command_t received_command;

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

        size_t msg_size = xMessageBufferReceive(xMessageBufferMPUOut2CNN, buffer, byte_size, portMAX_DELAY);
        ESP_LOGI(TAG, "Received %d bytes from message buffer. Expected byte_size: %d", msg_size, byte_size);

        tflite_invoke(buffer, received_command.sample_size * 6);

        free(buffer);
    }
    vTaskDelete(NULL);
}
