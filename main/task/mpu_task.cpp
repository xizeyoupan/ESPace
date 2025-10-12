/**
 * @file mpu_task.cpp
 * @brief ESP32手势识别的MPU（运动处理单元）任务实现
 *
 * 此文件实现MPU任务，负责处理手势识别的IMU数据采集。
 * 支持周期性采样和单次采样两种模式，管理数据缓冲区，
 * 并为TensorFlow Lite推理和WebSocket实时显示提供数据。
 */

#include "mpu_task.h"

#include "espace_define.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "stdint.h"
#include "string.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/message_buffer.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "service/mpu_service.h"
#include "service/ws2812_service.h"

#include "user_util.h"

static const char* TAG = "MPU_TASK";

// 外部依赖
extern user_config_t user_config;
extern MessageBufferHandle_t xMessageBufferReqSend;      // WebSocket通信的消息缓冲区
extern MessageBufferHandle_t xMessageBufferMPUOut2CNN;   // TensorFlow Lite输入的消息缓冲区
extern SemaphoreHandle_t mpu_mutex;                      // MPU硬件访问的互斥锁

// 全局状态变量
mpu_state_t mpu_state;                                   // 当前MPU采样状态和配置
EventGroupHandle_t x_mpu_event_group;                   // 任务同步的事件组

// 采样配置和索引
const int SAMPLE_START_OFFSET = 2;                      // 数据数组中元数据的偏移量（前2个float保留）
int sample_offset = 0;                                  // 当前样本写入位置

// 原始传感器数据存储
static double ax, ay, az, gx, gy, gz;                   // 原始IMU传感器值（双精度）
static float ax_f, ay_f, az_f, gx_f, gy_f, gz_f;        // 转换为float的IMU传感器值

// 数据缓冲区
float* imu_data = NULL;                                 // 周期采样模式的缓冲区
float* imu_data_oneshot = NULL;                         // 单次采样模式的缓冲区
int total_bytes = 0;                                    // imu_data缓冲区的总字节数

static char err_json[256];                              // JSON错误消息的缓冲区

/**
 * @brief 重置采样偏移量为零
 *
 * 将采样写入位置重置到数据缓冲区的开头。
 * 在采样变为未就绪状态或发生错误时使用。
 */
void mpu_reset_sample_offset(void)
{
    sample_offset = 0;
}

/**
 * @brief 设置采样偏移量为开始位置
 *
 * 将采样写入位置设置为SAMPLE_START_OFFSET，为元数据保留前2个float空间。
 */
void mpu_set_sample_offset_start(void)
{
    sample_offset = SAMPLE_START_OFFSET;
}

/**
 * @brief 发送IMU数据到WebSocket进行实时可视化
 *
 * 将收集的IMU数据发送到WebSocket消息缓冲区，在Web界面上实时显示。
 * 发送前设置元数据头部信息。
 */
static void send_imu_data_to_ws()
{
    // 设置元数据头部
    imu_data[0] = 0;                        // 消息类型标识符
    imu_data[1] = mpu_state.sample_size;    // 采集的样本数量

    size_t bytes_sent = xMessageBufferSend(xMessageBufferReqSend, imu_data, total_bytes, portMAX_DELAY);
    if (bytes_sent != total_bytes) {
        ESP_LOGE(TAG, "发送imu_data到ws缓冲区失败，发送 %d 字节，期望 %d 字节。", bytes_sent, total_bytes);
    } else {
        ESP_LOGI(TAG, "成功发送imu数据到ws缓冲区，大小: %d 字节", total_bytes);
    }
}

/**
 * @brief 处理并发送IMU数据到TensorFlow Lite进行手势识别
 *
 * 规范化IMU数据用于神经网络输入，并通过消息缓冲区发送到TFLite任务。
 * 为加速度计和陀螺仪数据应用不同的缩放因子以匹配模型训练要求。
 */
static void send_imu_data_to_tflite()
{
    // 规范化传感器数据用于神经网络输入
    // 加速度计：除以4（为模型调整范围）
    // 陀螺仪：除以500（为模型调整范围）
    for (size_t i = 0; i < mpu_state.sample_size; i++) {
        imu_data[SAMPLE_START_OFFSET + 0 * mpu_state.sample_size + i] /= 4;  // 加速度计X轴
        imu_data[SAMPLE_START_OFFSET + 1 * mpu_state.sample_size + i] /= 4;  // 加速度计Y轴
        imu_data[SAMPLE_START_OFFSET + 2 * mpu_state.sample_size + i] /= 4;  // 加速度计Z轴
        imu_data[SAMPLE_START_OFFSET + 3 * mpu_state.sample_size + i] /= 500; // 陀螺仪X轴
        imu_data[SAMPLE_START_OFFSET + 4 * mpu_state.sample_size + i] /= 500; // 陀螺仪Y轴
        imu_data[SAMPLE_START_OFFSET + 5 * mpu_state.sample_size + i] /= 500; // 陀螺仪Z轴
    }

    // 发送数据到TensorFlow Lite任务（跳过元数据头部）
    size_t data_size = mpu_state.sample_size * 6 * sizeof(float);
    size_t bytes_sent = xMessageBufferSend(xMessageBufferMPUOut2CNN, imu_data + 2, data_size, portMAX_DELAY);
    if (bytes_sent != data_size) {
        ESP_LOGE(TAG, "发送imu数据到tflite失败，发送 %d 字节，期望 %d 字节。", bytes_sent, data_size);
    } else {
        ESP_LOGI(TAG, "成功发送imu数据到tflite，大小: %d 字节", bytes_sent);
    }
}

