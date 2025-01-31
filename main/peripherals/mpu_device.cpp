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

#include "user_config.h"

float p_data[50][6];
QueueHandle_t xQueueTrans, xQueuePdata;
MessageBufferHandle_t xMessageBufferToClient;

static const char *TAG = "IMU";

// I2Cdev and MPU6050 must be installed as libraries, or else the .cpp/.h files
// for both classes must be in the include path of your project
#include "I2Cdev.h"
#include "MPU6050.h"

// Source: https://github.com/TKJElectronics/KalmanFilter
#include "Kalman.h"

#define RESTRICT_PITCH // Comment out to restrict roll to ±90deg instead
#define RAD_TO_DEG (180.0 / M_PI)
#define DEG_TO_RAD 0.0174533

MPU6050 mpu;
extern user_config_t user_config;

void start_i2c(void)
{
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

extern "C" void mpu6050(void *pvParameters)
{
    start_i2c();

    // Create Queue
    xQueueTrans = xQueueCreate(10, sizeof(POSE_t));
    configASSERT(xQueueTrans);

    // Create Message Buffer
    xMessageBufferToClient = xMessageBufferCreate(1024);
    configASSERT(xMessageBufferToClient);

    // Initialize mpu6050
    mpu.initialize();

    // Get DeviceID
    uint8_t devid = mpu.getDeviceID();
    ESP_LOGI(TAG, "devid=0x%x", devid);

    // Get the sample rate
    ESP_LOGI(TAG, "getRate()=%d", mpu.getRate());
    // Set the sample rate to 8kHz
    if (mpu.getRate() != 0)
        mpu.setRate(0);

    // Get FSYNC configuration value
    ESP_LOGI(TAG, "getExternalFrameSync()=%d", mpu.getExternalFrameSync());
    // Disable FSYNC and set 260 Hz Acc filtering, 256 Hz Gyro filtering
    if (mpu.getExternalFrameSync() != 0)
        mpu.setExternalFrameSync(0);

    // Set Digital Low Pass Filter
    ESP_LOGI(TAG, "getDLPFMode()=%d", mpu.getDLPFMode());
    if (mpu.getDLPFMode() != 6)
        mpu.setDLPFMode(6);

    // Get Accelerometer Scale Range
    ESP_LOGI(TAG, "getFullScaleAccelRange()=%d", mpu.getFullScaleAccelRange());
    // Set Accelerometer Full Scale Range to ±2g
    if (mpu.getFullScaleAccelRange() != 0)
        mpu.setFullScaleAccelRange(0); // -2 --> +2[g]

    // Get Gyro Scale Range
    ESP_LOGI(TAG, "getFullScaleGyroRange()=%d", mpu.getFullScaleGyroRange());
    // Set Gyro Full Scale Range to ±250deg/s
    if (mpu.getFullScaleGyroRange() != 0)
        mpu.setFullScaleGyroRange(0); // -250 --> +250[Deg/Sec]

    int16_t ax, ay, az;
    int16_t gx, gy, gz;

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    TickType_t xLastWakeTime = xTaskGetTickCount();
    uint16_t index = 0;
    uint8_t cnt = 0;
    while (1)
    {
        vTaskDelayUntil(&xLastWakeTime, 10 / portTICK_PERIOD_MS);
        int key_press = gpio_get_level(GPIO_NUM_9);
        if (key_press)
        {
            cnt = 0;
            continue;
        }

        cJSON *request;
        request = cJSON_CreateObject();
        // cJSON_AddStringToObject(request, "id", "data-request");
        mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

        p_data[cnt][0] = ax;
        p_data[cnt][1] = ay;
        p_data[cnt][2] = az;
        p_data[cnt][3] = gx;
        p_data[cnt][4] = gy;
        p_data[cnt][5] = gz;

        cJSON_AddNumberToObject(request, "ax", ax);
        cJSON_AddNumberToObject(request, "ay", ay);
        cJSON_AddNumberToObject(request, "az", az);
        cJSON_AddNumberToObject(request, "gx", gx);
        cJSON_AddNumberToObject(request, "gy", gy);
        cJSON_AddNumberToObject(request, "gz", gz);
        cJSON_AddNumberToObject(request, "index", index);
        cJSON_AddNumberToObject(request, "cnt", cnt);
        char *my_json_string = cJSON_Print(request);
        // ESP_LOGI("", "%s", my_json_string);
        // printf("%s\n", my_json_string);
        cJSON_Delete(request);
        cJSON_free(my_json_string);

        if (++cnt == 50)
        {
            cnt = 0;
            index++;

            if (xQueueSend(xQueuePdata, (void *)&p_data, 100) != pdPASS)
            {
                ESP_LOGE(pcTaskGetName(NULL), "xQueueSend fail");
            }
        }
    }

    // Never reach here
    vTaskDelete(NULL);
}
