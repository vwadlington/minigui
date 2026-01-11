#pragma once

#include "lvgl.h"

// Define the available screens
typedef enum {
    SCREEN_HOME,
    SCREEN_LOGS,
    SCREEN_SETTINGS,
    SCREEN_COUNT
} screen_type_t;

// Standard function prototype for creating a screen
typedef lv_obj_t* (*screen_create_fn_t)(void);

/**
 * @brief Initialize the GUI manager and the global menu.
 */
void minigui_init(void);

/**
 * @brief Switch the main content area to a specific screen.
 * This handles memory cleanup of the previous screen.
 * @param screen_type The enum of the screen to load.
 */
void minigui_switch_screen(screen_type_t screen_type);