#include "control_service.h"

ccb_t circulation_control_block_array[NUM_OF_MODULES];
SemaphoreHandle_t control_block_mutex;

mode_index_type mode_index = -1;

void init_control_block()
{
    control_block_mutex = xSemaphoreCreateMutex();
    configASSERT(control_block_mutex);

    // CNN 数据采集，训练
    circulation_control_block_array[MODE_DATASET].id       = MODE_DATASET;
    circulation_control_block_array[MODE_DATASET].bg_color = COLOR_GREEN;
}