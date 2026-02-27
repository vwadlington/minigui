/******************************************************************************
 ******************************************************************************
 ** @brief     MiniGUI Main UI Manager.
 **
 **            This module implements the core UI lifecycle, including
 **            initialization of the main flex layout, status bar, content area,
 **            and screen switching logic.
 **
 **            @section minigui.c - Main implementation file.
 ******************************************************************************
 ******************************************************************************/

/******************************************************************************
 ******************************************************************************
 ** 1. ESP-IDF / FreeRTOS Core (Framework)
 ******************************************************************************
 ******************************************************************************/
#include <time.h>
#include <stdio.h>
#include <string.h>

/******************************************************************************
 ******************************************************************************
 ** 2. Managed Espressif Components (External Managed Components)
 ******************************************************************************
 ******************************************************************************/
// None directly here, lvgl is included via minigui.h

/******************************************************************************
 ******************************************************************************
 ** 3. Project-Specific Components (Local)
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
 ******************************************************************************
 ** @brief Reference to the main flex container.
 **
 ** @section scope Internal Scope:
 ** - Internal to minigui.c.
 **
 ** @section rationale Rationale:
 ** - Acts as the root parent for all UI elements created by this module.
 ******************************************************************************
 ******************************************************************************/
static lv_obj_t *main_container = NULL;

/******************************************************************************
 ******************************************************************************
 ** @brief Reference to the top status bar.
 **
 ** @section scope Internal Scope:
 ** - Internal to minigui.c.
 **
 ** @section rationale Rationale:
 ** - Holds the menu button, title, and clock.
 ******************************************************************************
 ******************************************************************************/
static lv_obj_t *status_bar = NULL;

/******************************************************************************
 ******************************************************************************
 ** @brief Reference to the dynamic content area.
 **
 ** @section scope Internal Scope:
 ** - Internal to minigui.c.
 **
 ** @section rationale Rationale:
 ** - This is where different screens (Home, Logs, etc.) inject their content.
 ******************************************************************************
 ******************************************************************************/
static lv_obj_t *content_area = NULL;

/******************************************************************************
 ******************************************************************************
 ** @brief Reference to the screen title label.
 **
 ** @section scope Internal Scope:
 ** - Internal to minigui.c.
 **
 ** @section rationale Rationale:
 ** - Updated whenever the screen changes to reflect the current view name.
 ******************************************************************************
 ******************************************************************************/
static lv_obj_t *lbl_title = NULL;

/******************************************************************************
 ******************************************************************************
 ** @brief Reference to the clock label.
 **
 ** @section scope Internal Scope:
 ** - Internal to minigui.c.
 **
 ** @section rationale Rationale:
 ** - Updated every second by a timer to show current time.
 ******************************************************************************
 ******************************************************************************/
static lv_obj_t *lbl_clock = NULL;

// Callback hooks
/******************************************************************************
 ******************************************************************************
 ** @brief Hook for brightness control.
 **
 ** @section scope Internal Scope:
 ** - Internal to minigui.c.
 **
 ** @section rationale Rationale:
 ** - Stores the registered callback to proxy brightness adjustments to hardware.
 ******************************************************************************
 ******************************************************************************/
static minigui_brightness_cb_t brightness_cb = NULL;

/******************************************************************************
 ******************************************************************************
 ** @brief Hook for time retrieval.
 **
 ** @section scope Internal Scope:
 ** - Internal to minigui.c.
 **
 ** @section rationale Rationale:
 ** - Overrides standard C time with a custom provider (e.g., SNTP) if present.
 ******************************************************************************
 ******************************************************************************/
static minigui_time_provider_t global_time_provider = NULL;

/******************************************************************************
 ******************************************************************************
 ** @brief Hook for WiFi credential saving.
 **
 ** @section scope Internal Scope:
 ** - Internal to minigui.c.
 **
 ** @section rationale Rationale:
 ** - Used by the Settings screen to persist network settings.
 ******************************************************************************
 ******************************************************************************/
static minigui_wifi_save_cb_t wifi_save_cb = NULL;

/******************************************************************************
 ******************************************************************************
 ** @brief Hook for WiFi scanning.
 **
 ** @section scope Internal Scope:
 ** - Internal to minigui.c.
 **
 ** @section rationale Rationale:
 ** - Supplies the Settings screen with a list of available networks.
 ******************************************************************************
 ******************************************************************************/
static minigui_wifi_scan_provider_t wifi_scan_provider = NULL;

