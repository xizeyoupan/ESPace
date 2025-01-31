#ifndef __WS2812_H__
#define __WS2812_H__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/rmt_tx.h"
#include "user_config.h"

#ifdef __cplusplus
extern "C"
{
#endif

    void WS2812_ControllerTask(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif