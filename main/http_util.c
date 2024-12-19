#include "http_util.h"

static const char *TAG = "HTTP_UTIL";

static esp_err_t get_whoami_handler(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    const char *response = "0721esp32wand";
    return httpd_resp_send(req, response, strlen(response));
}

static esp_err_t wifi_config_post_handler(httpd_req_t *req)
{
    char content[128];
    int ret = httpd_req_recv(req, content, sizeof(content));
    if (ret <= 0)
    {
        return ESP_FAIL;
    }

    char wifi_ssid[WIFI_SSID_MAX_LEN] = {0};
    char wifi_pass[WIFI_PASS_MAX_LEN] = {0};

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

char *create_monitor_with_helpers(void)
{
    const unsigned int resolution_numbers[3][2] = {
        {1280, 720},
        {1920, 1080},
        {3840, 2160}};
    char *string = NULL;
    cJSON *resolutions = NULL;
    size_t index = 0;

    cJSON *monitor = cJSON_CreateObject();

    if (cJSON_AddStringToObject(monitor, "name", "Awesome 4K") == NULL)
    {
        goto json_end;
    }

    resolutions = cJSON_AddArrayToObject(monitor, "resolutions");
    if (resolutions == NULL)
    {
        goto json_end;
    }

    for (index = 0; index < (sizeof(resolution_numbers) / (2 * sizeof(int))); ++index)
    {
        cJSON *resolution = cJSON_CreateObject();

        if (cJSON_AddNumberToObject(resolution, "width", resolution_numbers[index][0]) == NULL)
        {
            goto json_end;
        }

        if (cJSON_AddNumberToObject(resolution, "height", resolution_numbers[index][1]) == NULL)
        {
            goto json_end;
        }

        cJSON_AddItemToArray(resolutions, resolution);
    }

    string = cJSON_Print(monitor);
    if (string == NULL)
    {
        fprintf(stderr, "Failed to print monitor.\n");
    }

json_end:
    cJSON_Delete(monitor);
    return string;
}

static char *get_device_info(void)
{
    char *string = NULL;
    cJSON *data = cJSON_CreateObject();
    if (data == NULL)
    {
        goto get_device_info_end;
    }

    cJSON *compile_time = NULL;
    compile_time = cJSON_CreateString(__TIMESTAMP__);
    if (compile_time == NULL)
    {
        goto get_device_info_end;
    }
    cJSON_AddItemToObject(data, "compile_time", compile_time);

    cJSON *git_commit_id = NULL;
    git_commit_id = cJSON_CreateString(GIT_COMMIT_SHA1);
    if (git_commit_id == NULL)
    {
        goto get_device_info_end;
    }
    cJSON_AddItemToObject(data, "git_commit_id", git_commit_id);

    cJSON *firmware_version = NULL;
    firmware_version = cJSON_CreateString(SW_VERSION);
    if (firmware_version == NULL)
    {
        goto get_device_info_end;
    }
    cJSON_AddItemToObject(data, "firmware_version", firmware_version);

    string = cJSON_Print(data);
    if (string == NULL)
    {
        ESP_LOGE(TAG, "string == NULL");
        goto get_device_info_end;
    }

get_device_info_end:
    cJSON_Delete(data);
    return string;
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

// HTTP 服务器启动
httpd_handle_t start_webserver()
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
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

    httpd_uri_t dev_uri = {
        .uri = "/dev",
        .method = HTTP_GET,
        .handler = device_info_get_handler,
    };
    httpd_register_uri_handler(server, &dev_uri);

    ESP_LOGI(TAG, "start_webserver");

    return server;
}
