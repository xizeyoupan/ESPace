#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <map>
#include <string>

#include "tensorflow/lite/core/c/common.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/testing/micro_test.h"

#include "math.h"
#include "string.h"

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cJSON.h"
#include "mdns.h"
#include "multi_button.h"

#include "esp_clk_tree.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_heap_caps.h"
#include "esp_http_server.h"
#include "esp_littlefs.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_sleep.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "esp_wifi.h"

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/rmt_tx.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/message_buffer.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "hal/efuse_hal.h"
#include "hal/efuse_ll.h"

#include "service/button_service.h"
#include "service/device_service.h"
#include "service/http_service.h"
#include "service/littlefs_service.h"
#include "service/mpu_service.h"
#include "service/tflite_service.h"
#include "service/wifi_service.h"
#include "service/ws2812_service.h"

#include "task/button.h"
#include "task/handle_req_task.h"
#include "task/ir_task.h"
#include "task/mpu_task.h"
#include "task/play_mp3.h"
#include "task/tflite_task.h"
#include "task/wand_server_task.h"
#include "task/ws2812_task.h"
#include "task/ws_task.h"

#include "nvs_flash.h"
#include "nvs_util.h"

#include "user_define.h"

#endif