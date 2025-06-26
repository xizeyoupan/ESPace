
#ifndef __CNN_H__
#define __CNN_H__

#include "user_config.h"

#ifdef __cplusplus
extern "C" {
#endif

void CNN_task(void* pvParameters);
TfLiteStatus tflite_init();
TfLiteStatus load_and_check_model(const void* buf, int* input_size, int* output_size);

#ifdef __cplusplus
}
#endif

#endif