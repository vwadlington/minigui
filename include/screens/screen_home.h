#ifndef SCREEN_HOME_H
#define SCREEN_HOME_H

/******************************************************************************
 ******************************************************************************
 * 1. ESP-IDF / FreeRTOS Core (Framework)
 ******************************************************************************
 ******************************************************************************/
// None

/******************************************************************************
 ******************************************************************************
 * 2. Managed Espressif Components (External Managed Components)
 ******************************************************************************
 ******************************************************************************/
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 ******************************************************************************
 * PUBLIC API FUNCTIONS
 ******************************************************************************
 ******************************************************************************/

/**
 * @brief Creates the Home screen object
 *
 * @section call_site
 * Called by `minigui_switch_screen` when navigating to the Home page.
 *
 * @section dependencies
 * - `lvgl`: For widget creation.
 *
 * @param parent The `content_area` object from `minigui.c`.
 * @return void (The created objects are children of `parent`).
 */
void create_screen_home(lv_obj_t *parent);

#ifdef __cplusplus
}
#endif

#endif // SCREEN_HOME_H
