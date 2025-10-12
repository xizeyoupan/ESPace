#ifndef __BUTTON_H__
#define __BUTTON_H__

#include "espace_define.h"

#ifdef __cplusplus
extern "C" {
#endif

void scan_button_task(void* pvParameters);

uint8_t read_button_GPIO(uint8_t button_id);

// 按钮动作处理函数
void handle_button_action(uint8_t button_id, uint8_t button_event);

#ifdef __cplusplus
}
#endif

#endif