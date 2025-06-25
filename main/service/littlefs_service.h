#ifndef __LITTLEFS_SERVICE_H__
#define __LITTLEFS_SERVICE_H__

#include "user_config.h"

#ifdef __cplusplus
extern "C" {
#endif

void littlefs_init(void);
void list_littlefs_files(void);

#ifdef __cplusplus
}
#endif

#endif