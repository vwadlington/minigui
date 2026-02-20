
/******************************************************************************
 ******************************************************************************
 * 1. ESP-IDF / FreeRTOS Core (Framework)
 ******************************************************************************
 ******************************************************************************/
#include <time.h>
#include <stdio.h>
#include <string.h>

/******************************************************************************
 ******************************************************************************
 * 2. Managed Espressif Components (External Managed Components)
 ******************************************************************************
 ******************************************************************************/
// None directly here, lvgl is included via minigui.h

/******************************************************************************
 ******************************************************************************
 * 3. Project-Specific Components (Local)
 ******************************************************************************
 ******************************************************************************/
#include "minigui.h"
#include "minigui_menu.h"
#include "screens/screen_home.h"
#include "screens/screen_logs.h"
#include "screens/screen_settings.h"

/******************************************************************************
 ******************************************************************************
 * FILE GLOBALS
 ******************************************************************************
 ******************************************************************************/

/******************************************************************************
 * @brief Reference to the main flex container
 * @section scope
 * Internal to minigui.c
 * @section rationale
 * Acts as the root parent for all UI elements created by this module.
 ******************************************************************************/
static lv_obj_t *main_container = NULL;

/******************************************************************************
 * @brief Reference to the top status bar
 * @section scope
 * Internal to minigui.c
 * @section rationale
 * Holds the menu button, title, and clock.
 ******************************************************************************/
static lv_obj_t *status_bar = NULL;

/******************************************************************************
 * @brief Reference to the dynamic content area
 * @section scope
 * Internal to minigui.c
 * @section rationale
 * This is where different screens (Home, Logs, etc.) inject their content.
 ******************************************************************************/
static lv_obj_t *content_area = NULL;

/******************************************************************************
 * @brief Reference to the screen title label
 * @section scope
 * Internal to minigui.c
 * @section rationale
 * Updated whenever the screen changes to reflect the current view name.
 ******************************************************************************/
static lv_obj_t *lbl_title = NULL;

/******************************************************************************
 * @brief Reference to the clock label
 * @section scope
 * Internal to minigui.c
 * @section rationale
 * Updated every second by a timer to show current time.
 ******************************************************************************/
static lv_obj_t *lbl_clock = NULL;

// Callback hooks
static minigui_brightness_cb_t brightness_cb = NULL;
static minigui_time_provider_t global_time_provider = NULL;
static minigui_wifi_save_cb_t wifi_save_cb = NULL;
static minigui_wifi_scan_provider_t wifi_scan_provider = NULL;
static minigui_system_stats_provider_t system_stats_provider = NULL;
static minigui_network_status_provider_t network_status_provider = NULL;

// Screen Creator Array
static const ui_screen_creator_t screen_creators[MINIGUI_SCREEN_COUNT] = {
    [MINIGUI_SCREEN_HOME]     = create_screen_home,
    [MINIGUI_SCREEN_LOGS]     = create_screen_logs,
    [MINIGUI_SCREEN_SETTINGS] = create_screen_settings
};

// ============================================================================
// PRIVATE HELPER FUNCTIONS
// ============================================================================

/******************************************************************************
 * @brief Updates the clock label with current system time.
 *
 * @section call_site
 * Called by `lv_timer` every 1000ms.
 *
 * @section dependencies
 * - `time.h`: For standard time functions.
 * - `lvgl`: For label updating.
 *
 * @param timer The LVGL timer instance triggering this callback.
 *
 * @section pointers
 * - `timer`: Owned by LVGL, unused in logic.
 *
 * @section variables
 * - `buf`: Character buffer [32] to hold the formatted time string. Rationale: Temporary storage for strftime output.
 * - `now`: `time_t` holding current epoch time. Rationale: Source for local time conversion.
 * - `tm_info`: `struct tm*` holding broken-down time. Rationale: Needed for formatting.
 *
 * @return void
 *
 * Implementation Steps
 * 1. Check if the clock label exists; return if not.
 * 2. If a global time provider is registered, use it to get the time string.
 * 3. Otherwise, get the system time using `time(NULL)`.
 * 4. Format the time as "Day Mon/Day Hr:Min:Sec".
 * 5. Update the LVGL label text with the formatted string.
 ******************************************************************************/
