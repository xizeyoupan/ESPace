#ifndef __MPU6500_H__
#define __MPU6500_H__

#include "user_config.h"

#ifdef __cplusplus
extern "C"
{
#endif
    void mpu6050(void *pvParameters);
    void reset_imu();

#ifdef __cplusplus
}
#endif

#endif