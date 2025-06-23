#ifndef __MPU_TASK_H__
#define __MPU_TASK_H__

#include "user_config.h"

#ifdef __cplusplus
extern "C" {
#endif

void mpu_task(void* pvParameters);

typedef enum {
    MPU_COMMAND_TYPE_GET_ANGLE,
    MPU_COMMAND_TYPE_GET_ROW,
    MPU_COMMAND_TYPE_RESET_IMU,
    MPU_COMMAND_TYPE_CLEAR,
} mpu_command_type_enum;

typedef enum {
    MODEL_TYPE_ONESHOT,
    MODEL_TYPE_PERIODIC,
} model_type_enum;

typedef struct
{
    char caller[32]; // 调用者
    mpu_command_type_enum type; // 命令类型
    model_type_enum model_type; // 模型类型
    uint32_t sample_size; // 采样大小
    uint32_t sample_tick; // 采样间隔
    uint8_t need_predict; // 是否需要预测
} mpu_command_t;

#ifdef __cplusplus
}
#endif

#endif