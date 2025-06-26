#include "http_service.h"

#define MIN(a, b) (a) < (b) ? (a) : (b)

static const char* TAG = "HTTP_SERVICE";

httpd_handle_t server = NULL;
int current_ws_fd = -1;
extern MessageBufferHandle_t xMessageBufferReqRecv;
extern user_config_t user_config;

static esp_err_t get_whoami_handler(httpd_req_t* req)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    const char* response = "0721ESPace";
    return httpd_resp_send(req, response, strlen(response));
}

static char query[2048];
static esp_err_t auth_req(httpd_req_t* req)
{
    char token[128];
    char expected_token[128];

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        if (httpd_query_key_value(query, "token", token, sizeof(token)) == ESP_OK) {
            ESP_LOGI(TAG, "Token received: %s", token);
            snprintf(expected_token, sizeof(expected_token), "%s:%s", user_config.username, user_config.password);
            if (strcmp(token, expected_token) != 0) {
                ESP_LOGW(TAG, "Unauthorized access: %s", token);
                httpd_resp_set_status(req, "401 Unauthorized");
                httpd_resp_send(req, "Unauthorized", HTTPD_RESP_USE_STRLEN);
                return ESP_FAIL;
            }
        } else {
            ESP_LOGW(TAG, "Authorization token missing");
            httpd_resp_set_status(req, "401 Unauthorized");
            httpd_resp_send(req, "Unauthorized", HTTPD_RESP_USE_STRLEN);
            return ESP_FAIL;
        }
    } else {
        ESP_LOGW(TAG, "Authorization token missing");
        httpd_resp_set_status(req, "401 Unauthorized");
        httpd_resp_send(req, "Unauthorized", HTTPD_RESP_USE_STRLEN);
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t websocket_handler(httpd_req_t* req)
{
    if (req->method == HTTP_GET) {

        esp_err_t auth_result = auth_req(req);
        if (auth_result != ESP_OK) {
            return auth_result;
        }

        if (httpd_ws_get_fd_info(req->handle, httpd_req_to_sockfd(req)) == HTTPD_WS_CLIENT_WEBSOCKET) {
            ESP_LOGI(TAG, "Client connected");
        }

        return ESP_OK;
    }

    current_ws_fd = httpd_req_to_sockfd(req);

    uint8_t* ws_recv_buf = NULL;
    ws_recv_buf = (uint8_t*)malloc(user_config.ws_recv_buf_size);

    // 处理 WebSocket 数据帧
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = ws_recv_buf;
    httpd_ws_recv_frame(req, &ws_pkt, user_config.ws_recv_buf_size);
    ws_pkt.payload[ws_pkt.len] = 0;
    ESP_LOGI(TAG, "Receive ws size: %d, data: %s", ws_pkt.len, ws_pkt.payload);

    if (ws_pkt.type == HTTPD_WS_TYPE_CLOSE) {
        current_ws_fd = -1;
        ESP_LOGI(TAG, "Ws client disconnected");
    } else {
        xMessageBufferSend(xMessageBufferReqRecv, ws_pkt.payload, ws_pkt.len, portMAX_DELAY);
    }

    free(ws_recv_buf);
    return ESP_OK;
}

esp_err_t ota_post_handler(httpd_req_t* req)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    esp_err_t auth_result = auth_req(req);
    if (auth_result != ESP_OK) {
        return auth_result;
    }

    esp_ota_handle_t ota_handle;
    const esp_partition_t* ota_partition = esp_ota_get_next_update_partition(NULL);
    if (!ota_partition) {
        ESP_LOGE("OTA", "No OTA partition found");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No OTA partition");
        return ESP_FAIL;
    }

    ESP_LOGI("OTA", "Start updating to partition: %s", ota_partition->label);
    esp_err_t err = esp_ota_begin(ota_partition, OTA_SIZE_UNKNOWN, &ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE("OTA", "esp_ota_begin failed: %s", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA begin failed");
        esp_ota_abort(ota_handle);
        return err;
    }

    const size_t BUF_SIZE = 1024 * 10;
    char* buf = (char*)malloc(BUF_SIZE);

    // 读取并写入 OTA 数据
    int remaining = req->content_len;
    int total = 0;
    int received;
    ESP_LOGI("OTA", "Receiving %d bytes", remaining);
    while (remaining > 0) {
        received = httpd_req_recv(req, buf, MIN(remaining, BUF_SIZE));
        if (received <= 0)
            break;
        esp_ota_write(ota_handle, buf, received);
        remaining -= received;
        total += received;
    }

    free(buf);
    esp_ota_end(ota_handle);

    // 验证并切换
    err = esp_ota_set_boot_partition(ota_partition);
    if (err != ESP_OK) {
        ESP_LOGE("OTA", "esp_ota_set_boot_partition failed: %s", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA set boot partition failed");
        return err;
    }
    ESP_LOGI("OTA", "Update complete: %d bytes. Restarting...", total);
    httpd_resp_sendstr(req, "Firmware update successful. Rebooting...");
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    esp_restart();
    return ESP_OK;
}

esp_err_t model_post_handler(httpd_req_t* req)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    esp_err_t auth_result = auth_req(req);
    if (auth_result != ESP_OK) {
        return auth_result;
    }

    char xor_code[4];
    char model_name[512];
    char model_path[512];

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        if (httpd_query_key_value(query, "xor", xor_code, sizeof(xor_code)) == ESP_OK) {
            ESP_LOGI(TAG, "xor received: %s", xor_code);
        } else {
            ESP_LOGW(TAG, "XOR parameter missing");
            httpd_resp_set_status(req, "400 Bad Request");
            httpd_resp_send(req, "XOR parameter missing", HTTPD_RESP_USE_STRLEN);
            return ESP_FAIL;
        }

        if (httpd_query_key_value(query, "name", model_name, sizeof(model_name)) == ESP_OK) {
            ESP_LOGI(TAG, "model_name received: %s", model_name);
        } else {
            ESP_LOGW(TAG, "Model name parameter missing");
            httpd_resp_set_status(req, "400 Bad Request");
            httpd_resp_send(req, "Model name parameter missing", HTTPD_RESP_USE_STRLEN);
            return ESP_FAIL;
        }
    }

    const size_t BUF_SIZE = 1024 * 2;
    char* buf = (char*)malloc(BUF_SIZE);

    int remaining = req->content_len;
    int total = 0;
    int received;
    int received_xor = 0;
    ESP_LOGI("Upload model", "Receiving %d bytes", remaining);

    strcpy(model_path, "/littlefs/");
    strcat(model_path, model_name);

    ESP_LOGI("Upload model", "model path: %s", model_path);

    FILE* f = fopen(model_path, "wb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }

    while (remaining > 0) {
        received = httpd_req_recv(req, buf, MIN(remaining, BUF_SIZE));
        if (received <= 0)
            break;

        size_t written = fwrite(buf, 1, received, f);
        if (written != received) {
            ESP_LOGE("upload", "File write error");
            fclose(f);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "File write failed");
            return ESP_FAIL;
        }

        for (int i = 0; i < received; ++i) {
            received_xor ^= buf[i];
        }

        remaining -= received;
        total += received;
    }

    ESP_LOGI("Upload model", "Received %d bytes, expected XOR:%s, XOR: %d", total, xor_code, received_xor);
    fclose(f);
    free(buf);

    uint32_t size = get_file_size(model_name);
    if (size == 0) {
        ESP_LOGE(TAG, "Uploaded file size is 0, removing file");
        return ESP_FAIL;
    } else {
        buf = (char*)malloc(size);
        if (!buf) {
            ESP_LOGE(TAG, "Failed to allocate buffer for verification");
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
            return ESP_FAIL;
        }

        read_model_to_buf(model_name, buf, size);

        int input_size, output_size;
        load_and_check_model(buf, &input_size, &output_size);
        free(buf);
    }

    list_littlefs_files(NULL);

    httpd_resp_sendstr(req, "Model upload successful.");
    return ESP_OK;
}

