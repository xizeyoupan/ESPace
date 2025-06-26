#ifndef __TFLITE_SERVICE_H__
#define __TFLITE_SERVICE_H__

#include "user_config.h"

#ifdef __cplusplus
extern "C" {
#endif

TfLiteStatus tflite_init();
TfLiteStatus load_and_check_model(int* input_size, int* output_size);
TfLiteStatus tflite_invoke(float* input_buf, size_t data_size);

#ifdef __cplusplus
}
#endif

#endif