static void update_clock_cb(lv_timer_t *timer) {
    (void)timer;
    if (!lbl_clock) return;

    char buf[32];

    // 1. Try the registered time provider first
    if (global_time_provider) {
        global_time_provider(buf, sizeof(buf));
    }
    // 2. Fall back to standard C time library
    else {
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        // Format: "Sat 02/07 12:08:45"
        strftime(buf, sizeof(buf), "%a %m/%d %H:%M:%S", tm_info);
    }

    lv_label_set_text(lbl_clock, buf);
}

/******************************************************************************
 * @brief Event wrapper to open the menu
 *
 * @section call_site
 * Assigned to the hamburger button's `LV_EVENT_CLICKED`.
 *
 * @section dependencies
 * - `minigui_menu.h`: To toggle the menu visibility.
 *
 * @param e LVGL event object.
 *
 * @return void
 *
 * Implementation Steps
 * 1. Log the user interaction.
 * 2. Call `minigui_menu_toggle()` to show/hide the sidebar.
 ******************************************************************************/
static void menu_btn_event_cb(lv_event_t * e) {
    LV_LOG_USER("Hamburger menu toggled");
    minigui_menu_toggle();
}

/******************************************************************************
 * @brief Ensures the hamburger button is a perfect square.
 *
 * @section call_site
 * Assigned to the hamburger button's `LV_EVENT_SIZE_CHANGED`.
 *
 * @section dependencies
 * - `lvgl`: For object geometry manipulation.
 *
 * @param e LVGL event object.
 *
 * @section pointers
 * - `e`: Owned by LVGL, contains target object.
 *
 * @section variables
 * - `btn`: target object. Rationale: The button being resized.
 * - `parent`: parent object. Rationale: The status bar, whose height determines the button size.
 * - `h`: height of the parent. Rationale: Used to set the width.
 *
 * @return void
 *
 * Implementation Steps
 * 1. Retrieve the button object from the event.
 * 2. Retrieve the parent object (status bar).
 * 3. Get the parent's height.
 * 4. Set the button's width equal to the parent's height to force a square aspect ratio.
 ******************************************************************************/
static void sync_square_size_cb(lv_event_t * e) {
    lv_obj_t * btn = lv_event_get_target(e);
    lv_obj_t * parent = lv_obj_get_parent(btn);
    int32_t h = lv_obj_get_height(parent);
    if (h > 0) {
        lv_obj_set_width(btn, h);
    }
}

// ============================================================================
// PUBLIC API FUNCTIONS
// ============================================================================

/******************************************************************************
 * @brief Initialize the UI manager
 *
 * @section call_site
 * Called from `app_main` or similar entry point.
 *
 * @section dependencies
 * - `minigui_menu.h`: For menu initialization.
 * - `lvgl`: For all UI creation.
 *
 * @return void
 *
 * Implementation Steps
 * 1. Initialize the side menu system.
 * 2. Configure the active screen background to black.
 * 3. Create the `main_container` with a vertical flex layout to hold status bar and content.
 * 4. Create the `status_bar` with a horizontal flex layout.
 * 5. Create the hamburger button and attach the square-size sync callback.
 * 6. Create the title label with flex-grow to push the clock to the right.
 * 7. Create the clock label and perform an initial update.
 * 8. Create a 1-second timer to keep the clock updated.
 * 9. Create the `content_area` container which will hold screen-specific widgets.
 * 10. Default to the Home screen by calling `minigui_switch_screen`.
 ******************************************************************************/
