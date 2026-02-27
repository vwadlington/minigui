/******************************************************************************
 ******************************************************************************
 ** @brief     MiniGUI Settings Screen.
 **
 **            This header defines the interface for the settings screen,
 **            which includes brightness control, WiFi configuration, and system
 **            monitoring panels.
 **
 **            @section screen_settings.h - Settings screen interface.
 ******************************************************************************
 ******************************************************************************/

#ifndef SCREEN_SETTINGS_H
#define SCREEN_SETTINGS_H

/******************************************************************************
 ******************************************************************************
 ** 1. ESP-IDF / FreeRTOS Core (Framework)
 ******************************************************************************
 ******************************************************************************/
// None required for this header

/******************************************************************************
 ******************************************************************************
 ** 2. Managed Espressif Components (External Managed Components)
 ******************************************************************************
 ******************************************************************************/
#include "lvgl.h"

/******************************************************************************
 ******************************************************************************
 ** 3. Project-Specific Components (Local)
 ******************************************************************************
 ******************************************************************************/
// None

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 ******************************************************************************
 ** PUBLIC API FUNCTIONS
 ******************************************************************************
 ******************************************************************************/

/******************************************************************************
 ******************************************************************************
 ** @brief Creates the Settings screen object with its multi-pane layout.
 **
 ** @section call_site Called from:
 ** - minigui_switch_screen() when navigating to Settings.
 **
 ** @section dependencies Required Headers:
 ** - lvgl.h (for complex layout with flex, text area, and keyboard)
 **
 ** @param parent (lv_obj_t*): The content_area object from minigui.c.
 **
 ** @section pointers 
 ** - parent: Owned by minigui.c, used as the root for screen widgets.
 **
 ** @section variables 
 ** - None (Implementation uses local objects for panes and controls)
 **
 ** @return void
 ******************************************************************************
 ******************************************************************************/
void create_screen_settings(lv_obj_t *parent);

#ifdef __cplusplus
}
#endif

#endif // SCREEN_SETTINGS_H