/**
 * @brief IMU数据采集的周期性定时器回调函数
 *
 * 此函数以固定间隔调用，用于采集IMU传感器数据。
 * 处理周期性（连续）采样和单次采样两种模式。
 *
 * @param arg 定时器参数（未使用）
 */
static void mpu_periodic_timer_callback(void* arg)
{
    // 验证前置条件
    if (imu_data == NULL) {
        ESP_LOGE(TAG, "imu_data为NULL，无法执行周期性回调。");
        return;
    } else if (mpu_state.sample_size == 0 || mpu_state.sample_tick == 0) {
        ESP_LOGE(TAG, "无效的样本大小或时间间隔，无法执行周期性回调。");
        return;
    }

    // 使用互斥锁保护读取传感器数据
    xSemaphoreTake(mpu_mutex, pdMS_TO_TICKS(mpu_state.sample_tick) || portMAX_DELAY);
    _getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    xSemaphoreGive(mpu_mutex);

    // 转换为float以保持一致性
    ax_f = ax;
    ay_f = ay;
    az_f = az;
    gx_f = gx;
    gy_f = gy;
    gz_f = gz;

      // 根据采样模式处理数据
    if (mpu_state.model_type == MODEL_TYPE_PERIODIC) {
        // 周期采样模式：连续采集数据，循环缓冲区
        imu_data[0 * mpu_state.sample_size + sample_offset] = ax_f;  // 加速度计X轴
        imu_data[1 * mpu_state.sample_size + sample_offset] = ay_f;  // 加速度计Y轴
        imu_data[2 * mpu_state.sample_size + sample_offset] = az_f;  // 加速度计Z轴
        imu_data[3 * mpu_state.sample_size + sample_offset] = gx_f;  // 陀螺仪X轴
        imu_data[4 * mpu_state.sample_size + sample_offset] = gy_f;  // 陀螺仪Y轴
        imu_data[5 * mpu_state.sample_size + sample_offset] = gz_f;  // 陀螺仪Z轴
        sample_offset++;

        // 检查是否达到最大样本数，实现循环缓冲区
        if (sample_offset >= mpu_state.sample_size + SAMPLE_START_OFFSET) {
            mpu_set_sample_offset_start();  // 重置到开始位置，保留元数据空间
            if (mpu_state.send_to_ws) {
                send_imu_data_to_ws();      // 发送数据到WebSocket
            }
        }

    } else if (mpu_state.model_type == MODEL_TYPE_ONESHOT) {
        // 单次采样模式：采集一次完整手势数据
        if (sample_offset >= user_config.mpu_one_shot_max_sample_size + SAMPLE_START_OFFSET) {
            ESP_LOGE(TAG, "达到最大单次采样大小，退出。");
            mpu_reset_sample_offset();
            xEventGroupSetBits(x_mpu_event_group, MPU_SAMPLING_STOP_BIT);
            strcpy(err_json, "{ \"type\": \"error\", \"requestId\": \"req_error\", \"error\": \"error.reached_maximum_oneshot_sample_size\" }");
            xMessageBufferSend(xMessageBufferReqSend, err_json, strlen(err_json), portMAX_DELAY);
            return;
        }

        // 存储单次采样数据
        imu_data_oneshot[0 * user_config.mpu_one_shot_max_sample_size + sample_offset] = ax_f;  // 加速度计X轴
        imu_data_oneshot[1 * user_config.mpu_one_shot_max_sample_size + sample_offset] = ay_f;  // 加速度计Y轴
        imu_data_oneshot[2 * user_config.mpu_one_shot_max_sample_size + sample_offset] = az_f;  // 加速度计Z轴
        imu_data_oneshot[3 * user_config.mpu_one_shot_max_sample_size + sample_offset] = gx_f;  // 陀螺仪X轴
        imu_data_oneshot[4 * user_config.mpu_one_shot_max_sample_size + sample_offset] = gy_f;  // 陀螺仪Y轴
        imu_data_oneshot[5 * user_config.mpu_one_shot_max_sample_size + sample_offset] = gz_f;  // 陀螺仪Z轴
        sample_offset++;
    }
}

