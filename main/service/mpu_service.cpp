/* ============================================
I2Cdev device library code is placed under the MIT license
Copyright (c) 2012 Jeff Rowberg
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
===============================================
*/

/*
https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map1.pdf
->
https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6500-Register-Map2.pdf
*/

#include "mpu_service.h"

// I2Cdev and MPU6050 must be installed as libraries, or else the .cpp/.h files
// for both classes must be in the include path of your project
#include "I2Cdev.h"
#include "MPU6050.h"

// Source: https://github.com/TKJElectronics/KalmanFilter
#include "Kalman.h"

static const char *TAG = "IMU";
#define RESTRICT_PITCH // Comment out to restrict roll to ±90deg instead
#define RAD_TO_DEG (180.0 / M_PI)
#define DEG_TO_RAD 0.0174533

extern MessageBufferHandle_t xMessageBufferToClient;
extern user_config_t user_config;
MPU6050 mpu;
Kalman kalmanX; // Create the Kalman instances
Kalman kalmanY;
SemaphoreHandle_t imu_mutex;

// Accel & Gyro scale factor
const double accel_sensitivity = 8192.0;
const double gyro_sensitivity = 65.536;
const int MAX_OUTPUT_SIZE = 500;
const int MAX_INPUT_SIZE = 800;
uint8_t imu_input_data[MAX_INPUT_SIZE * 24 + 128];
uint8_t imu_output_data[MAX_OUTPUT_SIZE * 24 + 128];

void linear_interpolation(uint8_t *input, uint32_t input_size, uint8_t *output, uint32_t output_size)
{
    for (uint32_t i = 0; i < output_size; i++)
    {
        float pos = (float)i * (input_size - 1) / (output_size - 1);
        uint32_t index = (uint32_t)pos;
        float weight = pos - index;

        float data0, data1;
        if (index >= input_size - 1)
        {
            memcpy((void *)&data0, input + (input_size - 1) * sizeof(float), sizeof(float));
            memcpy(output + i * sizeof(float), (void *)&data0, sizeof(float));
        }
        else
        {
            memcpy((void *)&data0, input + index * sizeof(float), sizeof(float));
            data0 = data0 * (1 - weight);
            memcpy((void *)&data1, input + (index + 1) * sizeof(float), sizeof(float));
            data1 = data1 * weight;
            data0 = data0 + data1;
            memcpy(output + i * sizeof(float), (void *)&data0, sizeof(float));
        }
    }
}

// Get scaled value
void _getMotion6(double *_ax, double *_ay, double *_az, double *_gx, double *_gy, double *_gz)
{
    int16_t ax, ay, az;
    int16_t gx, gy, gz;
    // read raw accel/gyro measurements from device
    // The accelerometer output is a 16-bit signed integer relative value.
    // The gyroscope output is a relative value in degrees per second (dps).
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    // Convert relative to absolute
    *_ax = (double)ax / accel_sensitivity;
    *_ay = (double)ay / accel_sensitivity;
    *_az = (double)az / accel_sensitivity;

    // Convert relative to absolute
    *_gx = (double)gx / gyro_sensitivity;
    *_gy = (double)gy / gyro_sensitivity;
    *_gz = (double)gz / gyro_sensitivity;
}

void getRollPitch(double accX, double accY, double accZ, double *roll, double *pitch)
{
    // atan2 outputs the value of -πto π(radians) - see http://en.wikipedia.org/wiki/Atan2
    // It is then converted from radians to degrees
#ifdef RESTRICT_PITCH // Eq. 25 and 26
    *roll = atan2(accY, accZ) * RAD_TO_DEG;
    *pitch = atan(-accX / sqrt(accY * accY + accZ * accZ)) * RAD_TO_DEG;
#else // Eq. 28 and 29
    *roll = atan(accY / sqrt(accX * accX + accZ * accZ)) * RAD_TO_DEG;
    *pitch = atan2(-accX, accZ) * RAD_TO_DEG;
#endif
}

// Set Kalman and gyro starting angle
double ax, ay, az;
double gx, gy, gz;
double roll, pitch;          // Roll and pitch are calculated using the accelerometer
double kalAngleX, kalAngleY; // Calculated angle using a Kalman filter

int64_t timer;

int8_t initialized = 0;
double initial_kalAngleX = 0.0;
double initial_kalAngleY = 0.0;

void start_i2c(void)
{
    i2c_driver_delete(I2C_NUM_0);
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = user_config.mpu_sda_gpio_num;
    conf.scl_io_num = user_config.mpu_scl_gpio_num;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 400000;
    conf.clk_flags = 0;
    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0));
}

