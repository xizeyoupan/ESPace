#ifndef __MPU_SERVICE_H__
#define __MPU_SERVICE_H__

#include "stdint.h"
#include "task/mpu_task.h"

#ifdef __cplusplus
extern "C" {
#endif

void reset_imu();
void mpu_mutex_init();
void _getMotion6(double* _ax, double* _ay, double* _az, double* _gx, double* _gy, double* _gz);
void get_angle_and_m6(double* _roll, double* _pitch, double* _ax, double* _ay, double* _az, double* _gx, double* _gy, double* _gz);
void linear_interpolation(const float* input, uint32_t input_size, float* output, uint32_t output_size);
void change_mpu_state(mpu_state_t* mpu_state, mpu_action_type_enum action, model_type_enum model_type, uint32_t sample_size, uint32_t sample_tick, uint8_t need_predict, uint8_t send_to_ws);
#ifdef __cplusplus
}
#endif

#endif