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
 * @brief WiFi credentials for connection
 */
typedef struct {
    char ssid[64];
    char password[128];
} minigui_wifi_credentials_t;

/**
 * @brief WiFi network info for scanning
 */
typedef struct {
    char ssid[64];
    int8_t rssi;
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
    float voltage;               // Incoming voltage (V)
    uint8_t cpu_usage;          // CPU usage percentage (0-100)
    uint32_t flash_used_kb;     // Flash memory used (KB)
    uint32_t flash_total_kb;    // Flash memory total (KB)
    uint32_t ram_used_kb;       // RAM used (KB)
    uint32_t ram_total_kb;      // RAM total (KB)
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
    bool connected;              // Whether network is connected
    char ssid[64];              // Connected network SSID
    char ip_address[16];        // IP address (e.g. "192.168.1.100")
    char mac_address[18];       // MAC address (e.g. "AA:BB:CC:DD:EE:FF")
} minigui_network_status_t;

/**
 * @brief Callback type for network status retrieval
 * @param status Output buffer to fill with current network status
 */
typedef void (*minigui_network_status_provider_t)(minigui_network_status_t *status);

/**
 * @brief Set screen brightness (proxies to registered callback)
 */
void minigui_set_brightness(uint8_t brightness);

/**
 * @brief Register a callback for WiFi configuration saving
 * @param cb The function pointer to handle saving credentials
 */
/**
 * @brief Internal helper to trigger the WiFi save callback.
 */
void minigui_save_wifi_credentials(const minigui_wifi_credentials_t *creds);

void minigui_register_wifi_save_cb(minigui_wifi_save_cb_t cb);

/**
 * @brief Register a callback for WiFi scanning
 * @param provider The function pointer to perform network scanning
 */
void minigui_register_wifi_scan_provider(minigui_wifi_scan_provider_t provider);

/**
 * @brief Internal helper to trigger the WiFi scan.
 * Used by the settings screen.
 */
size_t minigui_scan_wifi(minigui_wifi_network_t *networks, size_t max_count);

/**
 * @brief Register a callback for system statistics monitoring
 * @param provider The function pointer to retrieve system stats
 */
void minigui_register_system_stats_provider(minigui_system_stats_provider_t provider);

/**
 * @brief Internal helper to retrieve system statistics.
 * Used by the monitor panel.
 */
void minigui_get_system_stats(minigui_system_stats_t *stats);

/**
 * @brief Register a callback for network status monitoring
 * @param provider The function pointer to retrieve network status
 */
void minigui_register_network_status_provider(minigui_network_status_provider_t provider);

/**
 * @brief Internal helper to retrieve network status.
 * Used by the network panel.
 */
void minigui_get_network_status(minigui_network_status_t *status);

/**
 * @brief Internal helper to get the main content area (stage).
 */
lv_obj_t *minigui_get_content_area(void);

#ifdef __cplusplus
}
#endif

#endif // MINIGUI_H
