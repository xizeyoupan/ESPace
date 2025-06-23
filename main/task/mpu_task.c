#include "mpu_task.h"

const char* TAG = "MPU_TASK";
extern user_config_t user_config;

MessageBufferHandle_t xMessageBufferMPUCommand;
MessageBufferHandle_t xMessageBufferMPUOut;
MessageBufferHandle_t xMessageBufferMPUOut2CNN;
mpu_command_t command;

void mpu_task(void* pvParameters)
{
    BaseType_t core_id = xPortGetCoreID(); // 返回当前任务所在的核心 ID
    ESP_LOGI(TAG, "Task is running on core %d.", core_id);

    xMessageBufferMPUCommand = xMessageBufferCreate(user_config.mpu_command_buf_size);
    xMessageBufferMPUOut = xMessageBufferCreate(user_config.msg_buf_recv_size);
    xMessageBufferMPUOut2CNN = xMessageBufferCreate(user_config.msg_buf_recv_size);
    configASSERT(xMessageBufferMPUCommand);
    configASSERT(xMessageBufferMPUOut);
    configASSERT(xMessageBufferMPUOut2CNN);

    mpu_mutex_init();
    reset_imu();

    while (1) {
        size_t received = xMessageBufferReceive(xMessageBufferMPUCommand, &command, sizeof(command), portMAX_DELAY);
        if (received != sizeof(command)) {
            ESP_LOGE(TAG, "Received command size mismatch: expected %zu, got %zu", sizeof(command), received);
            continue;
        }

        switch (command.type) {
        case MPU_COMMAND_TYPE_GET_ANGLE:
            ESP_LOGI(TAG, "Received command: GET_ANGLE from %s", command.caller);
            // Call the function to get angle here
            break;

        case MPU_COMMAND_TYPE_GET_ROW:
            // Handle get row command
            ESP_LOGI(TAG, "Received command: GET_ROW from %s", command.caller);
            // Call the function to get row here
            break;

        case MPU_COMMAND_TYPE_RESET_IMU:
            ESP_LOGI(TAG, "Received command: RESET_IMU from %s", command.caller);
            reset_imu();
            break;

        case MPU_COMMAND_TYPE_CLEAR:
            // Handle clear command
            ESP_LOGI(TAG, "Received command: CLEAR from %s", command.caller);
            // Call the function to clear data here
            break;

        default:
            ESP_LOGE(TAG, "Unknown command type: %d", command.type);
            break;
        }
    }

    vTaskDelete(NULL);
}