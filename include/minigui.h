#ifndef MINIGUI_H
#define MINIGUI_H

#include "lvgl.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Available screens in the application
 */
typedef enum {
    MINIGUI_SCREEN_HOME,
    MINIGUI_SCREEN_LOGS,
    MINIGUI_SCREEN_SETTINGS,
    MINIGUI_SCREEN_COUNT
} minigui_screen_t;

/**
 * @brief Callback type for providing log data to the GUI.
 */
typedef void (*minigui_log_provider_t)(const char *filter, lv_obj_t *table, uint16_t max_rows);

/**
 * @brief Callback type for brightness control
 */
typedef void (*minigui_brightness_cb_t)(uint8_t brightness);

// --- NEW DEFINITION ---
/**
 * @brief Standard function pointer for creating a screen.
 * @param parent The container object to build the screen into.
 */
typedef void (*ui_screen_creator_t)(lv_obj_t *parent);

/**
 * @brief Initialize the UI manager
 */
void minigui_init(void);

/**
 * @brief Switch to a specific screen
 * @param screen The screen enum to load
 */
void minigui_switch_screen(minigui_screen_t screen);

/**
 * @brief Register a callback for hardware brightness control
 */
void minigui_register_brightness_cb(minigui_brightness_cb_t cb);

/**
 * @brief Set screen brightness (proxies to registered callback)
 */
void minigui_set_brightness(uint8_t brightness);

/**
 * @brief Registers a data source for the log screen.
 */
void minigui_register_log_provider(minigui_log_provider_t provider_cb);

/**
 * @brief Internal helper to retrieve the registered log provider.
 */
minigui_log_provider_t minigui_get_log_provider(void);

/**
 * @brief Internal helper to get the main content area (stage).
 */
lv_obj_t *minigui_get_content_area(void);

#ifdef __cplusplus
}
#endif

#endif // MINIGUI_H