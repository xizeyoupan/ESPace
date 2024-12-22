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
        ESP_LOGI(TAG, "BSSID   \t\t%02X%02X%02X%02X%02X%02X\n",
            ap_info[i].bssid[0], ap_info[i].bssid[1], ap_info[i].bssid[2],
            ap_info[i].bssid[3], ap_info[i].bssid[4], ap_info[i].bssid[5]);

        char bssid[18];
        sprintf(bssid, "%02X-%02X-%02X-%02X-%02X-%02X",
            ap_info[i].bssid[0], ap_info[i].bssid[1], ap_info[i].bssid[2],
            ap_info[i].bssid[3], ap_info[i].bssid[4], ap_info[i].bssid[5]);

        cJSON* wifi_item = cJSON_CreateObject();
        if (cJSON_AddStringToObject(wifi_item, "SSID", (char*)ap_info[i].ssid) == NULL) {
            goto get_wifi_list_end;
        }
        if (cJSON_AddNumberToObject(wifi_item, "RSSI", ap_info[i].rssi) == NULL) {
            goto get_wifi_list_end;
        }
        if (cJSON_AddStringToObject(wifi_item, "BSSID", bssid) == NULL) {
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

char* get_wifi_info(void)
{
    char* string = NULL;
    cJSON* data = cJSON_CreateObject();
    if (data == NULL) {
        goto get_wifi_info_end;
    }

    wifi_mode_t current_mode;
    ESP_ERROR_CHECK(esp_wifi_get_mode(&current_mode));

    if (cJSON_AddNumberToObject(data, "WiFi_mode", current_mode) == NULL) {
        goto get_wifi_info_end;
    }

    if (current_mode != WIFI_MODE_STA) {
        goto get_wifi_info_print;
    }

    wifi_ap_record_t wifi_ap_record;
    ESP_ERROR_CHECK(esp_wifi_sta_get_ap_info(&wifi_ap_record));
    char bssid[18];
    sprintf(bssid, "%02X-%02X-%02X-%02X-%02X-%02X",
        wifi_ap_record.bssid[0], wifi_ap_record.bssid[1], wifi_ap_record.bssid[2],
        wifi_ap_record.bssid[3], wifi_ap_record.bssid[4], wifi_ap_record.bssid[5]);

    if (cJSON_AddStringToObject(data, "SSID", (char*)wifi_ap_record.ssid) == NULL) {
        goto get_wifi_info_end;
    }
    if (cJSON_AddNumberToObject(data, "RSSI", wifi_ap_record.rssi) == NULL) {
        goto get_wifi_info_end;
    }
    if (cJSON_AddStringToObject(data, "BSSID", bssid) == NULL) {
        goto get_wifi_info_end;
    }
    if (cJSON_AddNumberToObject(data, "channel", wifi_ap_record.primary) == NULL) {
        goto get_wifi_info_end;
    }

    esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif == NULL) {
        ESP_LOGE(TAG, "Failed to get network interface");
        goto get_wifi_info_end;
    }

    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
        char ip[16];
        char gw[16];
        char netmask[16];
        sprintf(ip, IPSTR, IP2STR(&ip_info.ip));
        sprintf(gw, IPSTR, IP2STR(&ip_info.gw));
        sprintf(netmask, IPSTR, IP2STR(&ip_info.netmask));
        ESP_LOGI(TAG, "IP:         %s", ip);
        ESP_LOGI(TAG, "Gateway IP: %s", gw);
        ESP_LOGI(TAG, "Netmask:    %s", netmask);

        if (cJSON_AddStringToObject(data, "ip", ip) == NULL) {
            goto get_wifi_info_end;
        }
        if (cJSON_AddStringToObject(data, "gw", gw) == NULL) {
            goto get_wifi_info_end;
        }
        if (cJSON_AddStringToObject(data, "netmask", netmask) == NULL) {
            goto get_wifi_info_end;
        }
    } else {
        ESP_LOGE(TAG, "Failed to get IP information");
        goto get_wifi_info_end;
    }

get_wifi_info_print:
    string = cJSON_Print(data);
    if (string == NULL) {
        ESP_LOGE(TAG, "string == NULL");
        goto get_wifi_info_end;
    }

get_wifi_info_end:
    cJSON_Delete(data);
    return string;
}