/******************************************************************************
 ******************************************************************************
 ** @brief Hook for system statistics retrieval.
 **
 ** @section scope Internal Scope:
 ** - Internal to minigui.c.
 **
 ** @section rationale Rationale:
 ** - Provides data for the monitor dashboard (CPU, RAM, Flash).
 ******************************************************************************
 ******************************************************************************/
static minigui_system_stats_provider_t system_stats_provider = NULL;

/******************************************************************************
 ******************************************************************************
 ** @brief Hook for network status retrieval.
 **
 ** @section scope Internal Scope:
 ** - Internal to minigui.c.
 **
 ** @section rationale Rationale:
 ** - Provides connection details (IP, MAC) for the Settings screen.
 ******************************************************************************
 ******************************************************************************/
static minigui_network_status_provider_t network_status_provider = NULL;

/******************************************************************************
 ******************************************************************************
 ** @brief Mapping of screen IDs to their creator functions.
 **
 ** @section scope Internal Scope:
 ** - Internal to minigui.c.
 **
 ** @section rationale Rationale:
 ** - Decouples screen switching logic from individual implementations.
 ******************************************************************************
 ******************************************************************************/
static const ui_screen_creator_t screen_creators[MINIGUI_SCREEN_COUNT] = {
    [MINIGUI_SCREEN_HOME]     = create_screen_home,
    [MINIGUI_SCREEN_LOGS]     = create_screen_logs,
    [MINIGUI_SCREEN_SETTINGS] = create_screen_settings
};

// ============================================================================
// PRIVATE HELPER FUNCTIONS
// ============================================================================

/******************************************************************************
 ******************************************************************************
 ** @brief Updates the clock label with current system time.
 **
 ** @section call_site Called from:
 ** - lv_timer every 1000ms.
 ** - minigui_set_time_provider() for an immediate update.
 **
 ** @section dependencies Required Headers:
 ** - time.h (for standard time functions when no provider is set)
 **
 ** @param timer (lv_timer_t*): The LVGL timer instance triggering this callback.
 **
 ** @section pointers 
 ** - timer: Owned by LVGL, can be NULL if called manually.
 **
 ** @section variables Internal Variables:
 ** - @c buf (char[32]): Storage for the formatted time string.
 ** - @c now (time_t): Epoch time from system clock.
 ** - @c tm_info (struct tm*): Broken down time structure.
 **
 ** @return void
 **
 ** Implementation Steps:
 ** 1. Check if the clock label exists; return if not.
 ** 2. If a global time provider is registered, use it to populate the buffer.
 ** 3. Otherwise, fall back to standard C time and localtime.
 ** 4. Format the time as "%a %m/%d %H:%M:%S".
 ** 5. Update the LVGL label text.
 ******************************************************************************
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
 ******************************************************************************
 ** @brief Event wrapper to open the menu.
 **
 ** @section call_site Called from:
 ** - Hamburger button's LV_EVENT_CLICKED.
 **
 ** @section dependencies Required Headers:
 ** - minigui_menu.h (for minigui_menu_toggle)
 **
 ** @param e (lv_event_t*): LVGL event object.
 **
 ** @section pointers 
 ** - e: Owned by LVGL.
 **
 ** @section variables 
 ** - None
 **
 ** @return void
 **
 ** Implementation Steps:
 ** 1. Log the user interaction using LV_LOG_USER.
 ** 2. Call minigui_menu_toggle() to show/hide the sidebar.
 ******************************************************************************
 ******************************************************************************/
static void menu_btn_event_cb(lv_event_t * e) {
    LV_LOG_USER("Hamburger menu toggled");
    minigui_menu_toggle();
}

