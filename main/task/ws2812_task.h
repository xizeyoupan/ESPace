#ifndef __WS2812_TASK_H__
#define __WS2812_TASK_H__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

void ws2812_task(void* pvParameters);
extern TaskHandle_t ws2812_task_handle;

#ifdef __cplusplus
}
#endif

#endif