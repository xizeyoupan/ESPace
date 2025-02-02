#include "status_task.h"

#define STAT_DATA_SIZE 512
static const char *TAG = "STAT";

/*
1 byte: STAT_DATA_PREFIX
1 bytes: task_count

1 byte: task_name_length
n bytes: task_name
1 byte: task_number
1 byte: task_state
2 bytes: stack_water_mark

4 bytes: total_free_bytes
4 bytes: total_allocated_bytes
4 bytes: largest_free_block
4 bytes: minimum_free_bytes
2 bytes: ws_bytes_available
*/
char stat_data[STAT_DATA_SIZE];

extern MessageBufferHandle_t xMessageBufferToClient;

void ws_send_task_state(void)
{
    uint8_t task_count = uxTaskGetNumberOfTasks(); // 获取当前任务数量
    TaskStatus_t *task_status_array = malloc(sizeof(TaskStatus_t) * task_count);
    uint16_t data_index = 0;
    stat_data[data_index++] = STAT_DATA_PREFIX;

    if (task_status_array != NULL)
    {
        // 获取所有任务的状态信息
        task_count = uxTaskGetSystemState(task_status_array, task_count, NULL);
        memcpy(stat_data + data_index, &task_count, sizeof(task_count));
        data_index += sizeof(task_count);

        // 打印任务状态信息
        for (uint8_t i = 0; i < task_count; i++)
        {
            const char *pcTaskName = task_status_array[i].pcTaskName;
            uint8_t xTaskNumber = task_status_array[i].xTaskNumber;
            uint8_t eCurrentState = task_status_array[i].eCurrentState;
            uint16_t usStackHighWaterMark = task_status_array[i].usStackHighWaterMark;

            stat_data[data_index++] = strlen(pcTaskName);
            memcpy(stat_data + data_index, pcTaskName, strlen(pcTaskName));
            data_index += strlen(pcTaskName);

            memcpy(stat_data + data_index, &xTaskNumber, sizeof(xTaskNumber));
            data_index += sizeof(xTaskNumber);

            memcpy(stat_data + data_index, &eCurrentState, sizeof(eCurrentState));
            data_index += sizeof(eCurrentState);

            memcpy(stat_data + data_index, &usStackHighWaterMark, sizeof(usStackHighWaterMark));
            data_index += sizeof(usStackHighWaterMark);

            // ESP_LOGI(TAG, "Task %s (ID: %u) is in state %u and water mark %uB.", pcTaskName, xTaskNumber, eCurrentState, usStackHighWaterMark);
        }
        free(task_status_array); // 释放分配的内存
    }
    else
    {
        ESP_LOGE(TAG, "Failed to allocate memory for task state information.");
    }

    // 定义一个 heap_caps_info_t 结构体来存储内存信息
    multi_heap_info_t heap_info;
    // 获取堆内存的信息
    heap_caps_get_info(&heap_info, MALLOC_CAP_8BIT);

    uint32_t total_free_bytes = heap_info.total_free_bytes;
    uint32_t total_allocated_bytes = heap_info.total_allocated_bytes;
    uint32_t largest_free_block = heap_info.largest_free_block;
    uint32_t minimum_free_bytes = heap_info.minimum_free_bytes;

    memcpy(stat_data + data_index, &total_free_bytes, sizeof(total_free_bytes));
    data_index += sizeof(total_free_bytes);
    memcpy(stat_data + data_index, &total_allocated_bytes, sizeof(total_allocated_bytes));
    data_index += sizeof(total_allocated_bytes);
    memcpy(stat_data + data_index, &largest_free_block, sizeof(largest_free_block));
    data_index += sizeof(largest_free_block);
    memcpy(stat_data + data_index, &minimum_free_bytes, sizeof(minimum_free_bytes));
    data_index += sizeof(minimum_free_bytes);

    uint16_t ws_bytes_available = xMessageBufferSpacesAvailable(xMessageBufferToClient);
    memcpy(stat_data + data_index, &ws_bytes_available, sizeof(ws_bytes_available));
    data_index += sizeof(ws_bytes_available);

    // ESP_LOGI(TAG, "Heap Info: Total free: %lu, Total allocated: %lu, Largest free block: %lu, Minimum free: %lu", total_free_bytes, total_allocated_bytes, largest_free_block, minimum_free_bytes);
    // ESP_LOGI(TAG, "data size: %d", data_index);

    xMessageBufferSend(xMessageBufferToClient, stat_data, data_index, 0);
}

void status_task(void *pvParameters)
{
    BaseType_t core_id = xPortGetCoreID(); // 返回当前任务所在的核心 ID
    ESP_LOGI(TAG, "Task is running on core %d.", core_id);

    while (1)
    {
        ws_send_task_state();
        vTaskDelay(pdMS_TO_TICKS(2500));
    }
}
