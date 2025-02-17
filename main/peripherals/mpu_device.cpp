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

#include "user_config.h"

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
const double accel_sensitivity = 16384.0;
const double gyro_sensitivity = 131.0;
char data[50];

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

    // Set Accelerometer Full Scale Range to ±2g
    if (mpu.getFullScaleAccelRange() != 0)
        mpu.setFullScaleAccelRange(0); // -2 --> +2[g]

    // Get Accelerometer Scale Range
    ESP_LOGI(TAG, "getFullScaleAccelRange()=%d", mpu.getFullScaleAccelRange());

    // Set Gyro Full Scale Range to ±250deg/s
    if (mpu.getFullScaleGyroRange() != 0)
        mpu.setFullScaleGyroRange(0); // -250 --> +250[Deg/Sec]

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

extern "C" void mpu6050(void *pvParameters)
{
    BaseType_t core_id = xPortGetCoreID(); // 返回当前任务所在的核心 ID
    ESP_LOGI(TAG, "Task is running on core %d.", core_id);

    imu_mutex = xSemaphoreCreateMutex();
    configASSERT(imu_mutex);

    reset_imu();
    get_angle();

    while (1)
    {
        xSemaphoreTake(imu_mutex, portMAX_DELAY);
        _getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
        // ESP_LOGI(TAG, "mpu data: %f %f %f - %f %f %f", ax, ay, az, gx, gy, gz);
        get_angle();

        float ax_f = ax;
        float ay_f = ay;
        float az_f = az;
        float gx_f = gx;
        float gy_f = gy;
        float gz_f = gz;

        memset(data, 0, sizeof(data));
        data[0] = SEND_WS_IMU_DATA_PREFIX;
        memcpy(data + 1, (void *)&_roll, sizeof(_roll));
        memcpy(data + 5, (void *)&_pitch, sizeof(_pitch));

        memcpy(data + 9, (void *)&ax_f, sizeof(ax_f));
        memcpy(data + 13, (void *)&ay_f, sizeof(ay_f));
        memcpy(data + 17, (void *)&az_f, sizeof(az_f));
        memcpy(data + 21, (void *)&gx_f, sizeof(gx_f));
        memcpy(data + 25, (void *)&gy_f, sizeof(gy_f));
        memcpy(data + 29, (void *)&gz_f, sizeof(gz_f));

        xMessageBufferSend(xMessageBufferToClient, data, 33, 0);
        xSemaphoreGive(imu_mutex);

        vTaskDelay(pdMS_TO_TICKS(125));
    } // end while

    // Never reach here
    vTaskDelete(NULL);
}
