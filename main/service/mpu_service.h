#ifndef __MPU_SERVICE_H__
#define __MPU_SERVICE_H__

#include "user_config.h"

#ifdef __cplusplus
extern "C" {
#endif
void mpu6050(void* pvParameters);
void reset_imu();
void mpu_mutex_init();
void _getMotion6(double* _ax, double* _ay, double* _az, double* _gx, double* _gy, double* _gz);
void get_angle_and_m6(double* _roll, double* _pitch, double* _ax, double* _ay, double* _az, double* _gx, double* _gy, double* _gz);

#ifdef __cplusplus
}
#endif

#endif