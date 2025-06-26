#ifndef __LITTLEFS_SERVICE_H__
#define __LITTLEFS_SERVICE_H__

#include "user_config.h"

#ifdef __cplusplus
extern "C" {
#endif

void littlefs_init(void);
void list_littlefs_files(char* file_list);
void read_model_to_buf(const char* model_name, void* buf, int buf_size);
uint32_t get_file_size(const char* file_name);
void unlink_file(char* filename);
void rename_file(char* old_name, char* new_name);

#ifdef __cplusplus
}
#endif

#endif