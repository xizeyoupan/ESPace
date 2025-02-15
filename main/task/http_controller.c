#include "http_controller.h"

static const char *TAG = "HTTP_CONTROLLER";

#define MAX_PAYLOAD_LEN 128
uint8_t ws_buf[MAX_PAYLOAD_LEN] = {0};
httpd_handle_t server = NULL;
int fd = -1;
extern user_config_t user_config;

static esp_err_t get_whoami_handler(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    const char *response = "0721esp32wand";
    return httpd_resp_send(req, response, strlen(response));
}

static esp_err_t wifi_config_post_handler(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    char content[128];
    int ret = httpd_req_recv(req, content, sizeof(content));
    if (ret <= 0)
    {
        return ESP_FAIL;
    }

    char wifi_ssid[WIFI_SSID_MAX_LEN + 1] = {0};
    char wifi_pass[WIFI_PASS_MAX_LEN + 1] = {0};

    content[ret] = '\0';
    ESP_LOGI(TAG, "content:\t\t%s", content);

    sscanf(content, "ssid=%32[^&]&password=%64s", wifi_ssid, wifi_pass);
    if (!strlen(wifi_ssid))
    {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, "wifi_ssid is empty", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Received Wi-Fi credentials: SSID:%s Password:%s", wifi_ssid, wifi_pass);
    save_wifi_config(wifi_ssid, wifi_pass);

    httpd_resp_send(req, "Configuration received. Rebooting...", HTTPD_RESP_USE_STRLEN);
    vTaskDelay(pdMS_TO_TICKS(1000));
    start_sta_mode();
    return ESP_OK;
}

static esp_err_t device_info_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    char *response = get_device_info();

    httpd_resp_send(req, response, strlen(response));
    cJSON_free(response);
    return ESP_OK;
}

static esp_err_t wifi_list_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    char *response = get_wifi_list();

    httpd_resp_send(req, response, strlen(response));
    cJSON_free(response);
    return ESP_OK;
}

static esp_err_t wifi_info_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    char *response = get_wifi_info();

    httpd_resp_send(req, response, strlen(response));
    cJSON_free(response);
    return ESP_OK;
}

char once_ws_buffer[1024];
esp_err_t websocket_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET)
    {
        ESP_LOGI(TAG, "Handshake done, connection open");
        return ESP_OK;
    }

    fd = httpd_req_to_sockfd(req);

    // 处理 WebSocket 数据帧
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    ws_pkt.payload = ws_buf;
    httpd_ws_recv_frame(req, &ws_pkt, MAX_PAYLOAD_LEN);
    ws_pkt.payload[ws_pkt.len] = 0;
    ESP_LOGI(TAG, "ws size: %d, data[0]: %d", ws_pkt.len, ws_pkt.payload[0]);

    switch (ws_pkt.payload[0])
    {
    case FETCHED_SET_CONFIG:
        memcpy(&user_config, ws_pkt.payload + 1, sizeof(user_config));
        save_user_config();
        once_ws_buffer[0] = SEND_USER_CONFIG_DATA_PREFIX;
        memcpy(once_ws_buffer + 1, &user_config, sizeof(user_config));
        memcpy(ws_pkt.payload, once_ws_buffer, 1 + sizeof(user_config));
        ws_pkt.len = 1 + sizeof(user_config);
        break;
    case FETCHED_RESET_CONFIG:
        reset_user_config();
        once_ws_buffer[0] = SEND_USER_CONFIG_DATA_PREFIX;
        memcpy(once_ws_buffer + 1, &user_config, sizeof(user_config));
        memcpy(ws_pkt.payload, once_ws_buffer, 1 + sizeof(user_config));
        ws_pkt.len = 1 + sizeof(user_config);
        break;
    case FETCHED_GET_CONFIG:
        once_ws_buffer[0] = SEND_USER_CONFIG_DATA_PREFIX;
        memcpy(once_ws_buffer + 1, &user_config, sizeof(user_config));
        memcpy(ws_pkt.payload, once_ws_buffer, 1 + sizeof(user_config));
        ws_pkt.len = 1 + sizeof(user_config);
        break;

    default:
        break;
    }

    ws_pkt.type = HTTPD_WS_TYPE_BINARY;
    httpd_ws_send_frame(req, &ws_pkt);

    return ESP_OK;
}

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
char ws_buffer[1024];
void websocket_send_task(void *pvParameters)
{
    BaseType_t core_id = xPortGetCoreID(); // 返回当前任务所在的核心 ID
    ESP_LOGI(TAG, "Task is running on core %d.", core_id);

    // Create Message Buffer
    xMessageBufferToClient = xMessageBufferCreate(1024 * 10);
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
}

// HTTP 服务器启动
httpd_handle_t start_webserver()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 1024 * 50;
    ESP_ERROR_CHECK(httpd_start(&server, &config));

    httpd_uri_t whoami_uri = {
        .uri = "/whoami",
        .method = HTTP_GET,
        .handler = get_whoami_handler,
    };
    httpd_register_uri_handler(server, &whoami_uri);

    httpd_uri_t config_uri = {
        .uri = "/connect_ap",
        .method = HTTP_POST,
        .handler = wifi_config_post_handler,
    };
    httpd_register_uri_handler(server, &config_uri);

    httpd_uri_t info_uri = {
        .uri = "/info",
        .method = HTTP_GET,
        .handler = device_info_get_handler,
    };
    httpd_register_uri_handler(server, &info_uri);

    httpd_uri_t wifi_list_uri = {
        .uri = "/wifi_list",
        .method = HTTP_GET,
        .handler = wifi_list_get_handler,
    };
    httpd_register_uri_handler(server, &wifi_list_uri);

    httpd_uri_t wifi_info_uri = {
        .uri = "/wifi_info",
        .method = HTTP_GET,
        .handler = wifi_info_get_handler,
    };
    httpd_register_uri_handler(server, &wifi_info_uri);

    httpd_uri_t ws_uri = {
        .uri = "/ws",
        .method = HTTP_GET,
        .handler = websocket_handler,
        .is_websocket = true};
    httpd_register_uri_handler(server, &ws_uri);

    ESP_LOGI(TAG, "start_webserver");

    return server;
}