extern "C" void reset_imu()
{
    xSemaphoreTake(imu_mutex, portMAX_DELAY);

    start_i2c();

    // Initialize mpu6050
    mpu.initialize();
    mpu.reset();
    vTaskDelay(pdMS_TO_TICKS(10));
    mpu.initialize();

    // Get DeviceID
    uint8_t devid = mpu.getDeviceID();
    ESP_LOGI(TAG, "devid=0x%x", devid);

    // Set the sample rate div=1
    if (mpu.getRate() != 0)
        mpu.setRate(0);
    // Get the sample rate
    ESP_LOGI(TAG, "sample rate div getRate()=%d", mpu.getRate());

    // Disable FSYNC
    if (mpu.getExternalFrameSync() != 0)
        mpu.setExternalFrameSync(0);
    // Get FSYNC configuration value
    ESP_LOGI(TAG, "getExternalFrameSync()=%d", mpu.getExternalFrameSync());

    // Set Accelerometer Full Scale Range to ±4g
    if (mpu.getFullScaleAccelRange() != 1)
        mpu.setFullScaleAccelRange(1); // -4 --> +4[g]

    // Get Accelerometer Scale Range
    ESP_LOGI(TAG, "getFullScaleAccelRange()=%d", mpu.getFullScaleAccelRange());

    // Set Gyro Full Scale Range to ±500deg/s
    if (mpu.getFullScaleGyroRange() != 1)
        mpu.setFullScaleGyroRange(1); // -500 --> +500[Deg/Sec]

    // Get Gyro Scale Range
    ESP_LOGI(TAG, "getFullScaleGyroRange()=%d", mpu.getFullScaleGyroRange());

    // Set Digital Low Pass Filter, default 460 Hz Acc filtering, 250 Hz Gyro filtering
    if (mpu.getDLPFMode() != 0)
        mpu.setDLPFMode(0);
    ESP_LOGI(TAG, "getDLPFMode()=%d", mpu.getDLPFMode());

    _getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    getRollPitch(ax, ay, az, &roll, &pitch);
    kalAngleX = roll;
    kalAngleY = pitch;
    kalmanX.setAngle(roll); // Set starting angle
    kalmanY.setAngle(pitch);

    timer = esp_timer_get_time();

    initialized = 0;
    initial_kalAngleX = 0.0;
    initial_kalAngleY = 0.0;

    xSemaphoreGive(imu_mutex);
}

float _roll;
float _pitch;

void get_angle()
{
    getRollPitch(ax, ay, az, &roll, &pitch);

    double dt = (double)(esp_timer_get_time() - timer) / 1000000; // Calculate delta time
    timer = esp_timer_get_time();

    /* Roll and pitch estimation */
    double gyroXrate = gx;
    double gyroYrate = gy;

#ifdef RESTRICT_PITCH
    // This fixes the transition problem when the accelerometer angle jumps between -180 and 180 degrees
    if ((roll < -90 && kalAngleX > 90) || (roll > 90 && kalAngleX < -90))
    {
        kalmanX.setAngle(roll);
        kalAngleX = roll;
    }
    else
        kalAngleX = kalmanX.getAngle(roll, gyroXrate, dt); // Calculate the angle using a Kalman filter

    if (abs(kalAngleX) > 90)
        gyroYrate = -gyroYrate; // Invert rate, so it fits the restriced accelerometer reading
    kalAngleY = kalmanY.getAngle(pitch, gyroYrate, dt);
#else
    // This fixes the transition problem when the accelerometer angle jumps between -180 and 180 degrees
    if ((pitch < -90 && kalAngleY > 90) || (pitch > 90 && kalAngleY < -90))
    {
        kalmanY.setAngle(pitch);
        kalAngleY = pitch;
    }
    else
        kalAngleY = kalmanY.getAngle(pitch, gyroYrate, dt); // Calculate the angle using a Kalman filter

    if (abs(kalAngleY) > 90)
        gyroXrate = -gyroXrate;                        // Invert rate, so it fits the restriced accelerometer reading
    kalAngleX = kalmanX.getAngle(roll, gyroXrate, dt); // Calculate the angle using a Kalman filter
#endif

    // Set the first data
    if (!initialized)
    {
        initial_kalAngleX = kalAngleX;
        initial_kalAngleY = kalAngleY;
        initialized = 1;
    }

#if 0
        printf("roll:%f", roll); printf(" ");
        printf("kalAngleX:%f", kalAngleX); printf(" ");
        printf("initial_kalAngleX:%f", initial_kalAngleX); printf(" ");
        printf("kalAngleX-initial_kalAngleX:%f", kalAngleX-initial_kalAngleX); printf(" ");
        printf("\n");

        printf("pitch:%f", pitch); printf(" ");
        printf("kalAngleY:%f", kalAngleY); printf(" ");
        printf("initial_kalAngleY:%f", initial_kalAngleY); printf(" ");
        printf("kalAngleY-initial_kalAngleY: %f", kalAngleY-initial_kalAngleY); printf(" ");
        printf("\n");
#endif

    _roll = kalAngleX - initial_kalAngleX;
    _pitch = kalAngleY - initial_kalAngleY;
    // ESP_LOGI(TAG, "roll:%f pitch=%f", _roll, _pitch);
}

