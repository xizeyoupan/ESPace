#include "wand_server_task.h"
#include "cJSON.h"

#define WIFI_SSID_MAX_LEN 32
#define WIFI_PASS_MAX_LEN 64
#define AP_SSID "ESP32_Config"
#define AP_PASS "12345678"
static const char *TAG = "WAND_SERVER_TASK";

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

static char wifi_ssid[WIFI_SSID_MAX_LEN] = {0};
static char wifi_pass[WIFI_PASS_MAX_LEN] = {0};

const int WIFI_CONNECTED_BIT = BIT0;
const int WIFI_FAIL_BIT = BIT1;

static esp_netif_t *sta_netif = NULL;
static esp_netif_t *ap_netif = NULL;
httpd_handle_t server = NULL;

static int s_retry_num = 0;

// Wi-Fi 事件处理程序
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT)
    {
        if (event_id == WIFI_EVENT_STA_START)
        {
            ESP_LOGI(TAG, "WIFI_EVENT_STA_START.");
            esp_wifi_connect();
        }
        else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
        {
            if (s_retry_num < CONFIG_ESP_MAXIMUM_RETRY)
            {
                esp_wifi_connect();
                s_retry_num++;
                ESP_LOGI(TAG, "retry to connect to the AP, cnt = %d", s_retry_num);
            }
            else
            {
                ESP_LOGI(TAG, "Failed to connect, switching back to AP mode.");
                xEventGroupClearBits(s_wifi_event_group, WIFI_FAIL_BIT);
                esp_wifi_stop();
                start_ap_mode();
            }
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        s_retry_num = 0;
    }
}

// 配置 STA 模式
static void start_sta_mode()
{
    ESP_LOGI(TAG, "Starting STA Mode...");
    esp_wifi_stop();

    wifi_config_t sta_config = {
        .sta = {
            .ssid = "",
            .password = "",
        },
    };

    strncpy((char *)sta_config.sta.ssid, wifi_ssid, strlen(wifi_ssid));
    strncpy((char *)sta_config.sta.password, wifi_pass, strlen(wifi_pass));

    ESP_LOGI(TAG, "connecting to ap SSID:%s password:%s", sta_config.sta.ssid, sta_config.sta.password);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

// 从 NVS 加载 Wi-Fi 配置
static bool load_wifi_config()
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("wifi_config", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK)
    {
        return false;
    }
    size_t ssid_len = WIFI_SSID_MAX_LEN;
    size_t pass_len = WIFI_PASS_MAX_LEN;
    err = nvs_get_str(nvs_handle, "ssid", wifi_ssid, &ssid_len);
    if (err != ESP_OK)
    {
        nvs_close(nvs_handle);
        return false;
    }
    err = nvs_get_str(nvs_handle, "password", wifi_pass, &pass_len);
    if (err != ESP_OK)
    {
        nvs_close(nvs_handle);
        return false;
    }
    nvs_close(nvs_handle);
    return true;
}

// HTTP 服务器处理函数
static esp_err_t root_get_handler(httpd_req_t *req)
{
    const char *response =
        "<html><body><h1>Wi-Fi Config</h1>"
        "<form action=\"/config\" method=\"post\">"
        "SSID: <input type=\"text\" name=\"ssid\"><br>"
        "Password: <input type=\"password\" name=\"password\"><br>"
        "<input type=\"submit\" value=\"Submit\">"
        "</form></body></html>";
    httpd_resp_send(req, response, strlen(response));
    return ESP_OK;
}

// 保存 Wi-Fi 配置到 NVS
static void save_wifi_config()
{
    nvs_handle_t nvs_handle;
    ESP_ERROR_CHECK(nvs_open("wifi_config", NVS_READWRITE, &nvs_handle));
    ESP_ERROR_CHECK(nvs_set_str(nvs_handle, "ssid", wifi_ssid));
    ESP_ERROR_CHECK(nvs_set_str(nvs_handle, "password", wifi_pass));
    ESP_ERROR_CHECK(nvs_commit(nvs_handle));
    nvs_close(nvs_handle);
}

static esp_err_t wifi_config_post_handler(httpd_req_t *req)
{
    char content[128];
    int ret = httpd_req_recv(req, content, sizeof(content));
    if (ret <= 0)
    {
        return ESP_FAIL;
    }

    content[ret] = '\0';
    sscanf(content, "ssid=%31[^&]&password=%63s", wifi_ssid, wifi_pass);

    ESP_LOGI(TAG, "Received Wi-Fi credentials: SSID:%s Password:%s", wifi_ssid, wifi_pass);

    save_wifi_config();

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

static esp_err_t test_get_handler(httpd_req_t *req)
{
    char *response = create_monitor_with_helpers();

    httpd_resp_send(req, response, strlen(response));
    cJSON_free(response);
    return ESP_OK;
}

// HTTP 请求拦截器
esp_err_t http_request_interceptor(httpd_req_t *req)
{
    // 打印请求信息
    ESP_LOGI(TAG, "Intercepted HTTP request: %d %s", req->method, req->uri);

    // 可以在这里做一些检查，比如验证 token 或者记录日志等
    // 例如：如果请求的路径是 /restricted，拒绝处理请求
    if (strncmp(req->uri, "/restricted", strlen("/restricted")) == 0)
    {
        ESP_LOGW(TAG, "Access to /restricted is not allowed!");
        httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "Access denied");
        return ESP_FAIL; // 阻止继续处理这个请求
    }

    // 请求通过拦截器，继续处理
    return ESP_OK;
}

// HTTP 服务器启动
static httpd_handle_t start_webserver()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(httpd_start(&server, &config));

    httpd_uri_t root_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_get_handler,
    };
    httpd_register_uri_handler(server, &root_uri);

    httpd_uri_t config_uri = {
        .uri = "/config",
        .method = HTTP_POST,
        .handler = wifi_config_post_handler,
    };
    httpd_register_uri_handler(server, &config_uri);

    httpd_uri_t test_uri = {
        .uri = "/test",
        .method = HTTP_GET,
        .handler = test_get_handler,
    };
    httpd_register_uri_handler(server, &test_uri);

    ESP_LOGI(TAG, "start_webserver");

    return server;
}

static void start_ap_mode()
{
    ESP_LOGI(TAG, "Starting AP Mode...");
    esp_wifi_stop();

    wifi_config_t ap_config = {
        .ap = {
            .ssid = AP_SSID,
            .ssid_len = strlen(AP_SSID),
            .password = AP_PASS,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
        },
    };

    if (strlen(AP_PASS) == 0)
    {
        ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "AP Mode started. SSID:%s Password:%s", AP_SSID, AP_PASS);
}

esp_err_t start_wifi(void)
{
    esp_err_t ret_value = ESP_OK;
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ap_netif = esp_netif_create_default_wifi_ap();
    sta_netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));

    start_webserver();

    if (load_wifi_config())
    {
        start_sta_mode();
    }
    else
    {
        start_ap_mode();
    }

    return ret_value;
}

void wand_server_task(void *pvParameters)
{
    start_wifi();
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}