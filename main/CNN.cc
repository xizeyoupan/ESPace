#include <algorithm>
#include <cstdint>
#include <iterator>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/message_buffer.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "model.h"
#include "tensorflow/lite/core/c/common.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/testing/micro_test.h"
#include <map>
#include <string>

namespace micro_test
{
    int tests_passed;
    int tests_failed;
    bool is_test_complete;
    bool did_test_fail;
}

#define TF_LITE_MICRO_CHECK_FAIL()     \
    do                                 \
    {                                  \
        if (micro_test::did_test_fail) \
        {                              \
            return kTfLiteError;       \
        }                              \
    } while (false)

static const char *TAG = "TF";
// Arena size is a guesstimate, followed by use of
// MicroInterpreter::arena_used_bytes() on both the AudioPreprocessor and
// MicroSpeech models and using the larger of the two results.

// constexpr int kAudioSampleFrequency = 16000;
constexpr int kFeatureSize = 50;
constexpr int kFeatureCount = 6;
constexpr int kFeatureElementCount = (kFeatureSize * kFeatureCount);
// constexpr int kFeatureStrideMs = 20;
// constexpr int kFeatureDurationMs = 30;

// Variables for the model's output categories.
constexpr int kCategoryCount = 3;

constexpr size_t kArenaSize = 10 * 1024; // xtensa p6
uint8_t g_arena[kArenaSize];

typedef float modeltype;

using Features = float_t[kFeatureSize][kFeatureCount];
Features g_features;

// constexpr int kAudioSampleDurationCount = kFeatureDurationMs * kAudioSampleFrequency / 1000;
// constexpr int kAudioSampleStrideCount = kFeatureStrideMs * kAudioSampleFrequency / 1000;

using OpResolver = tflite::MicroMutableOpResolver<9>;

TfLiteStatus RegisterOps(OpResolver &op_resolver)
{
    TF_LITE_ENSURE_STATUS(op_resolver.AddFullyConnected());
    TF_LITE_ENSURE_STATUS(op_resolver.AddConv2D());
    TF_LITE_ENSURE_STATUS(op_resolver.AddRelu());
    TF_LITE_ENSURE_STATUS(op_resolver.AddReshape());
    TF_LITE_ENSURE_STATUS(op_resolver.AddMaxPool2D());
    TF_LITE_ENSURE_STATUS(op_resolver.AddExpandDims());
    TF_LITE_ENSURE_STATUS(op_resolver.AddSoftmax());
    TF_LITE_ENSURE_STATUS(op_resolver.AddTranspose());
    TF_LITE_ENSURE_STATUS(op_resolver.AddLogSoftmax());
    return kTfLiteOk;
}

TfLiteStatus LoadMicroSpeechModelAndPerformInference()
{
    std::map<int, std::string> mp;
    mp[0] = "顺时针";
    mp[1] = "逆时针";
    mp[2] = "不动";
    // Map the model into a usable data structure. This doesn't involve any
    // copying or parsing, it's a very lightweight operation.
    const tflite::Model *model = tflite::GetModel(g_model);
    TF_LITE_MICRO_EXPECT(model->version() == TFLITE_SCHEMA_VERSION);
    TF_LITE_MICRO_CHECK_FAIL();

    OpResolver op_resolver;
    TF_LITE_MICRO_EXPECT(RegisterOps(op_resolver) == kTfLiteOk);
    TF_LITE_MICRO_CHECK_FAIL();

    tflite::MicroInterpreter interpreter(model, op_resolver, g_arena, kArenaSize);

    ESP_LOGI(TAG, "free_heap_size = %lu", esp_get_free_heap_size());
    TF_LITE_MICRO_EXPECT(interpreter.AllocateTensors() == kTfLiteOk);
    TF_LITE_MICRO_CHECK_FAIL();

    MicroPrintf("MicroSpeech model arena size = %u",
                interpreter.arena_used_bytes());

    TfLiteTensor *input = interpreter.input(0);
    TF_LITE_MICRO_EXPECT(input != nullptr);
    TF_LITE_MICRO_CHECK_FAIL();
    // check input shape is compatible with our feature data size
    TF_LITE_MICRO_EXPECT_EQ(kFeatureCount,
                            input->dims->data[input->dims->size - 1]);
    TF_LITE_MICRO_CHECK_FAIL();

    TfLiteTensor *output = interpreter.output(0);
    TF_LITE_MICRO_EXPECT(output != nullptr);
    TF_LITE_MICRO_CHECK_FAIL();
    // check output shape is compatible with our number of prediction categories
    TF_LITE_MICRO_EXPECT_EQ(kCategoryCount,
                            output->dims->data[output->dims->size - 1]);
    TF_LITE_MICRO_CHECK_FAIL();

    while (1)
    {
        break;

        uint64_t start = esp_timer_get_time();

        std::copy_n(&g_features[0][0], kFeatureElementCount,
                    tflite::GetTensorData<modeltype>(input));
        TF_LITE_MICRO_EXPECT(interpreter.Invoke() == kTfLiteOk);
        TF_LITE_MICRO_CHECK_FAIL();

        uint64_t end = esp_timer_get_time();

        ESP_LOGI(TAG, "Took %llu milliseconds", (end - start) / 1000);

        float output_scale = output->params.scale;
        int output_zero_point = output->params.zero_point;

        // Dequantize output values
        float category_predictions[kCategoryCount];
        float _max = -FLT_MAX;
        int _max_index;

        for (int i = 0; i < kCategoryCount; i++)
        {
            // category_predictions[i] = (tflite::GetTensorData<modeltype>(output)[i] - output_zero_point) * output_scale;
            category_predictions[i] = tflite::GetTensorData<modeltype>(output)[i];
            if (category_predictions[i] > _max)
            {
                _max = category_predictions[i];
                _max_index = i;
            }

            MicroPrintf("  %.4f %d", static_cast<double>(category_predictions[i]), i);
        }

        MicroPrintf("Max index: %d, value: %s", _max_index, mp[_max_index].c_str());

        ESP_LOGI(TAG, "END, free_heap_size = %lu", esp_get_free_heap_size());
    }

    return kTfLiteOk;
}

extern "C" void CNN_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Task is running on core .");

    BaseType_t core_id = xPortGetCoreID(); // 返回当前任务所在的核心 ID
    ESP_LOGI(TAG, "Task is running on core %d.", core_id);

    LoadMicroSpeechModelAndPerformInference();
    // while (1) {
    //     ESP_LOGI(TAG, "free_heap_size = %lu", esp_get_free_heap_size());
    //     vTaskDelay(500 / portTICK_PERIOD_MS);
    // }
    vTaskDelete(NULL);
}