extern EventGroupHandle_t button_event_group;
extern model_t current_model;
extern "C" void mpu6050(void *pvParameters)
{
    BaseType_t core_id = xPortGetCoreID(); // 返回当前任务所在的核心 ID
    ESP_LOGI(TAG, "Task is running on core %d.", core_id);

    imu_mutex = xSemaphoreCreateMutex();
    configASSERT(imu_mutex);

    reset_imu();
    get_angle();

    float ax_f = ax;
    float ay_f = ay;
    float az_f = az;
    float gx_f = gx;
    float gy_f = gy;
    float gz_f = gz;

    while (1)
    {
        if (user_config.enable_imu_det)
        {
            xSemaphoreTake(imu_mutex, portMAX_DELAY);
            _getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
            xSemaphoreGive(imu_mutex);
            // ESP_LOGI(TAG, "mpu data: %f %f %f - %f %f %f", ax, ay, az, gx, gy, gz);

            get_angle();
            ax_f = ax;
            ay_f = ay;
            az_f = az;
            gx_f = gx;
            gy_f = gy;
            gz_f = gz;

            memset(imu_input_data, 0, sizeof(imu_input_data));
            imu_input_data[0] = SEND_WS_IMU_DATA_PREFIX;
            memcpy(imu_input_data + 1, (void *)&_roll, sizeof(_roll));
            memcpy(imu_input_data + 5, (void *)&_pitch, sizeof(_pitch));
            memcpy(imu_input_data + 9, (void *)&ax_f, sizeof(ax_f));
            memcpy(imu_input_data + 13, (void *)&ay_f, sizeof(ay_f));
            memcpy(imu_input_data + 17, (void *)&az_f, sizeof(az_f));
            memcpy(imu_input_data + 21, (void *)&gx_f, sizeof(gx_f));
            memcpy(imu_input_data + 25, (void *)&gy_f, sizeof(gy_f));
            memcpy(imu_input_data + 29, (void *)&gz_f, sizeof(gz_f));
            xMessageBufferSend(xMessageBufferToClient, imu_input_data, 33, 0);
            vTaskDelay(pdMS_TO_TICKS(125));
        }
        else if (current_model.id == MODEL_DATASET_ID)
        {
            EventBits_t uxReturn;
            uint32_t tick_size = 0;
            const uint32_t start_offset = 5;
            TickType_t xLastWakeTime = xTaskGetTickCount();

            uxReturn = xEventGroupWaitBits(button_event_group, BTN0_DOWN_BIT | BTN1_DOWN_BIT, pdFALSE, pdFALSE, 0);
            if ((uxReturn & BTN0_DOWN_BIT) && current_model.type == CONTIOUS_MODEL)
            {
                // 持续模型
                tick_size = 0;
                while ((uxReturn & BTN0_DOWN_BIT) && current_model.type == CONTIOUS_MODEL)
                {
                    uxReturn = xEventGroupWaitBits(button_event_group, BTN0_DOWN_BIT, pdFALSE, pdFALSE, 0);
                    // ESP_LOGI(TAG, "get btn 0 honlding");

                    xSemaphoreTake(imu_mutex, portMAX_DELAY);
                    _getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
                    xSemaphoreGive(imu_mutex);
                    ax_f = ax;
                    ay_f = ay;
                    az_f = az;
                    gx_f = gx;
                    gy_f = gy;
                    gz_f = gz;

                    memcpy(imu_output_data + start_offset + (current_model.sample_size * 0 + tick_size) * sizeof(float), (void *)&ax_f, sizeof(ax_f));
                    memcpy(imu_output_data + start_offset + (current_model.sample_size * 1 + tick_size) * sizeof(float), (void *)&ay_f, sizeof(ay_f));
                    memcpy(imu_output_data + start_offset + (current_model.sample_size * 2 + tick_size) * sizeof(float), (void *)&az_f, sizeof(az_f));
                    memcpy(imu_output_data + start_offset + (current_model.sample_size * 3 + tick_size) * sizeof(float), (void *)&gx_f, sizeof(gx_f));
                    memcpy(imu_output_data + start_offset + (current_model.sample_size * 4 + tick_size) * sizeof(float), (void *)&gy_f, sizeof(gy_f));
                    memcpy(imu_output_data + start_offset + (current_model.sample_size * 5 + tick_size) * sizeof(float), (void *)&gz_f, sizeof(gz_f));

                    tick_size++;
                    if (tick_size >= current_model.sample_size)
                    {
                        imu_output_data[0] = SEND_IMU_DATASET_DATA_PREFIX;
                        memcpy(imu_output_data + 1, (void *)&tick_size, sizeof(tick_size));
                        xMessageBufferSend(xMessageBufferToClient, imu_output_data, tick_size * 24 + start_offset, portMAX_DELAY);
                        tick_size = 0;
                    }
                    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(current_model.sample_tick));
                }
                ESP_LOGI(TAG, "exit btn 0 honlding");
            }
            else if ((uxReturn & BTN1_DOWN_BIT) && current_model.type == COMMAND_MODEL)
            {
                // 指令模型
                tick_size = 0;
                while ((uxReturn & BTN1_DOWN_BIT) && current_model.type == COMMAND_MODEL)
                {
                    uxReturn = xEventGroupWaitBits(button_event_group, BTN1_DOWN_BIT, pdFALSE, pdFALSE, 0);
                    // ESP_LOGI(TAG, "get btn 1 honlding");

                    xSemaphoreTake(imu_mutex, portMAX_DELAY);
                    _getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
                    xSemaphoreGive(imu_mutex);
                    ax_f = ax;
                    ay_f = ay;
                    az_f = az;
                    gx_f = gx;
                    gy_f = gy;
                    gz_f = gz;

                    memcpy(imu_input_data + (MAX_INPUT_SIZE * 0 + tick_size) * sizeof(float), (void *)&ax_f, sizeof(ax_f));
                    memcpy(imu_input_data + (MAX_INPUT_SIZE * 1 + tick_size) * sizeof(float), (void *)&ay_f, sizeof(ay_f));
                    memcpy(imu_input_data + (MAX_INPUT_SIZE * 2 + tick_size) * sizeof(float), (void *)&az_f, sizeof(az_f));
                    memcpy(imu_input_data + (MAX_INPUT_SIZE * 3 + tick_size) * sizeof(float), (void *)&gx_f, sizeof(gx_f));
                    memcpy(imu_input_data + (MAX_INPUT_SIZE * 4 + tick_size) * sizeof(float), (void *)&gy_f, sizeof(gy_f));
                    memcpy(imu_input_data + (MAX_INPUT_SIZE * 5 + tick_size) * sizeof(float), (void *)&gz_f, sizeof(gz_f));

                    tick_size++;
                    if (tick_size >= MAX_INPUT_SIZE)
                    {
                        ws2812_blink(COLOR_RED);
                        break;
                    }
                    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(current_model.sample_tick));
                }

                linear_interpolation(imu_input_data + MAX_INPUT_SIZE * sizeof(float) * 0, tick_size, imu_output_data + start_offset + current_model.sample_size * sizeof(float) * 0, current_model.sample_size);
                linear_interpolation(imu_input_data + MAX_INPUT_SIZE * sizeof(float) * 1, tick_size, imu_output_data + start_offset + current_model.sample_size * sizeof(float) * 1, current_model.sample_size);
                linear_interpolation(imu_input_data + MAX_INPUT_SIZE * sizeof(float) * 2, tick_size, imu_output_data + start_offset + current_model.sample_size * sizeof(float) * 2, current_model.sample_size);
                linear_interpolation(imu_input_data + MAX_INPUT_SIZE * sizeof(float) * 3, tick_size, imu_output_data + start_offset + current_model.sample_size * sizeof(float) * 3, current_model.sample_size);
                linear_interpolation(imu_input_data + MAX_INPUT_SIZE * sizeof(float) * 4, tick_size, imu_output_data + start_offset + current_model.sample_size * sizeof(float) * 4, current_model.sample_size);
                linear_interpolation(imu_input_data + MAX_INPUT_SIZE * sizeof(float) * 5, tick_size, imu_output_data + start_offset + current_model.sample_size * sizeof(float) * 5, current_model.sample_size);

                imu_output_data[0] = SEND_IMU_DATASET_DATA_PREFIX;
                tick_size = current_model.sample_size;
                memcpy(imu_output_data + 1, (void *)&tick_size, sizeof(tick_size));
                int sent = xMessageBufferSend(xMessageBufferToClient, imu_output_data, tick_size * 24 + start_offset, portMAX_DELAY);
                ESP_LOGI(TAG, "data sent: %d, real size: %d", sent, int(tick_size * 24 + start_offset));
                tick_size = 0;
                ESP_LOGI(TAG, "exit btn 1 honlding");
            }
            vTaskDelay(pdMS_TO_TICKS(pdMS_TO_TICKS(1)));
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Never reach here
    vTaskDelete(NULL);
}
