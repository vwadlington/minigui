#ifndef MINIGUI_H
#define MINIGUI_H

/******************************************************************************
 ******************************************************************************
 * 1. ESP-IDF / FreeRTOS Core (Framework)
 ******************************************************************************
 ******************************************************************************/
#include <stdint.h>

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
 * ENUMS & TYPEDEFS
 ******************************************************************************
 ******************************************************************************/

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
 * @brief Maximum number of log entries to retain
 */
#define MINIGUI_MAX_LOGS 50

/**
 * @brief Log entry structure for UI display
 */
typedef struct {
    char timestamp[16];   /**< Time string (e.g. "12:00:00") */
    char source[16];      /**< Log source tag (e.g. "WIFI") */
    char level[10];       /**< Log level (e.g. "INFO") */
    char message[256];    /**< The log message content */
} minigui_log_entry_t;

/**
 * @brief Callback type for brightness control
 * @param brightness Target brightness level (0-255)
 */
typedef void (*minigui_brightness_cb_t)(uint8_t brightness);

/**
 * @brief Standard function pointer for creating a screen.
 * @param parent The container object to build the screen into.
 */
typedef void (*ui_screen_creator_t)(lv_obj_t *parent);

/**
 * @brief Callback type for log retrieval
 * @param logs Output buffer to fill with log entries
 * @param max_count Maximum entries the buffer can hold
 * @param filter Optional source filter string (can be NULL)
 * @return Number of entries actually written to the buffer
 */
typedef size_t (*minigui_log_provider_t)(minigui_log_entry_t *logs, size_t max_count, const char *filter);

/**
 * @brief Callback type for time retrieval
 * @param buf Output buffer to fill with formatted time string
 * @param max_len Maximum length of the buffer
 */
typedef void (*minigui_time_provider_t)(char *buf, size_t max_len);

/**
 * @brief WiFi credentials for connection
 */
typedef struct {
    char ssid[64];        /**< Service Set Identifier */
    char password[128];   /**< WPA/WPA2 Password */
} minigui_wifi_credentials_t;

/**
 * @brief WiFi network info for scanning
 */
typedef struct {
    char ssid[64];        /**< Service Set Identifier */
    int8_t rssi;          /**< Received Signal Strength Indicator */
} minigui_wifi_network_t;

/**
 * @brief Callback type for WiFi configuration save
 * @param credentials The entered WiFi SSID and Password
 */
typedef void (*minigui_wifi_save_cb_t)(const minigui_wifi_credentials_t *credentials);

/**
 * @brief Callback type for WiFi scanning
 * @param networks Output buffer to fill with discovered networks
 * @param max_count Maximum entries to retrieve
 * @return Number of entries retrieved
 */
typedef size_t (*minigui_wifi_scan_provider_t)(minigui_wifi_network_t *networks, size_t max_count);

/**
 * @brief System statistics for monitoring dashboard
 */
typedef struct {
    float voltage;               /**< Incoming voltage (V) */
    uint8_t cpu_usage;           /**< CPU usage percentage (0-100) */
    uint32_t flash_used_kb;      /**< Flash memory used (KB) */
    uint32_t flash_total_kb;     /**< Flash memory total (KB) */
    uint32_t ram_used_kb;        /**< RAM used (KB) */
    uint32_t ram_total_kb;       /**< RAM total (KB) */
} minigui_system_stats_t;

/**
 * @brief Callback type for system stats retrieval
 * @param stats Output buffer to fill with current system statistics
 */
typedef void (*minigui_system_stats_provider_t)(minigui_system_stats_t *stats);

/**
 * @brief Current network connection status
 */
typedef struct {
    bool connected;              /**< Whether network is connected */
    char ssid[64];               /**< Connected network SSID */
    char ip_address[16];         /**< IP address (e.g. "192.168.1.100") */
    char mac_address[18];        /**< MAC address (e.g. "AA:BB:CC:DD:EE:FF") */
} minigui_network_status_t;

/**
 * @brief Callback type for network status retrieval
 * @param status Output buffer to fill with current network status
 */
typedef void (*minigui_network_status_provider_t)(minigui_network_status_t *status);


/******************************************************************************
 ******************************************************************************
 * PUBLIC API FUNCTIONS
 ******************************************************************************
 ******************************************************************************/

