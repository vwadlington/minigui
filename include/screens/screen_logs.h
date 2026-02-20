#ifndef SCREEN_LOGS_H
#define SCREEN_LOGS_H

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
 * @brief Creates the Logs screen object.
 *
 * @section call_site
 * Called by `minigui_switch_screen` when navigating to the Logs page.
 *
 * @section dependencies
 * - `lvgl`: For table and dropdown creation.
 *
 * @param parent The `content_area` object from `minigui.c`.
 * @return void
 */
void create_screen_logs(lv_obj_t *parent);

/**
 * @brief Manually triggers a refresh of the log table.
 *
 * @section call_site
 * Can be called by external events or the refresh button.
 *
 * @param filter The source filter string (e.g., "WIFI") or NULL/ALL.
 */
void refresh_log_table(const char *filter);

#ifdef __cplusplus
}
#endif

#endif // SCREEN_LOGS_H
