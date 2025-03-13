#include "http_service.h"

static const char *TAG = "HTTP_SERVICE";

#define MAX_PAYLOAD_LEN 1024
uint8_t ws_buf[MAX_PAYLOAD_LEN] = {0};
httpd_handle_t server           = NULL;
int fd                          = -1;

extern user_config_t user_config;
extern mode_index_type mode_index;
extern SemaphoreHandle_t control_block_mutex;
extern ccb_t circulation_control_block_array[NUM_OF_MODULES];

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
    if (ret <= 0) {
        return ESP_FAIL;
    }

    char wifi_ssid[WIFI_SSID_MAX_LEN + 1] = {0};
    char wifi_pass[WIFI_PASS_MAX_LEN + 1] = {0};

    content[ret] = '\0';
    ESP_LOGI(TAG, "content:\t\t%s", content);

    sscanf(content, "ssid=%32[^&]&password=%64s", wifi_ssid, wifi_pass);
    if (!strlen(wifi_ssid)) {
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

esp_err_t websocket_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "Handshake done, connection open");
        return ESP_OK;
    }

    fd = httpd_req_to_sockfd(req);

    // 处理 WebSocket 数据帧
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type    = HTTPD_WS_TYPE_TEXT;
    ws_pkt.payload = ws_buf;
    httpd_ws_recv_frame(req, &ws_pkt, MAX_PAYLOAD_LEN);
    ws_pkt.payload[ws_pkt.len] = 0;
    ESP_LOGI(TAG, "Receive ws size: %d, data[0]: %d", ws_pkt.len, ws_pkt.payload[0]);

    switch (ws_pkt.payload[0]) {
        case FETCHED_SET_CONFIG:
            memcpy(&user_config, ws_pkt.payload + 1, sizeof(user_config));
            save_user_config();
            ws_pkt.payload[0] = SEND_USER_CONFIG_DATA_PREFIX;
            memcpy(ws_pkt.payload + 1, &user_config, sizeof(user_config));
            ws_pkt.len = 1 + sizeof(user_config);
            break;
        case FETCHED_RESET_CONFIG:
            reset_user_config();
            ws_pkt.payload[0] = SEND_USER_CONFIG_DATA_PREFIX;
            memcpy(ws_pkt.payload + 1, &user_config, sizeof(user_config));
            ws_pkt.len = 1 + sizeof(user_config);
            break;
        case FETCHED_GET_CONFIG:
            ws_pkt.payload[0] = SEND_USER_CONFIG_DATA_PREFIX;
            memcpy(ws_pkt.payload + 1, &user_config, sizeof(user_config));
            ws_pkt.len = 1 + sizeof(user_config);
            break;
        case FETCHED_RESET_IMU:
            reset_imu();
            return ESP_OK;
            break;
        case FETCHED_READY_TO_SCAN:
            model_type_t m_type    = ws_pkt.payload[1];
            uint16_t sample_tick = ws_pkt.payload[2] << 8 | ws_pkt.payload[3];
            uint16_t sample_size = ws_pkt.payload[4] << 8 | ws_pkt.payload[5];

            xSemaphoreTake(control_block_mutex, portMAX_DELAY);
            mode_index                                       = MODE_DATASET;
            circulation_control_block_array[mode_index].type = m_type;
            if (m_type == COMMAND_MODEL) {
                circulation_control_block_array[mode_index].command_mcb.sample_tick = sample_tick;
                circulation_control_block_array[mode_index].command_mcb.sample_size = sample_size;
            } else {
                circulation_control_block_array[mode_index].continuous_mcb.sample_tick = sample_tick;
                circulation_control_block_array[mode_index].continuous_mcb.sample_size = sample_size;
            }
            xSemaphoreGive(control_block_mutex);

            ESP_LOGI(TAG, "dataset info: type: %d, sample_tick: %d, sample_size: %d", m_type, sample_tick, sample_size);
            return ESP_OK;
            break;
        default:
            break;
    }

    ws_pkt.type = HTTPD_WS_TYPE_BINARY;
    httpd_ws_send_frame(req, &ws_pkt);

    return ESP_OK;
}

// HTTP 服务器启动
httpd_handle_t start_webserver()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(httpd_start(&server, &config));

    httpd_uri_t whoami_uri = {
        .uri     = "/whoami",
        .method  = HTTP_GET,
        .handler = get_whoami_handler,
    };
    httpd_register_uri_handler(server, &whoami_uri);

    httpd_uri_t config_uri = {
        .uri     = "/connect_ap",
        .method  = HTTP_POST,
        .handler = wifi_config_post_handler,
    };
    httpd_register_uri_handler(server, &config_uri);

    httpd_uri_t info_uri = {
        .uri     = "/info",
        .method  = HTTP_GET,
        .handler = device_info_get_handler,
    };
    httpd_register_uri_handler(server, &info_uri);

    httpd_uri_t wifi_list_uri = {
        .uri     = "/wifi_list",
        .method  = HTTP_GET,
        .handler = wifi_list_get_handler,
    };
    httpd_register_uri_handler(server, &wifi_list_uri);

    httpd_uri_t wifi_info_uri = {
        .uri     = "/wifi_info",
        .method  = HTTP_GET,
        .handler = wifi_info_get_handler,
    };
    httpd_register_uri_handler(server, &wifi_info_uri);

    httpd_uri_t ws_uri = {
        .uri          = "/ws",
        .method       = HTTP_GET,
        .handler      = websocket_handler,
        .is_websocket = true};
    httpd_register_uri_handler(server, &ws_uri);

    ESP_LOGI(TAG, "start_webserver");

    return server;
}
