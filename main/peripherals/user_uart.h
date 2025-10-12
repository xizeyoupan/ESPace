#ifndef __USER_UART_H__
#define __USER_UART_H__

#include "esp_err.h"
#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

void uart0_init();
void test_uart(void* pvParameters);

#ifdef __cplusplus
}
#endif

#endif