/**
 * @brief Initialize the UI manager
 *
 * @section call_site
 * Call this function from the application setup (e.g., `app_main`) after LVGL is initialized.
 *
 * @section dependencies
 * - `minigui_menu.h`: For initializing the menu system.
 * - `lvgl.h`: Core graphics processing.
 */
void minigui_init(void);

/**
 * @brief Register a log provider (call before switching to logs screen)
 *
 * @section call_site
 * Typically called during system initialization to hook up the backend logger.
 *
 * @param provider The function pointer to retrieve logs
 */
void minigui_set_log_provider(minigui_log_provider_t provider);

/**
 * @brief Register a time provider for the status bar clock
 *
 * @section call_site
 * Called during initialization to provide real-time clock data (e.g. from SNTP).
 *
 * @param provider The function pointer to retrieve formatted time
 */
void minigui_set_time_provider(minigui_time_provider_t provider);

/**
 * @brief Switch to a specific screen
 *
 * @section call_site
 * Called by the menu system or navigation buttons to change the active view.
 *
 * @param screen The screen enum to load
 */
void minigui_switch_screen(minigui_screen_t screen);

/**
 * @brief Register a callback for hardware brightness control
 *
 * @section call_site
 * Called during initialization to link the UI slider to the hardware PWM.
 *
 * @param cb Function to handle brightness changes
 */
void minigui_register_brightness_cb(minigui_brightness_cb_t cb);

/**
 * @brief Set screen brightness (proxies to registered callback)
 *
 * @section call_site
 * Called by the Settings screen brightness slider.
 *
 * @param brightness Target brightness value (0-255)
 */
void minigui_set_brightness(uint8_t brightness);

/**
 * @brief Internal helper to trigger the WiFi save callback.
 *
 * @section call_site
 * Called by the Settings screen when user submits WiFi credentials.
 *
 * @param creds Pointer to the collected credentials
 */
void minigui_save_wifi_credentials(const minigui_wifi_credentials_t *creds);

/**
 * @brief Register a callback for WiFi configuration saving
 *
 * @section call_site
 * Called during initialization to handle persistent storage of WiFi settings.
 *
 * @param cb The function pointer to handle saving credentials
 */
void minigui_register_wifi_save_cb(minigui_wifi_save_cb_t cb);

/**
 * @brief Register a callback for WiFi scanning
 *
 * @section call_site
 * Called during initialization to provide network scanning capabilities.
 *
 * @param provider The function pointer to perform network scanning
 */
void minigui_register_wifi_scan_provider(minigui_wifi_scan_provider_t provider);

/**
 * @brief Internal helper to trigger the WiFi scan.
 *
 * @section call_site
 * Used by the settings screen to populate the network list.
 *
 * @param networks Output buffer
 * @param max_count Capacity of the buffer
 * @return Number of networks found
 */
size_t minigui_scan_wifi(minigui_wifi_network_t *networks, size_t max_count);

/**
 * @brief Register a callback for system statistics monitoring
 *
 * @section call_site
 * Called during initialization to provide system health data.
 *
 * @param provider The function pointer to retrieve system stats
 */
void minigui_register_system_stats_provider(minigui_system_stats_provider_t provider);

/**
 * @brief Internal helper to retrieve system statistics.
 *
 * @section call_site
 * Used by the monitor panel to refresh the dashboard.
 *
 * @param stats Output pointer to fill
 */
void minigui_get_system_stats(minigui_system_stats_t *stats);

/**
 * @brief Register a callback for network status monitoring
 *
 * @section call_site
 * Called during initialization to provide connection state.
 *
 * @param provider The function pointer to retrieve network status
 */
void minigui_register_network_status_provider(minigui_network_status_provider_t provider);

/**
 * @brief Internal helper to retrieve network status.
 *
 * @section call_site
 * Used by the network panel to show IP/MAC address.
 *
 * @param status Output pointer to fill
 */
void minigui_get_network_status(minigui_network_status_t *status);

/**
 * @brief Internal helper to get the main content area (stage).
 *
 * @section call_site
 * Used by screen creators to attach their content.
 *
 * @return Pointer to the main content container
 */
lv_obj_t *minigui_get_content_area(void);

#ifdef __cplusplus
}
#endif

#endif // MINIGUI_H
