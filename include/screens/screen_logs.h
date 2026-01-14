#ifndef SCREEN_LOGS_H
#define SCREEN_LOGS_H

#include "lvgl.h"

// Standard Creator
void create_screen_logs(lv_obj_t *parent);

// Public function for refreshing data
void refresh_log_table(const char *filter);

#endif // SCREEN_LOGS_H