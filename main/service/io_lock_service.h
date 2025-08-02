#ifndef __IO_LOCK_SERVICE_H__
#define __IO_LOCK_SERVICE_H__

#include "user_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IO_MUTEX_ARRAY_SIZE 40
#define HOLDER_STRING_SIZE 32

#define INVALID_HOLDER "INVALID_HOLDER"
#define PSRAM_HOLDER "PSRAM"
#define FLASH_HOLDER "FLASH"
#define UPKEY_HOLDER "UP_KEY"
#define DOWNKEY_HOLDER "DOWN_KEY"
#define MPU_SDA_HOLDER "MPU_SDA"
#define MPU_SCL_HOLDER "MPU_SCL"
#define WS2812_HOLDER "WS2812"
#define PERIPH_PWR_HOLDER "PERIPH_PWR"
#define I2S_BCK_HOLDER "I2S_BCK"
#define I2S_WS_HOLDER "I2S_WS"
#define I2S_DOUT_HOLDER "I2S_DOUT"
#define IR_RX_HOLDER "IR_RX"
#define IR_TX_HOLDER "IR_TX"
#define USB_BRIDGE_HOLDER "USB_BRIDGE"
#define LEDC_PWM_HOLDER "LEDC_PWM"
#define DAC_COSINE_HOLDER "DAC_COSINE"

typedef struct {
    SemaphoreHandle_t mutex; // 实际的资源锁
    SemaphoreHandle_t meta_lock; // 保护 holder 的轻量互斥锁
    char holder[HOLDER_STRING_SIZE];
    int channel; // 资源锁的通道
} io_mutex_t;

void io_mutex_init(void);
BaseType_t io_mutex_lock(int index, const char* holder, int channel, TickType_t timeout_ticks);
BaseType_t io_mutex_unlock(int index, const char* holder, int channel);
void io_mutex_get_status(int index, char* holder, int* channel);

#ifdef __cplusplus
}
#endif

#endif