#ifndef __MPU_SERVICE_H__
#define __MPU_SERVICE_H__

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

void reset_imu();
void mpu_mutex_init();
void _getMotion6(double* _ax, double* _ay, double* _az, double* _gx, double* _gy, double* _gz);
void get_angle_and_m6(double* _roll, double* _pitch, double* _ax, double* _ay, double* _az, double* _gx, double* _gy, double* _gz);
void linear_interpolation(const float* input, uint32_t input_size, float* output, uint32_t output_size);

#ifdef __cplusplus
}
#endif

#endif