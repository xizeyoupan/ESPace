#include "littlefs_service.h"
#define MAX_PATH_SIZE 512

#include "espace_define.h"

#include "esp_err.h"
#include "esp_littlefs.h"
#include "esp_log.h"

#include "stdint.h"
#include "string.h"

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

static const char* TAG = "LITTLEFS_SERVICE";

char full_path[MAX_PATH_SIZE];
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

void list_littlefs_files(char* file_list)
{
    DIR* dir = opendir("/littlefs");
    if (!dir) {
        ESP_LOGE(TAG, "Failed to open /littlefs");
        return;
    }

    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL) {
        if (file_list) {
            strcat(file_list, entry->d_name);
            strcat(file_list, "\n");
        }

        snprintf(full_path, MAX_PATH_SIZE, "/littlefs/%s", entry->d_name);

        struct stat st;
        if (stat(full_path, &st) == 0) {
            ESP_LOGI(TAG, "File: %s, Size: %ld bytes", entry->d_name, st.st_size);
        } else {
            ESP_LOGW(TAG, "Failed to stat file: %s", entry->d_name);
        }
    }
    closedir(dir);
}

uint32_t get_file_size(const char* file_name)
{

    snprintf(full_path, MAX_PATH_SIZE, "/littlefs/%s", file_name);
    struct stat st;
    if (stat(full_path, &st) == 0) {
        ESP_LOGI(TAG, "File: %s, Size: %ld bytes", file_name, st.st_size);
        return st.st_size;
    } else {
        ESP_LOGE(TAG, "Failed to stat file: %s", file_name);
        return 0;
    }
}

void read_model_to_buf(const char* model_name, void* buf, int buf_size)
{
    snprintf(full_path, MAX_PATH_SIZE, "/littlefs/%s", model_name);
    uint32_t size = get_file_size(model_name);

    FILE* f = fopen(full_path, "rb");
    rewind(f);
    size_t read_bytes = fread(buf, 1, buf_size, f);
    ESP_LOGI(TAG, "Read %zu bytes into buffer of size %d (file size: %lu)", read_bytes, buf_size, size);
    fclose(f);
}

char old_path[MAX_PATH_SIZE];
char new_path[MAX_PATH_SIZE];
void rename_file(char* old_name, char* new_name)
{

    snprintf(old_path, MAX_PATH_SIZE, "/littlefs/%s", old_name);
    snprintf(new_path, MAX_PATH_SIZE, "/littlefs/%s", new_name);

    if (rename(old_path, new_path) == 0) {
        ESP_LOGI(TAG, "Renamed file from %s to %s", old_name, new_name);
    } else {
        ESP_LOGE(TAG, "Failed to rename file from %s to %s", old_name, new_name);
    }
}

void unlink_file(char* filename)
{
    snprintf(full_path, MAX_PATH_SIZE, "/littlefs/%s", filename);

    if (unlink(full_path) == 0) {
        ESP_LOGI(TAG, "Deleted file: %s", filename);
    } else {
        ESP_LOGE(TAG, "Failed to delete file: %s", filename);
    }
}
