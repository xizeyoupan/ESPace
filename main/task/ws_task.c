#include "ws_task.h"

static const char *TAG = "WS_TASK";

extern httpd_handle_t server;
extern int fd;
extern user_config_t user_config;

esp_err_t ws_send_data(char *data, uint16_t data_size)
{
    if (data == NULL)
    {
        return ESP_FAIL;
    }

    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = (uint8_t *)data;
    ws_pkt.len = data_size;
    ws_pkt.type = HTTPD_WS_TYPE_BINARY;

    // ESP_LOGI(TAG, "data len:%d", ws_pkt.len);

    httpd_ws_send_frame_async(server, fd, &ws_pkt);
    return ESP_OK;
}

MessageBufferHandle_t xMessageBufferToClient;
char ws_buffer[20000];
void websocket_send_task(void *pvParameters)
{
    BaseType_t core_id = xPortGetCoreID(); // 返回当前任务所在的核心 ID
    ESP_LOGI(TAG, "Task is running on core %d.", core_id);

    // Create Message Buffer
    xMessageBufferToClient = xMessageBufferCreate(40000);
    configASSERT(xMessageBufferToClient);

    while (1)
    {
        size_t msgSize = xMessageBufferReceive(xMessageBufferToClient, ws_buffer, sizeof(ws_buffer), portMAX_DELAY); // 读取完整消息

        if (server && fd != -1)
        {
            ws_send_data(ws_buffer, msgSize);
        }
        else
        {
            // ESP_LOGW(TAG, "server is null");
        }
    }
    vTaskDelete(NULL);
}
