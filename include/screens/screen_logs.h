/******************************************************************************
 ******************************************************************************
 ** @brief     MiniGUI Logs Screen.
 **
 **            This header defines the interface for the system logging screen,
 **            which displays a filterable table of log entries.
 **
 **            @section screen_logs.h - Logs screen interface.
 ******************************************************************************
 ******************************************************************************/

#ifndef SCREEN_LOGS_H
#define SCREEN_LOGS_H

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
 ** @brief Creates the Logs screen object.
 **
 ** @section call_site Called from:
 ** - minigui_switch_screen() when navigating to the Logs page.
 **
 ** @section dependencies Required Headers:
 ** - lvgl.h (for table and dropdown creation)
 **
 ** @param parent (lv_obj_t*): The content_area object from minigui.c.
 **
 ** @section pointers 
 ** - parent: Owned by minigui.c, used as the root for screen widgets.
 **
 ** @section variables 
 ** - None
 **
 ** @return void
 ******************************************************************************
 ******************************************************************************/
void create_screen_logs(lv_obj_t *parent);

/******************************************************************************
 ******************************************************************************
 ** @brief Manually triggers a refresh of the log table.
 **
 ** @section call_site Called from:
 ** - External events or the refresh button.
 **
 ** @section dependencies Required Headers:
 ** - lvgl.h (to update table cells)
 **
 ** @param filter (const char*): The source filter string or NULL/ALL.
 **
 ** @section pointers 
 ** - filter: String literal or owned by caller.
 **
 ** @section variables 
 ** - None
 **
 ** @return void
 ******************************************************************************
 ******************************************************************************/
void refresh_log_table(const char *filter);

#ifdef __cplusplus
}
#endif

#endif // SCREEN_LOGS_H