/**
 * @brief MPU任务主函数
 *
 * 负责IMU数据采集的核心任务，支持周期性采样和单次采样两种模式。
 * 管理定时器、内存分配、数据发送等功能。
 *
 * @param pvParameters 任务参数（未使用）
 */
void mpu_task(void* pvParameters)
{
    BaseType_t core_id = xPortGetCoreID(); // 获取当前任务所在的核心ID
    ESP_LOGI(TAG, "MPU任务运行在核心 %d。", core_id);

    // 创建事件组用于任务间同步
    x_mpu_event_group = xEventGroupCreate();

    // 创建周期性定时器用于数据采集
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &mpu_periodic_timer_callback,
        .name = "mpu periodic"
    };
    esp_timer_handle_t mpu_periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &mpu_periodic_timer));

    // 初始化MPU硬件和互斥锁
    mpu_mutex_init();
    reset_imu();

    // 为单次采样模式分配内存缓冲区
    int imu_data_oneshot_size = user_config.mpu_one_shot_max_sample_size * 6 * sizeof(float) + 16;
    imu_data_oneshot = (float*)malloc(imu_data_oneshot_size);
    if (imu_data_oneshot == NULL) {
        ESP_LOGE(TAG, "为imu_data_oneshot分配内存失败，大小: %d 字节", imu_data_oneshot_size);
        vTaskDelete(NULL);
    }

      // 主任务循环：等待采样事件
    while (1) {
        EventBits_t uxBits = xEventGroupWaitBits(x_mpu_event_group, MPU_SAMPLING_START_BIT | MPU_SAMPLING_STOP_BIT, pdTRUE, pdFALSE, portMAX_DELAY);

        if (uxBits & MPU_SAMPLING_START_BIT) {
            // 开始采样事件
            ESP_LOGI(TAG, "触发开始采样事件。");
            if (mpu_state.model_type >= 0 && mpu_state.sample_size > 0 && mpu_state.sample_tick > 0) {
                // 验证采样参数有效性
                mpu_set_sample_offset_start();  // 设置采样偏移到开始位置

                // 释放旧的内存缓冲区
                if (imu_data != NULL) {
                    free(imu_data);
                    imu_data = NULL;
                }

                // 为周期采样分配新的内存缓冲区
                total_bytes = mpu_state.sample_size * 6 * sizeof(float) + SAMPLE_START_OFFSET * sizeof(float);
                imu_data = (float*)malloc(total_bytes);
                if (imu_data == NULL) {
                    ESP_LOGE(TAG, "为imu_data分配内存失败，大小: %d 字节", total_bytes);
                    continue;
                }

                // 启动周期性定时器开始数据采集
                ESP_ERROR_CHECK(esp_timer_start_periodic(mpu_periodic_timer, mpu_state.sample_tick * 1000));
                ESP_LOGI(TAG, "启动MPU周期性采样。");

            } else {
                ESP_LOGE(TAG, "无效或空的命令参数。");
            }

        } else if (uxBits & MPU_SAMPLING_STOP_BIT) {
            // 停止采样事件
            ESP_LOGI(TAG, "触发停止采样事件。");

            // 停止定时器
            if (esp_timer_is_active(mpu_periodic_timer)) {
                ESP_ERROR_CHECK(esp_timer_stop(mpu_periodic_timer));
            } else {
                ESP_LOGI(TAG, "MPU周期性定时器未运行。");
            }

            // 处理单次采样模式的数据后处理
            if (mpu_state.model_type == MODEL_TYPE_ONESHOT && sample_offset > SAMPLE_START_OFFSET) {
                int real_size = sample_offset - SAMPLE_START_OFFSET;
                ESP_LOGI(TAG, "完成单次数据采样，实际样本大小: %d, 预期:%lu",
                    real_size, mpu_state.sample_size);

                // 使用线性插值将采集的数据调整到模型期望的样本大小
                for (int i = 0; i < 6; i++) {
                    linear_interpolation(i * user_config.mpu_one_shot_max_sample_size + imu_data_oneshot + SAMPLE_START_OFFSET, real_size,
                        i * mpu_state.sample_size + imu_data + SAMPLE_START_OFFSET, mpu_state.sample_size);
                }

                // 根据配置发送数据
                if (mpu_state.send_to_ws) {
                    send_imu_data_to_ws();     // 发送到WebSocket
                }

                if (mpu_state.need_predict) {
                    send_imu_data_to_tflite(); // 发送到TensorFlow Lite进行预测
                }
            }
        }
    }

    vTaskDelete(NULL);
}