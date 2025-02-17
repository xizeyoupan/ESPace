#ifndef __BUTTON_H__
#define __BUTTON_H__

#include "user_config.h"

#define BUTTON0_GPIO_NUM GPIO_NUM_18
#define BUTTON1_GPIO_NUM GPIO_NUM_0

#ifdef __cplusplus
extern "C"
{
#endif

    void scan_button_task(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif