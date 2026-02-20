#ifndef SCREEN_SETTINGS_H
#define SCREEN_SETTINGS_H

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
 * @brief Creates the Settings screen object with its multi-pane layout.
 *
 * @section call_site
 * Called by `minigui_switch_screen` when navigating to Settings.
 *
 * @section dependencies
 * - `lvgl`: For complex layout (flex, text area, keyboard).
 *
 * @param parent The `content_area` object from `minigui.c`.
 * @return void
 */
void create_screen_settings(lv_obj_t *parent);

#ifdef __cplusplus
}
#endif

#endif // SCREEN_SETTINGS_H
