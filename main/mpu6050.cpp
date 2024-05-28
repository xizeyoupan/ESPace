// I2C device class (I2Cdev) demonstration Arduino sketch for MPU6050 class using DMP (MotionApps v2.0)
// 6/21/2012 by Jeff Rowberg <jeff@rowberg.net>
// Updates should (hopefully) always be available at https://github.com/jrowberg/i2cdevlib
//
// Changelog:
//		2023-03-10 - Fit to esp-idf v5
//		2019-07-08 - Added Auto Calibration and offset generator
//		   - and altered FIFO retrieval sequence to avoid using blocking code
//		2016-04-18 - Eliminated a potential infinite loop
//		2013-05-08 - added seamless Fastwire support
//				   - added note about gyro calibration
//		2012-06-21 - added note about Arduino 1.0.1 + Leonardo compatibility error
//		2012-06-20 - improved FIFO overflow handling and simplified read process
//		2012-06-19 - completely rearranged DMP initialization code and simplification
//		2012-06-13 - pull gyro and accel data from FIFO packet instead of reading directly
//		2012-06-09 - fix broken FIFO read sequence and change interrupt detection to RISING
//		2012-06-05 - add gravity-compensated initial reference frame acceleration output
//				   - add 3D math helper file to DMP6 example sketch
//				   - add Euler output and Yaw/Pitch/Roll output formats
//		2012-06-04 - remove accel offset clearing for better results (thanks Sungon Lee)
//		2012-06-01 - fixed gyro sensitivity to be 2000 deg/sec instead of 250
//		2012-05-30 - basic DMP initialization working

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

#include "math.h"

#include "cJSON.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/message_buffer.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "parameter.h"

extern QueueHandle_t xQueueTrans;
extern MessageBufferHandle_t xMessageBufferToClient;

static const char* TAG = "IMU";

// I2Cdev and MPU6050 must be installed as libraries, or else the .cpp/.h files
// for both classes must be in the include path of your project
#include "I2Cdev.h"
#include "MPU6050.h"

// Source: https://github.com/TKJElectronics/KalmanFilter
#include "Kalman.h"

#define RESTRICT_PITCH // Comment out to restrict roll to ±90deg instead
#define RAD_TO_DEG (180.0 / M_PI)
#define DEG_TO_RAD 0.0174533

// Arduino macro
#define micros() (unsigned long)(esp_timer_get_time())
#define delay(ms) esp_rom_delay_us(ms * 1000)

MPU6050 mpu;

void mpu6050(void* pvParameters)
{
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

    gpio_config_t key_conf = {
        .pin_bit_mask = (1ULL << 9),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&key_conf);

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    TickType_t xLastWakeTime = xTaskGetTickCount();
    uint16_t index = 0;
    uint8_t cnt = 0;
    while (1) {
        vTaskDelayUntil(&xLastWakeTime, 10 / portTICK_PERIOD_MS);
        int key_press = gpio_get_level(GPIO_NUM_9);
        if (key_press) {
            cnt = 0;
            continue;
        }

        cJSON* request;
        request = cJSON_CreateObject();
        // cJSON_AddStringToObject(request, "id", "data-request");
        mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
        cJSON_AddNumberToObject(request, "ax", ax);
        cJSON_AddNumberToObject(request, "ay", ay);
        cJSON_AddNumberToObject(request, "az", az);
        cJSON_AddNumberToObject(request, "gx", gx);
        cJSON_AddNumberToObject(request, "gy", gy);
        cJSON_AddNumberToObject(request, "gz", gz);
        cJSON_AddNumberToObject(request, "index", index);
        cJSON_AddNumberToObject(request, "cnt", cnt);
        char* my_json_string = cJSON_Print(request);
        // ESP_LOGI("", "%s", my_json_string);
        printf("%s\n", my_json_string);
        cJSON_Delete(request);
        cJSON_free(my_json_string);

        if (++cnt == 50) {
            cnt = 0;
            index++;
        }
    }

    // Never reach here
    vTaskDelete(NULL);
}
