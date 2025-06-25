#include "littlefs_service.h"

static const char* TAG = "LITTLEFS_SERVICE";

void littlefs_init(void)
{
    ESP_LOGI(TAG, "Initializing LittleFS");

    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/littlefs",
        .partition_label = "models",
        .format_if_mount_failed = true,
        .dont_mount = false,
    };

    esp_err_t ret = esp_vfs_littlefs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find LittleFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_littlefs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get LittleFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
}

void list_littlefs_files(void)
{
    DIR* dir = opendir("/littlefs");
    if (!dir) {
        ESP_LOGE(TAG, "Failed to open /littlefs");
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        char full_path[512];
        snprintf(full_path, sizeof(full_path), "/littlefs/%s", entry->d_name);

        struct stat st;
        if (stat(full_path, &st) == 0) {
            ESP_LOGI(TAG, "File: %s, Size: %ld bytes", entry->d_name, st.st_size);
        } else {
            ESP_LOGW(TAG, "Failed to stat file: %s", entry->d_name);
        }
    }

    closedir(dir);
}