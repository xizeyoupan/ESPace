#ifndef __CONTROL_SERVICE_SERVICE_H__
#define __CONTROL_SERVICE_SERVICE_H__

#include "user_config.h"

#define NUM_OF_MODULES (1)

typedef enum {
    COMMAND_MODEL  = 0,
    CONTIOUS_MODEL = 1,
} model_type;

typedef enum {
    MODE_INVAL = -1,
    MODE_DATASET,
    MODE_NUM,
} mode_index_type;

typedef struct
{
    char uuid[32];
    char name[64];
    uint16_t sample_tick;
    uint16_t sample_size;
    FILE *f;
} mcb_t;

/*
circulation control block
ccb领域大神
*/
typedef struct
{
    int8_t id;
    uint32_t bg_color;
    mcb_t mcb;
    model_type type;
    void (*btn0_click_cb)();
    void (*btn1_click_cb)();
    void (*continuous_model_cb)(int);
    void (*command_model_cb)(int);
} ccb_t;

#ifdef __cplusplus
extern "C" {
#endif
void init_control_block();

#ifdef __cplusplus
}
#endif

#endif