/******************************************************************************
 ******************************************************************************
 ** @brief Ensures the hamburger button is a perfect square.
 **
 ** @section call_site Called from:
 ** - Hamburger button's LV_EVENT_SIZE_CHANGED.
 **
 ** @section dependencies Required Headers:
 ** - lvgl.h (for object geometry manipulation)
 **
 ** @param e (lv_event_t*): LVGL event object.
 **
 ** @section pointers 
 ** - e: Owned by LVGL.
 **
 ** @section variables Internal Variables:
 ** - @c btn (lv_obj_t*): The button object being resized.
 ** - @c parent (lv_obj_t*): The status bar parent.
 ** - @c h (int32_t): Height of the parent status bar.
 **
 ** @return void
 **
 ** Implementation Steps:
 ** 1. Retrieve the button object from the event.
 ** 2. Retrieve the parent status bar.
 ** 3. Get the parent's height.
 ** 4. Set the button's width equal to that height to ensure a 1:1 ratio.
 ******************************************************************************
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
 ******************************************************************************
 * @brief Initialize the UI manager dynamically and set up flex layouts.
 *
 * @section call_site
 * Called from `app_main` or similar entry point.
 *
 * @section dependencies
 * - `minigui_menu.h`: For menu initialization.
 * - `lvgl`: For all UI creation and thread-safe locks (`lv_lock`).
 *
 * @param None
 *
 * @section pointers
 * - None
 *
 * @section variables
 * - `scr`: Pointer to the active LVGL screen. Rationale: Root parent for UI elements.
 * - `btn_menu`: Pointer to hamburger button. Rationale: Toggles the sideline menu.
 * - `label_menu`: Pointer to hamburger icon label. Rationale: Displays LV_SYMBOL_LIST.
 *
 * @return void
 *
 * Implementation Steps
 * 1. Log initialization start.
 * 2. Acquire LVGL lock (`lv_lock`) for thread safety.
 * 3. Initialize the side menu system.
 * 4. Configure the active screen background to black.
 * 5. Create the `main_container` with a vertical flex layout to hold status bar and content.
 * 6. Create the `status_bar` with a horizontal flex layout.
 * 7. Create the hamburger button and attach the square-size sync callback.
 * 8. Create the title label with flex-grow to push the clock to the right.
 * 9. Create the clock label and perform an initial update.
 * 10. Create a 1-second timer to keep the clock updated.
 * 11. Create the `content_area` container which will hold screen-specific widgets.
 * 12. Release LVGL lock (`lv_unlock`).
 * 13. Default to the Home screen by calling `minigui_switch_screen`.
 ******************************************************************************/
void minigui_init(void) {
    // Using LVGL native logging instead of ESP_LOG
    LV_LOG_INFO("MiniGUI: Initializing nested flex layout...");

    lv_lock();

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
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_color(lbl_title, lv_color_white(), 0);
    lv_obj_set_flex_grow(lbl_title, 1);
    lv_obj_set_style_text_align(lbl_title, LV_TEXT_ALIGN_CENTER, 0);

    // 5. CLOCK (Placed after title, will be on the right)
    lbl_clock = lv_label_create(status_bar);
    lv_obj_set_style_text_font(lbl_clock, &lv_font_montserrat_20, 0);
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

    lv_unlock();

    minigui_switch_screen(MINIGUI_SCREEN_HOME);
}

/******************************************************************************
 ******************************************************************************
 ** @brief Switch the active screen rendered in the main content area.
 **
 ** @section call_site Called from:
 ** - minigui_init() for initial screen set.
 ** - Menu system interaction.
 ** - External navigation triggers.
 **
 ** @section dependencies Required Headers:
 ** - lvgl.h (for UI cleanup and creation)
 **
 ** @param screen_type (minigui_screen_t): The ID of the screen to switch to.
 **
 ** @section pointers 
 ** - None
 **
 ** @section variables Internal Variables:
 ** - @c titles (const char*[]): Array of screen display titles.
 **
 ** @return void
 **
 ** Implementation Steps:
 ** 1. Validate the screen ID and existence of content_area.
 ** 2. Log the screen switch event.
 ** 3. Acquire LVGL lock (lv_lock).
 ** 4. Clear all children from content_area.
 ** 5. Reset scroll and flex properties on content_area.
 ** 6. Update the title label text.
 ** 7. Invoke the creator function for the requested screen if it exists.
 ** 8. Release LVGL lock (lv_unlock).
 ******************************************************************************
 ******************************************************************************/
void minigui_switch_screen(minigui_screen_t screen_type) {
    if (screen_type >= MINIGUI_SCREEN_COUNT || !content_area) return;

    LV_LOG_INFO("MiniGUI: Switching to screen ID %d", screen_type);

    lv_lock();
    lv_obj_clean(content_area);
    lv_obj_set_style_flex_flow(content_area, 0, 0);
    lv_obj_set_scrollbar_mode(content_area, LV_SCROLLBAR_MODE_AUTO);

    const char *titles[] = {"Home", "System Logs", "Settings"};
    lv_label_set_text(lbl_title, titles[screen_type]);

    if (screen_creators[screen_type]) {
        screen_creators[screen_type](content_area);
    }
    lv_unlock();
}

/******************************************************************************
 ******************************************************************************
 ** @brief Register a callback for hardware brightness control.
 **
 ** @section call_site Called from:
 ** - System initialization to link UI to PWM driver.
 **
 ** @section dependencies Required Headers:
 ** - None
 **
 ** @param cb (minigui_brightness_cb_t): Function to handle brightness changes.
 **
 ** @section pointers 
 ** - cb: Function pointer owned by caller.
 **
 ** @section variables 
 ** - None
 **
 ** @return void
 **
 ** Implementation Steps:
 ** 1. Store the provided callback pointer in the internal @c brightness_cb variable.
 ******************************************************************************
 ******************************************************************************/
