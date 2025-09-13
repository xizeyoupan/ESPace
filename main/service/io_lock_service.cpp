#include "io_lock_service.h"

#include "espace_define.h"

#include "esp_err.h"
#include "esp_log.h"

#include "stdint.h"
#include "string.h"

static const char* TAG = "IO_LOCK_SERVICE";
extern user_config_t user_config;

io_mutex_t io_mutex_array[IO_MUTEX_ARRAY_SIZE];

void io_mutex_init(void)
{
    for (int i = 0; i < IO_MUTEX_ARRAY_SIZE; ++i) {
        io_mutex_array[i].mutex = xSemaphoreCreateMutex();
        io_mutex_array[i].meta_lock = xSemaphoreCreateMutex();
        io_mutex_array[i].holder[0] = '\0'; // 初始化为空字符串
        io_mutex_array[i].channel = -1; // 初始值为 -1
    }
    // gpio20、gpio24、gpio28、gpio29、gpio30、gpio31 不能作为 IO 通道
    io_mutex_lock(20, INVALID_HOLDER, -1, portMAX_DELAY);
    io_mutex_lock(24, INVALID_HOLDER, -1, portMAX_DELAY);
    io_mutex_lock(28, INVALID_HOLDER, -1, portMAX_DELAY);
    io_mutex_lock(29, INVALID_HOLDER, -1, portMAX_DELAY);
    io_mutex_lock(30, INVALID_HOLDER, -1, portMAX_DELAY);
    io_mutex_lock(31, INVALID_HOLDER, -1, portMAX_DELAY);

    // 1, 3: usb bridge
    io_mutex_lock(1, USB_BRIDGE_HOLDER, -1, portMAX_DELAY);
    io_mutex_lock(3, USB_BRIDGE_HOLDER, -1, portMAX_DELAY);

    /*
    6-11: flash, 16-17: psram
    */
    io_mutex_lock(6, FLASH_HOLDER, -1, portMAX_DELAY);
    io_mutex_lock(7, FLASH_HOLDER, -1, portMAX_DELAY);
    io_mutex_lock(8, FLASH_HOLDER, -1, portMAX_DELAY);
    io_mutex_lock(9, FLASH_HOLDER, -1, portMAX_DELAY);
    io_mutex_lock(10, FLASH_HOLDER, -1, portMAX_DELAY);
    io_mutex_lock(11, FLASH_HOLDER, -1, portMAX_DELAY);
    io_mutex_lock(16, PSRAM_HOLDER, -1, portMAX_DELAY);
    io_mutex_lock(17, PSRAM_HOLDER, -1, portMAX_DELAY);

    io_mutex_lock(user_config.up_key_gpio_num, UPKEY_HOLDER, 0, portMAX_DELAY);
    io_mutex_lock(user_config.down_key_gpio_num, DOWNKEY_HOLDER, 0, portMAX_DELAY);
    io_mutex_lock(user_config.mpu_sda_gpio_num, MPU_SDA_HOLDER, 0, portMAX_DELAY);
    io_mutex_lock(user_config.mpu_scl_gpio_num, MPU_SCL_HOLDER, 0, portMAX_DELAY);
    io_mutex_lock(user_config.ws2812_gpio_num, WS2812_HOLDER, 0, portMAX_DELAY);
    io_mutex_lock(user_config.periph_pwr_gpio_num, PERIPH_PWR_HOLDER, 0, portMAX_DELAY);
    io_mutex_lock(user_config.i2s_bck_gpio_num, I2S_BCK_HOLDER, 0, portMAX_DELAY);
    io_mutex_lock(user_config.i2s_ws_gpio_num, I2S_WS_HOLDER, 0, portMAX_DELAY);
    io_mutex_lock(user_config.i2s_dout_gpio_num, I2S_DOUT_HOLDER, 0, portMAX_DELAY);
    io_mutex_lock(user_config.ir_rx_gpio_num, IR_RX_HOLDER, 0, portMAX_DELAY);
    io_mutex_lock(user_config.ir_tx_gpio_num, IR_TX_HOLDER, 0, portMAX_DELAY);
    ESP_LOGI(TAG, "IO mutex initialized.");
}