esp_err_t options_handler(httpd_req_t* req)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");

    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

// HTTP 服务器启动
httpd_handle_t start_webserver()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 16;
    config.stack_size = 1024 * 10;
    ESP_ERROR_CHECK(httpd_start(&server, &config));

    httpd_uri_t cors_options_uri = {
        .uri = "/upload",
        .method = HTTP_OPTIONS,
        .handler = options_handler,
    };
    httpd_register_uri_handler(server, &cors_options_uri);

    httpd_uri_t whoami_uri = {
        .uri = "/whoami",
        .method = HTTP_GET,
        .handler = get_whoami_handler,
    };
    httpd_register_uri_handler(server, &whoami_uri);

    httpd_uri_t ws_uri = {
        .uri = "/esp-ws",
        .method = HTTP_GET,
        .handler = websocket_handler,
        .is_websocket = true,
    };
    httpd_register_uri_handler(server, &ws_uri);

    httpd_uri_t ota_post_uri = {
        .uri = "/upload",
        .method = HTTP_POST,
        .handler = ota_post_handler,
    };
    httpd_register_uri_handler(server, &ota_post_uri);

    httpd_uri_t model_post_uri = {
        .uri = "/upload_model",
        .method = HTTP_POST,
        .handler = model_post_handler,
    };
    httpd_register_uri_handler(server, &model_post_uri);

    ESP_LOGI(TAG, "start_webserver");

    return server;
}