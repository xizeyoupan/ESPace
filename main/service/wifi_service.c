#include "wifi_service.h"
static const char* TAG = "WIFI_SERVICE";

wifi_ap_record_t ap_info[CONFIG_WIFI_SCAN_LIST_SIZE];
wifi_scan_config_t wifi_scan_config;
char temp_str[50] = { '\0' };

char* get_wifi_list(void)
{
    char* string = NULL;
    cJSON* data = cJSON_CreateObject();
    if (data == NULL) {
        goto get_wifi_list_end;
    }

    cJSON* wifi_lsit = NULL;
    wifi_lsit = cJSON_AddArrayToObject(data, "wifi_lsit");
    if (wifi_lsit == NULL) {
        goto get_wifi_list_end;
    }

    uint16_t number = CONFIG_WIFI_SCAN_LIST_SIZE;
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));

    wifi_scan_config.show_hidden = true;
    wifi_scan_config.scan_type = WIFI_SCAN_TYPE_ACTIVE;
    wifi_scan_config.scan_time.active.min = 0;
    wifi_scan_config.scan_time.active.max = 120;
    wifi_scan_config.scan_time.passive = 360;
    wifi_scan_config.home_chan_dwell_time = 30;

    ESP_ERROR_CHECK(esp_wifi_scan_start(&wifi_scan_config, true));

    ESP_LOGI(TAG, "Max AP number ap_info can hold = %u", number);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ESP_LOGI(TAG, "Total APs scanned = %u, actual AP number ap_info holds = %u", ap_count, number);

    for (size_t i = 0; i < number; i++) {
        ESP_LOGI(TAG, "SSID    \t\t%s", ap_info[i].ssid);
        ESP_LOGI(TAG, "RSSI    \t\t%d", ap_info[i].rssi);
        ESP_LOGI(TAG, "Channel \t\t%d", ap_info[i].primary);

        cJSON* wifi_item = cJSON_CreateObject();
        if (cJSON_AddStringToObject(wifi_item, "SSID", (char*)ap_info[i].ssid) == NULL) {
            goto get_wifi_list_end;
        }
        if (cJSON_AddNumberToObject(wifi_item, "RSSI", ap_info[i].rssi) == NULL) {
            goto get_wifi_list_end;
        }
        if (cJSON_AddNumberToObject(wifi_item, "channel", ap_info[i].primary) == NULL) {
            goto get_wifi_list_end;
        }
        if (cJSON_AddNumberToObject(wifi_item, "authmode", ap_info[i].authmode) == NULL) {
            goto get_wifi_list_end;
        }

        memcpy(temp_str, ap_info[i].country.cc, 2);
        if (cJSON_AddStringToObject(wifi_item, "country", temp_str) == NULL) {
            goto get_wifi_list_end;
        }

        cJSON_AddItemToArray(wifi_lsit, wifi_item);
    }

    string = cJSON_Print(data);
    if (string == NULL) {
        ESP_LOGE(TAG, "string == NULL");
        goto get_wifi_list_end;
    }

get_wifi_list_end:
    cJSON_Delete(data);
    return string;
}