BaseType_t io_mutex_lock(int index, const char* holder, int channel, TickType_t timeout_ticks)
{
    if (index < 0 || index >= IO_MUTEX_ARRAY_SIZE)
        return pdFALSE;

    // 获取资源锁
    if (xSemaphoreTake(io_mutex_array[index].mutex, timeout_ticks) == pdTRUE) {
        // 获取 meta_lock 来保护 holder  和 channel
        if (xSemaphoreTake(io_mutex_array[index].meta_lock, portMAX_DELAY) == pdTRUE) {
            // 更新持有者的模块ID和通道信息
            strncpy(io_mutex_array[index].holder, holder, sizeof(io_mutex_array[index].holder) - 1);
            io_mutex_array[index].holder[sizeof(io_mutex_array[index].holder) - 1] = '\0';
            io_mutex_array[index].channel = channel;
            xSemaphoreGive(io_mutex_array[index].meta_lock);
        }
        return pdTRUE;
    }
    return pdFALSE;
}

BaseType_t io_mutex_unlock(int index, const char* holder, int channel)
{
    if (index < 0 || index >= IO_MUTEX_ARRAY_SIZE)
        return pdFALSE;

    BaseType_t success = pdFALSE;

    // 获取 meta_lock 来检查持有者是否匹配
    if (xSemaphoreTake(io_mutex_array[index].meta_lock, portMAX_DELAY) == pdTRUE) {
        if (strncmp(io_mutex_array[index].holder, holder, sizeof(io_mutex_array[index].holder)) == 0 && io_mutex_array[index].channel == channel) {
            // 如果是当前模块ID，释放锁
            io_mutex_array[index].holder[0] = '\0'; // 清空模块ID
            io_mutex_array[index].channel = -1; // 清空通道信息
            success = pdTRUE;
        } else {
            ESP_LOGE(TAG, "Unlock failed: holder mismatch. Expected holder %s, channel %d, got holder %s, channel %d", io_mutex_array[index].holder, io_mutex_array[index].channel, holder, channel);
        }
        xSemaphoreGive(io_mutex_array[index].meta_lock);
    }

    if (success) {
        // 解锁资源
        xSemaphoreGive(io_mutex_array[index].mutex);
    }

    ESP_LOGI(TAG, "IO mutex unlocked for gpio %d by holder %s on channel %d", index, holder, channel);
    return success;
}

void io_mutex_get_status(int index, char* holder, int* channel)
{
    if (index < 0 || index >= IO_MUTEX_ARRAY_SIZE)
        return;

    // 获取 meta_lock 来读取状态
    if (xSemaphoreTake(io_mutex_array[index].meta_lock, portMAX_DELAY) == pdTRUE) {
        strncpy(holder, io_mutex_array[index].holder, HOLDER_STRING_SIZE);
        *channel = io_mutex_array[index].channel;
        xSemaphoreGive(io_mutex_array[index].meta_lock);
    }
}

void try_to_lock_io(int gpio_num, const char* holder, int channel, user_def_err_t* user_err)
{
    char io_holder[HOLDER_STRING_SIZE] = { 0 };
    int io_holder_channel;

    io_mutex_get_status(gpio_num, io_holder, &io_holder_channel);

    if (strlen(io_holder) == 0) {
        if (io_mutex_lock(gpio_num, holder, channel, portMAX_DELAY) != pdTRUE) {
            if (user_err) {
                snprintf(user_err->err_msg, USER_DEF_ERR_MSG_LEN,
                    "Failed to lock GPIO %d for %s", gpio_num, holder);
                user_err->esp_err = ESP_ERR_INVALID_STATE;
                ESP_LOGE(TAG, "%s", user_err->err_msg);
            }
        } else {
            if (user_err) {
                user_err->esp_err = ESP_OK;
            }
        }
    } else if (strcmp(io_holder, holder) != 0 || io_holder_channel != channel) {
        if (user_err) {
            snprintf(user_err->err_msg, USER_DEF_ERR_MSG_LEN,
                "GPIO %d is already locked by %s on channel %d",
                gpio_num, io_holder, io_holder_channel);
            user_err->esp_err = ESP_ERR_INVALID_STATE;
            ESP_LOGE(TAG, "%s", user_err->err_msg);
        }
    }
}