void minigui_register_brightness_cb(minigui_brightness_cb_t cb) { brightness_cb = cb; }

/******************************************************************************
 ******************************************************************************
 ** @brief Register a callback for WiFi configuration saving.
 **
 ** @section call_site Called from:
 ** - System initialization to handle NVS persistence.
 **
 ** @section dependencies Required Headers:
 ** - None
 **
 ** @param cb (minigui_wifi_save_cb_t): Callback for credential saving.
 **
 ** @section pointers 
 ** - cb: Function pointer owned by caller.
 **
 ** @section variables 
 ** - None
 **
 ** @return void
 **
 ** Implementation Steps:
 ** 1. Store the provided callback pointer in the internal @c wifi_save_cb variable.
 ******************************************************************************
 ******************************************************************************/
void minigui_register_wifi_save_cb(minigui_wifi_save_cb_t cb) {
    wifi_save_cb = cb;
}

/******************************************************************************
 ******************************************************************************
 ** @brief Register a callback for WiFi scanning.
 **
 ** @section call_site Called from:
 ** - System initialization to enable network discovery in Settings.
 **
 ** @section dependencies Required Headers:
 ** - None
 **
 ** @param provider (minigui_wifi_scan_provider_t): Callback for network scanning.
 **
 ** @section pointers 
 ** - provider: Function pointer owned by caller.
 **
 ** @section variables 
 ** - None
 **
 ** @return void
 **
 ** Implementation Steps:
 ** 1. Store the provided callback pointer in the internal @c wifi_scan_provider variable.
 ******************************************************************************
 ******************************************************************************/
void minigui_register_wifi_scan_provider(minigui_wifi_scan_provider_t provider) {
    wifi_scan_provider = provider;
}

/******************************************************************************
 ******************************************************************************
 ** @brief Register a callback for system statistics monitoring.
 **
 ** @section call_site Called from:
 ** - System initialization to provide health data to the UI.
 **
 ** @section dependencies Required Headers:
 ** - None
 **
 ** @param provider (minigui_system_stats_provider_t): Callback for stats retrieval.
 **
 ** @section pointers 
 ** - provider: Function pointer owned by caller.
 **
 ** @section variables 
 ** - None
 **
 ** @return void
 **
 ** Implementation Steps:
 ** 1. Store the provided callback pointer in the internal @c system_stats_provider variable.
 ******************************************************************************
 ******************************************************************************/
void minigui_register_system_stats_provider(minigui_system_stats_provider_t provider) {
    system_stats_provider = provider;
}

/******************************************************************************
 ******************************************************************************
 ** @brief Register a callback for network status monitoring.
 **
 ** @section call_site Called from:
 ** - System initialization to provide IP/MAC info.
 **
 ** @section dependencies Required Headers:
 ** - None
 **
 ** @param provider (minigui_network_status_provider_t): Callback for status retrieval.
 **
 ** @section pointers 
 ** - provider: Function pointer owned by caller.
 **
 ** @section variables 
 ** - None
 **
 ** @return void
 **
 ** Implementation Steps:
 ** 1. Store the provided callback pointer in the internal @c network_status_provider variable.
 ******************************************************************************
 ******************************************************************************/
void minigui_register_network_status_provider(minigui_network_status_provider_t provider) {
    network_status_provider = provider;
}

/******************************************************************************
 ******************************************************************************
 * @brief Register a time provider for the status bar clock.
 *
 * @section call_site
 * Called during initialization to provide real-time clock data (e.g., from SNTP).
 *
 * @section dependencies
 * - `lvgl`: For thread-safe locking.
 *
 * @param provider The function pointer to retrieve formatted time.
 *
 * @section pointers
 * - `provider`: Owned by caller, callback function pointer.
 *
 * @section variables
 * - None
 *
 * @return void
 *
 * Implementation Steps
 * 1. Store the provider globally.
 * 2. Acquire LVGL lock (`lv_lock`).
 * 3. Immediately update the clock via `update_clock_cb` to reflect new source.
 * 4. Release LVGL lock (`lv_unlock`).
 ******************************************************************************/
void minigui_set_time_provider(minigui_time_provider_t provider) {
    global_time_provider = provider;
    // Update immediately if possible
    lv_lock();
    if (lbl_clock) update_clock_cb(NULL);
    lv_unlock();
}

