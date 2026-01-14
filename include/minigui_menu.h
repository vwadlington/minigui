#ifndef MINIGUI_MENU_H
#define MINIGUI_MENU_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the global navigation menu.
 * * This creates the hamburger button, the background blocker, 
 * and the side drawer on lv_layer_top().
 */
void minigui_menu_init(void);

/**
 * @brief Toggles the visibility of the navigation menu.
 * * Triggers the slide animation for the drawer and toggles 
 * the visibility of the background blocker.
 */
void minigui_menu_toggle(void);

#ifdef __cplusplus
}
#endif

#endif // MINIGUI_MENU_H