#include "device_service.h"

static const char* TAG = "DEVICE_SERVICE";

char* get_device_info(void)
{
    char* string = NULL;
    cJSON* data = cJSON_CreateObject();
    if (data == NULL) {
        goto get_device_info_end;
    }

    cJSON* compile_time = NULL;
    compile_time = cJSON_CreateString(__TIMESTAMP__);
    if (compile_time == NULL) {
        goto get_device_info_end;
    }
    cJSON_AddItemToObject(data, "compile_time", compile_time);

    cJSON* git_commit_id = NULL;
    git_commit_id = cJSON_CreateString("GIT_COMMIT_SHA1");
    if (git_commit_id == NULL) {
        goto get_device_info_end;
    }
    cJSON_AddItemToObject(data, "git_commit_id", git_commit_id);

    cJSON* firmware_version = NULL;
    firmware_version = cJSON_CreateString(SW_VERSION);
    if (firmware_version == NULL) {
        goto get_device_info_end;
    }
    cJSON_AddItemToObject(data, "firmware_version", firmware_version);

    cJSON* package_version = NULL;
    package_version = cJSON_CreateNumber(efuse_ll_get_chip_ver_pkg());
    if (package_version == NULL) {
        goto get_device_info_end;
    }
    cJSON_AddItemToObject(data, "package_version", package_version);

    cJSON* chip_version = NULL;
    chip_version = cJSON_CreateNumber(efuse_hal_chip_revision());
    if (chip_version == NULL) {
        goto get_device_info_end;
    }
    cJSON_AddItemToObject(data, "chip_version", chip_version);

    cJSON* cpu_freq = NULL;
    uint32_t cpu_freq_value;
    ESP_ERROR_CHECK(esp_clk_tree_src_get_freq_hz(SOC_MOD_CLK_CPU, ESP_CLK_TREE_SRC_FREQ_PRECISION_APPROX, &cpu_freq_value));

    cpu_freq = cJSON_CreateNumber(cpu_freq_value);
    if (cpu_freq == NULL) {
        goto get_device_info_end;
    }
    cJSON_AddItemToObject(data, "cpu_freq", cpu_freq);

    cJSON* idf_version = NULL;
    char idf_version_str[16];
    sprintf(idf_version_str, "%d.%d.%d", ESP_IDF_VERSION_MAJOR, ESP_IDF_VERSION_MINOR, ESP_IDF_VERSION_PATCH);
    idf_version = cJSON_CreateString(idf_version_str);
    if (idf_version == NULL) {
        goto get_device_info_end;
    }
    cJSON_AddItemToObject(data, "idf_version", idf_version);

    string = cJSON_Print(data);
    if (string == NULL) {
        ESP_LOGE(TAG, "string == NULL");
        goto get_device_info_end;
    }

get_device_info_end:
    cJSON_Delete(data);
    return string;
}