void minigui_init(void) {
    // Using LVGL native logging instead of ESP_LOG
    LV_LOG_INFO("MiniGUI: Initializing nested flex layout...");

    minigui_menu_init();

    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

    // 1. MAIN CONTAINER (Vertical Flex Box)
    main_container = lv_obj_create(scr);
    lv_obj_set_size(main_container, lv_pct(100), lv_pct(100));
    lv_obj_set_flex_flow(main_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(main_container, 0, 0);
    lv_obj_set_style_pad_gap(main_container, 0, 0);
    lv_obj_set_style_border_width(main_container, 0, 0);
    lv_obj_set_style_radius(main_container, 0, 0);
    lv_obj_set_style_bg_color(main_container, lv_color_black(), 0);

    // 2. STATUS BAR (Flex Row)
    status_bar = lv_obj_create(main_container);
    lv_obj_set_size(status_bar, lv_pct(100), lv_pct(12));
    lv_obj_set_flex_flow(status_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(status_bar, lv_color_hex(0x202020), 0);
    lv_obj_set_style_border_width(status_bar, 0, 0);
    lv_obj_set_style_radius(status_bar, 0, 0);
    lv_obj_set_style_pad_all(status_bar, 0, 0);
    lv_obj_set_style_pad_right(status_bar, 15, 0); // Padding for the clock on the right
    lv_obj_set_scrollbar_mode(status_bar, LV_SCROLLBAR_MODE_OFF);

    // 3. HAMBURGER BUTTON
    lv_obj_t *btn_menu = lv_button_create(status_bar);
    lv_obj_set_height(btn_menu, lv_pct(100));
    lv_obj_add_event_cb(btn_menu, sync_square_size_cb, LV_EVENT_SIZE_CHANGED, NULL);
    lv_obj_set_style_radius(btn_menu, 0, 0);
    lv_obj_set_style_bg_color(btn_menu, lv_color_hex(0x222222), 0);
    lv_obj_set_style_border_width(btn_menu, 1, 0);
    lv_obj_set_style_border_side(btn_menu, LV_BORDER_SIDE_RIGHT, 0);
    lv_obj_set_style_border_color(btn_menu, lv_color_hex(0x444444), 0);
    lv_obj_set_style_shadow_width(btn_menu, 0, 0);

    lv_obj_t *label_menu = lv_label_create(btn_menu);
    lv_label_set_text(label_menu, LV_SYMBOL_LIST);
    lv_obj_center(label_menu);
    lv_obj_add_event_cb(btn_menu, menu_btn_event_cb, LV_EVENT_CLICKED, NULL);

    // 4. TITLE (Flex grow will push neighbors aside)
    lbl_title = lv_label_create(status_bar);
    lv_label_set_text(lbl_title, "Dashboard");
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(lbl_title, lv_color_white(), 0);
    lv_obj_set_flex_grow(lbl_title, 1);
    lv_obj_set_style_text_align(lbl_title, LV_TEXT_ALIGN_CENTER, 0);

    // 5. CLOCK (Placed after title, will be on the right)
    lbl_clock = lv_label_create(status_bar);
    lv_obj_set_style_text_font(lbl_clock, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_clock, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_width(lbl_clock, 180); // INCREASED WIDTH for longer date format
    lv_obj_set_style_text_align(lbl_clock, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_margin_right(lbl_clock, 10, 0);

    // Initial update
    update_clock_cb(NULL);

    // Create timer for 1s updates
    lv_timer_create(update_clock_cb, 1000, NULL);

    // 6. CONTENT AREA
    content_area = lv_obj_create(main_container);
    lv_obj_set_width(content_area, lv_pct(100));
    lv_obj_set_flex_grow(content_area, 1);
    lv_obj_set_style_bg_color(content_area, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(content_area, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(content_area, 0, 0);
    lv_obj_set_style_radius(content_area, 0, 0);
    lv_obj_set_style_pad_all(content_area, 0, 0);

    minigui_switch_screen(MINIGUI_SCREEN_HOME);
}

/******************************************************************************
 * @brief Switch to a specific screen
 *
 * @section call_site
 * Called internally on init, or by the menu when a user selects a page.
 *
 * @param screen_type The ID of the screen to switch to.
 *
 * @return void
 *
 * Implementation Steps
 * 1. Validate the screen ID and existence of `content_area`.
 * 2. Log the screen switch event.
 * 3. Clear all children from `content_area` to remove the old screen.
 * 4. Reset flex/scroll styles on `content_area`.
 * 5. Update the title label based on the new screen ID.
 * 6. Invoke the specific screen creator function to build the new UI.
 ******************************************************************************/
void minigui_switch_screen(minigui_screen_t screen_type) {
    if (screen_type >= MINIGUI_SCREEN_COUNT || !content_area) return;

    LV_LOG_INFO("MiniGUI: Switching to screen ID %d", screen_type);

    lv_obj_clean(content_area);
    lv_obj_set_style_flex_flow(content_area, 0, 0);
    lv_obj_set_scrollbar_mode(content_area, LV_SCROLLBAR_MODE_AUTO);

    const char *titles[] = {"Home", "System Logs", "Settings"};
    lv_label_set_text(lbl_title, titles[screen_type]);

    if (screen_creators[screen_type]) {
        screen_creators[screen_type](content_area);
    }
}

// --- Bridge Functions ---

/******************************************************************************
 * @brief Register a callback for hardware brightness control
 *
 * @param cb Function to handle brightness changes
 *
 * Implementation Steps
 * 1. Store the provided callback pointer in `brightness_cb`.
 ******************************************************************************/
void minigui_register_brightness_cb(minigui_brightness_cb_t cb) { brightness_cb = cb; }

/******************************************************************************
 * @brief Register a callback for WiFi configuration saving
 *
 * @param cb The function pointer to handle saving credentials
 *
 * Implementation Steps
 * 1. Store the provided callback pointer in `wifi_save_cb`.
 ******************************************************************************/
void minigui_register_wifi_save_cb(minigui_wifi_save_cb_t cb) {
    wifi_save_cb = cb;
}

/******************************************************************************
 * @brief Register a callback for WiFi scanning
 *
 * @param provider The function pointer to perform network scanning
 *
 * Implementation Steps
 * 1. Store the provided callback pointer in `wifi_scan_provider`.
 ******************************************************************************/
void minigui_register_wifi_scan_provider(minigui_wifi_scan_provider_t provider) {
    wifi_scan_provider = provider;
}

/******************************************************************************
 * @brief Register a callback for system statistics monitoring
 *
 * @param provider The function pointer to retrieve system stats
 *
 * Implementation Steps
 * 1. Store the provided callback pointer in `system_stats_provider`.
 ******************************************************************************/
void minigui_register_system_stats_provider(minigui_system_stats_provider_t provider) {
    system_stats_provider = provider;
}

/******************************************************************************
 * @brief Register a callback for network status monitoring
 *
 * @param provider The function pointer to retrieve network status
 *
 * Implementation Steps
 * 1. Store the provided callback pointer in `network_status_provider`.
 ******************************************************************************/
void minigui_register_network_status_provider(minigui_network_status_provider_t provider) {
    network_status_provider = provider;
}

/******************************************************************************
 * @brief Register a time provider for the status bar clock
 *
 * @param provider The function pointer to retrieve formatted time
 *
 * Implementation Steps
 * 1. Store the provider.
 * 2. Immediately update the clock via `update_clock_cb` to reflect new source.
 ******************************************************************************/
void minigui_set_time_provider(minigui_time_provider_t provider) {
    global_time_provider = provider;
    // Update immediately if possible
    if (lbl_clock) update_clock_cb(NULL);
}

/******************************************************************************
 * @brief Set screen brightness (proxies to registered callback)
 *
 * @param brightness Target brightness value (0-255)
 *
 * Implementation Steps
 * 1. Check if a callback is registered.
 * 2. Invoke the callback with the new brightness value.
 ******************************************************************************/
void minigui_set_brightness(uint8_t brightness) { if (brightness_cb) brightness_cb(brightness); }

/******************************************************************************
 * @brief Internal helper to trigger the WiFi save callback.
 *
 * @param creds Pointer to the collected credentials
 *
 * Implementation Steps
 * 1. Check if `wifi_save_cb` is registered.
 * 2. Execute the callback with the credentials.
 ******************************************************************************/
void minigui_save_wifi_credentials(const minigui_wifi_credentials_t *creds) {
    if (wifi_save_cb) wifi_save_cb(creds);
}

/******************************************************************************
 * @brief Internal helper to trigger the WiFi scan.
 *
 * @param networks Output buffer
 * @param max_count Capacity of the buffer
 * @return Number of networks found
 *
 * Implementation Steps
 * 1. If a real provider is registered, delegate to it.
 * 2. Otherwise, use a simulated list of mock networks.
 * 3. Populate simululated networks with mock SSID and RSSI data.
 * 4. Return the number of networks found (max 5).
 ******************************************************************************/
size_t minigui_scan_wifi(minigui_wifi_network_t *networks, size_t max_count) {
    if (wifi_scan_provider) {
        return wifi_scan_provider(networks, max_count);
    }

    // Default Mock Scan Provider (for simulator)
    const char *mock_ssids[] = {"Home_WiFi_2.4G", "Office_Secure", "CoffeeShop_Free", "Starlink_99", "Guest_Lounge"};
    size_t count = (5 < max_count) ? 5 : max_count;

    for (size_t i = 0; i < count; i++) {
        strncpy(networks[i].ssid, mock_ssids[i], sizeof(networks[i].ssid) - 1);
        networks[i].ssid[sizeof(networks[i].ssid) - 1] = '\0';
        networks[i].rssi = -50 - (i * 10); // Simulated RSSI
    }

    return count;
}

/******************************************************************************
 * @brief Internal helper to retrieve system statistics.
 *
 * @param stats Output pointer to fill
 *
 * Implementation Steps
 * 1. If a real provider is registered, delegate to it.
 * 2. Otherwise, fill the stats structure with simulated data.
 * 3. Simulate fluctuating voltage and CPU usage using `lv_tick_get()`.
 ******************************************************************************/
void minigui_get_system_stats(minigui_system_stats_t *stats) {
    if (system_stats_provider) {
        system_stats_provider(stats);
        return;
    }

    // Default Mock Stats Provider (for simulator)
    stats->voltage = 5.0f + ((float)(lv_tick_get() % 100) / 100.0f); // Simulate 5.0-5.1V
    stats->cpu_usage = 15 + (lv_tick_get() % 40); // Simulate 15-55%
    stats->flash_used_kb = 512;
    stats->flash_total_kb = 4096;
    stats->ram_used_kb = 128;
    stats->ram_total_kb = 520;
}

/******************************************************************************
 * @brief Internal helper to retrieve network status.
 *
 * @param status Output pointer to fill
 *
 * Implementation Steps
 * 1. If a real provider is registered, delegate to it.
 * 2. Otherwise, return a hardcoded "Connected" status with mock IP/MAC.
 ******************************************************************************/
void minigui_get_network_status(minigui_network_status_t *status) {
    if (network_status_provider) {
        network_status_provider(status);
        return;
    }

    // Default Mock Network Status (for simulator)
    status->connected = true;
    strncpy(status->ssid, "Home_WiFi_2.4G", sizeof(status->ssid) - 1);
    status->ssid[sizeof(status->ssid) - 1] = '\0';
    strncpy(status->ip_address, "192.168.1.42", sizeof(status->ip_address) - 1);
    status->ip_address[sizeof(status->ip_address) - 1] = '\0';
    strncpy(status->mac_address, "AA:BB:CC:DD:EE:FF", sizeof(status->mac_address) - 1);
    status->mac_address[sizeof(status->mac_address) - 1] = '\0';
}

/******************************************************************************
 * @brief Internal helper to get the main content area (stage).
 *
 * @return Pointer to the main content container
 *
 * Implementation Steps
 * 1. Return the static `content_area` pointer.
 ******************************************************************************/
lv_obj_t *minigui_get_content_area(void) { return content_area; }
