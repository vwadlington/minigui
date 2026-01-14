#ifndef SCREEN_HOME_H
#define SCREEN_HOME_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Creates the Home screen object
 * @return Pointer to the created lv_obj_t (screen)
 */
void create_screen_home(lv_obj_t *parent);

#ifdef __cplusplus
}
#endif

#endif // SCREEN_HOME_H