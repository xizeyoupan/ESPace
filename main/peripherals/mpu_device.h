#ifndef __MPU6500_H__
#define __MPU6500_H__

#include "user_config.h"

typedef struct
{
    float quatx;
    float quaty;
    float quatz;
    float quatw;
    float roll;
    float pitch;
    float yaw;
} POSE_t;

#ifdef __cplusplus
extern "C"
{
#endif
    void mpu6050(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif