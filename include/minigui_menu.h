#ifndef MINIGUI_MENU_H
#define MINIGUI_MENU_H

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
 * @brief Initializes the global navigation menu.
 *
 * @section call_site
 * Called by `minigui_init()` during system startup.
 *
 * @section dependencies
 * - `lvgl`: To create the drawer and blocker objects.
 */
void minigui_menu_init(void);

/**
 * @brief Toggles the visibility of the navigation menu.
 *
 * @section call_site
 * Called by the hamburger button (open) or the blocker/nav buttons (close).
 *
 * @section dependencies
 * - `lvgl`: To animate the drawer position.
 */
void minigui_menu_toggle(void);

#ifdef __cplusplus
}
#endif

#endif // MINIGUI_MENU_H
