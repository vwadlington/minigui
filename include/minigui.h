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
 * @brief Log entry structure for UI display
 */
#define MINIGUI_MAX_LOGS 50

typedef struct {
    char timestamp[16];
    char source[16];
    char level[10];
    char message[256];
} minigui_log_entry_t;

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
 * @brief Callback type for log retrieval
 * @param logs Output buffer
 * @param max_count Maximum entries to retrieve
 * @param filter Source filter string
 * @return Number of entries retrieved
 */
typedef size_t (*minigui_log_provider_t)(minigui_log_entry_t *logs, size_t max_count, const char *filter);

/**
 * @brief Initialize the UI manager
 */
void minigui_init(void);

/**
 * @brief Register a log provider (call before switching to logs screen)
 * @param provider The function pointer to retrieve logs
 */
void minigui_set_log_provider(minigui_log_provider_t provider);

/**
 * @brief Callback type for time retrieval
 * @param buf Output buffer to fill with formatted time string
 * @param max_len Maximum length of the buffer
 */
typedef void (*minigui_time_provider_t)(char *buf, size_t max_len);

/**
 * @brief Register a time provider for the status bar clock
 * @param provider The function pointer to retrieve formatted time
 */
void minigui_set_time_provider(minigui_time_provider_t provider);

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
 * @brief Internal helper to get the main content area (stage).
 */
lv_obj_t *minigui_get_content_area(void);

#ifdef __cplusplus
}
#endif

#endif // MINIGUI_H
