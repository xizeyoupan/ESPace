#ifndef __MPU_TASK_H__
#define __MPU_TASK_H__

#include "esp_bit_defs.h"

#include "stdint.h"

typedef enum {
    MODEL_TYPE_ONESHOT,
    MODEL_TYPE_PERIODIC,
} model_type_enum;

typedef enum {
    MPU_ACTION_IDLE,
    MPU_ACTION_SAMPLING,
} mpu_action_type_enum;

#pragma pack(push, 1)
typedef struct
{
    mpu_action_type_enum action;
    model_type_enum model_type; // 模型类型
    uint32_t sample_size; // 采样大小
    uint32_t sample_tick; // 采样间隔
    uint8_t send_to_ws;
    uint8_t need_predict; // 是否需要预测
} mpu_state_t;
#pragma pack(pop)

#define MPU_SAMPLING_START_BIT BIT2
#define MPU_SAMPLING_STOP_BIT BIT3

#ifdef __cplusplus
extern "C" {
#endif

void mpu_task(void* pvParameters);

// Sample offset management functions
void mpu_reset_sample_offset(void);
void mpu_set_sample_offset_start(void);

#ifdef __cplusplus
}
#endif

#endif