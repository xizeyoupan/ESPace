#include "http_controller.h"

static const char* TAG = "HTTP_UTIL";

static esp_err_t get_whoami_handler(httpd_req_t* req)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    const char* response = "0721esp32wand";
    return httpd_resp_send(req, response, strlen(response));
}

static esp_err_t wifi_config_post_handler(httpd_req_t* req)
{
    char content[128];
    int ret = httpd_req_recv(req, content, sizeof(content));
    if (ret <= 0) {
        return ESP_FAIL;
    }

    char wifi_ssid[WIFI_SSID_MAX_LEN] = { 0 };
    char wifi_pass[WIFI_PASS_MAX_LEN] = { 0 };

    content[ret] = '\0';
    sscanf(content, "ssid=%31[^&]&password=%63s", wifi_ssid, wifi_pass);
    // ESP_ERR_INVALID_ARG;
    ESP_LOGI(TAG, "Received Wi-Fi credentials: SSID:%s Password:%s", wifi_ssid, wifi_pass);

    save_wifi_config(wifi_ssid, wifi_pass);

    httpd_resp_send(req, "Configuration received. Rebooting...", HTTPD_RESP_USE_STRLEN);
    vTaskDelay(pdMS_TO_TICKS(1000));
    start_sta_mode();
    return ESP_OK;
}

static esp_err_t device_info_get_handler(httpd_req_t* req)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    char* response = get_device_info();

    httpd_resp_send(req, response, strlen(response));
    cJSON_free(response);
    return ESP_OK;
}

static esp_err_t wifi_list_get_handler(httpd_req_t* req)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    char* response = get_wifi_list();

    httpd_resp_send(req, response, strlen(response));
    cJSON_free(response);
    return ESP_OK;
}

// HTTP 服务器启动
httpd_handle_t start_webserver()
{
    httpd_handle_t server = NULL;
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
        .uri = "/config",
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

    ESP_LOGI(TAG, "start_webserver");

    return server;
}