/******************************************************************************
 ******************************************************************************
 ** @brief Set screen brightness (proxies to registered callback).
 **
 ** @section call_site Called from:
 ** - Settings screen brightness slider.
 **
 ** @section dependencies Required Headers:
 ** - None
 **
 ** @param brightness (uint8_t): Target brightness value (0-255).
 **
 ** @section pointers 
 ** - None
 **
 ** @section variables 
 ** - None
 **
 ** @return void
 **
 ** Implementation Steps:
 ** 1. Check if a callback is registered in @c brightness_cb.
 ** 2. If present, invoke it with the new brightness value.
 ******************************************************************************
 ******************************************************************************/
void minigui_set_brightness(uint8_t brightness) { if (brightness_cb) brightness_cb(brightness); }

/******************************************************************************
 ******************************************************************************
 ** @brief Internal helper to trigger the WiFi save callback.
 **
 ** @section call_site Called from:
 ** - Settings screen WiFi panel submission.
 **
 ** @section dependencies Required Headers:
 ** - None
 **
 ** @param creds (const minigui_wifi_credentials_t*): Collected credentials.
 **
 ** @section pointers 
 ** - creds: Pointer to static or local credentials structure.
 **
 ** @section variables 
 ** - None
 **
 ** @return void
 **
 ** Implementation Steps:
 ** 1. Check if @c wifi_save_cb is registered.
 ** 2. Execute the callback with the provided credentials.
 ******************************************************************************
 ******************************************************************************/
void minigui_save_wifi_credentials(const minigui_wifi_credentials_t *creds) {
    if (wifi_save_cb) wifi_save_cb(creds);
}

/******************************************************************************
 ******************************************************************************
 ** @brief Internal helper to trigger a WiFi scan.
 **
 ** @section call_site Called from:
 ** - Settings screen to populate network list.
 **
 ** @section dependencies Required Headers:
 ** - string.h (for strncpy)
 **
 ** @param networks (minigui_wifi_network_t*): Output buffer for results.
 ** @param max_count (size_t): Capacity of the buffer.
 **
 ** @section pointers 
 ** - networks: Pointer to buffer owned by caller.
 **
 ** @section variables Internal Variables:
 ** - @c mock_ssids (const char*[]): Simulated network names.
 ** - @c count (size_t): Number of results to return.
 **
 ** @return size_t: Number of networks actually found.
 **
 ** Implementation Steps:
 ** 1. If a real provider is registered, delegate the scan to it.
 ** 2. Otherwise, use simulated mock network data.
 ** 3. Populate caller buffer with mock SSIDs and RSSIs.
 ** 4. Return the result count.
 ******************************************************************************
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
 ******************************************************************************
 ** @brief Internal helper to retrieve system statistics.
 **
 ** @section call_site Called from:
 ** - Monitor panel to refresh dashboard data.
 **
 ** @section dependencies Required Headers:
 ** - lvgl.h (for lv_tick_get simulation)
 **
 ** @param stats (minigui_system_stats_t*): Output pointer to fill.
 **
 ** @section pointers 
 ** - stats: Pointer to struct owned by caller.
 **
 ** @section variables 
 ** - None
 **
 ** @return void
 **
 ** Implementation Steps:
 ** 1. If a real provider is registered, delegate the request.
 ** 2. Otherwise, fill the structure with mock data fluctuating based on ticks.
 ******************************************************************************
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
 ******************************************************************************
 ** @brief Internal helper to retrieve network status.
 **
 ** @section call_site Called from:
 ** - Network panel to display IP/MAC/SSID.
 **
 ** @section dependencies Required Headers:
 ** - string.h (for strncpy)
 **
 ** @param status (minigui_network_status_t*): Output pointer to fill.
 **
 ** @section pointers 
 ** - status: Pointer to struct owned by caller.
 **
 ** @section variables 
 ** - None
 **
 ** @return void
 **
 ** Implementation Steps:
 ** 1. If a real provider is registered, delegate the request.
 ** 2. Otherwise, populate with mock "Connected" status.
 ******************************************************************************
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
 ******************************************************************************
 ** @brief Internal helper to get the main content area (stage).
 **
 ** @section call_site Called from:
 ** - Screen creators to attach widgets.
 **
 ** @section dependencies Required Headers:
 ** - lvgl.h (for lv_obj_t handle)
 **
 ** @param None
 **
 ** @section pointers 
 ** - None
 **
 ** @return lv_obj_t*: Pointer to the static content area container.
 **
 ** Implementation Steps:
 ** 1. Return the static @c content_area pointer.
 ******************************************************************************
 ******************************************************************************/
lv_obj_t *minigui_get_content_area(void) { return content_area; }
