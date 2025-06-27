#include "tflite_service.h"

namespace micro_test {
int tests_passed;
int tests_failed;
bool is_test_complete;
bool did_test_fail;
}

#define TF_LITE_MICRO_CHECK_FAIL()       \
    do {                                 \
        if (micro_test::did_test_fail) { \
            return kTfLiteError;         \
        }                                \
    } while (false)

typedef float model_type;

static const char* TAG = "TFLITE_SERVICE";

static uint8_t* tflite_arena_buffer = NULL;
extern user_config_t user_config;
uint8_t* tflite_model_buf;

using OpResolver = tflite::MicroMutableOpResolver<9>;
OpResolver op_resolver;
const tflite::Model* model;
tflite::MicroInterpreter* interpreter;
TfLiteTensor* input;
TfLiteTensor* output;

TfLiteStatus RegisterOps(OpResolver& op_resolver)
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

TfLiteStatus tflite_init()
{
    tflite_arena_buffer = (uint8_t*)malloc(user_config.tflite_arena_size);
    if (tflite_arena_buffer == nullptr) {
        ESP_LOGE(TAG, "Failed to allocate model arena of size %u", user_config.tflite_arena_size);
        return kTfLiteError;
    }

    tflite_model_buf = (uint8_t*)malloc(user_config.tflite_model_size);
    if (tflite_model_buf == nullptr) {
        ESP_LOGE(TAG, "Failed to allocate model buffer of size %u", user_config.tflite_model_size);
        return kTfLiteError;
    }

    TF_LITE_MICRO_EXPECT(RegisterOps(op_resolver) == kTfLiteOk);
    TF_LITE_MICRO_CHECK_FAIL();
    return kTfLiteOk;
}

TfLiteStatus load_and_check_model(int* input_size, int* output_size)
{
    model = tflite::GetModel(tflite_model_buf);
    TF_LITE_MICRO_EXPECT(model->version() == TFLITE_SCHEMA_VERSION);
    TF_LITE_MICRO_CHECK_FAIL();

    static tflite::MicroInterpreter static_interpreter(model, op_resolver, tflite_arena_buffer, user_config.tflite_arena_size);
    interpreter = &static_interpreter;

    ESP_LOGI(TAG, "free_heap_size = %lu", esp_get_free_heap_size());
    TF_LITE_MICRO_EXPECT(interpreter->AllocateTensors() == kTfLiteOk);
    TF_LITE_MICRO_CHECK_FAIL();

    size_t arena_used_size = interpreter->arena_used_bytes();
    ESP_LOGI(TAG, "tflite arena size = %u, remain %d bytes.", arena_used_size, user_config.tflite_arena_size - arena_used_size);

    input = interpreter->input(0);
    TF_LITE_MICRO_EXPECT(input != nullptr);
    TF_LITE_MICRO_CHECK_FAIL();

    ESP_LOGI(TAG, "model input dims:");
    for (int i = 0; i < input->dims->size; ++i) {
        ESP_LOGI(TAG, "model input dim[%d]=%d", i, input->dims->data[i]);
    }
    if (input_size) {
        *input_size = input->dims->data[2];
    }

    output = interpreter->output(0);
    TF_LITE_MICRO_EXPECT(output != nullptr);
    TF_LITE_MICRO_CHECK_FAIL();

    ESP_LOGI(TAG, "model input dims:");
    for (int i = 0; i < output->dims->size; ++i) {
        ESP_LOGI(TAG, "model output dim[%d]=%d", i, output->dims->data[i]);
    }
    if (output_size) {
        *output_size = output->dims->data[1];
    }

    return kTfLiteOk;
}

TfLiteStatus tflite_invoke(float* input_buf, size_t data_size, float* out_put_buf, int* output_size)
{
    if (output_size) {
        *output_size = output->dims->data[1];
    }
    uint64_t start = esp_timer_get_time();

    std::copy_n(input_buf, data_size, tflite::GetTensorData<model_type>(input));
    size_t arena_used_size = interpreter->arena_used_bytes();

    TF_LITE_MICRO_EXPECT(interpreter->Invoke() == kTfLiteOk);
    TF_LITE_MICRO_CHECK_FAIL();

    uint64_t end = esp_timer_get_time();
    ESP_LOGI(TAG, "Took %llu milliseconds", (end - start) / 1000);
    ESP_LOGI(TAG, "tflite arena size = %u, remain %d bytes.", arena_used_size, user_config.tflite_arena_size - arena_used_size);

    // Log output tensor values
    for (int i = 0; i < output->dims->data[1]; ++i) {
        ESP_LOGI(TAG, "output[%d]=%f", i, tflite::GetTensorData<model_type>(output)[i]);
    }

    if (out_put_buf) {
        for (int i = 0; i < output->dims->data[1]; ++i) {
            out_put_buf[i] = tflite::GetTensorData<model_type>(output)[i];
        }
    }

    return kTfLiteOk;
}
