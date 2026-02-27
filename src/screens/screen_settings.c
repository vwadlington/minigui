/******************************************************************************
 ******************************************************************************
 ** @brief     Settings Screen Implementation.
 **
 **            Provides a multi-category settings interface (Screen, Network,
 **            System, Monitor) with a split-pane layout and on-screen keyboard.
 **
 **            @section screen_settings.c - Settings UI implementation.
 ******************************************************************************
 ******************************************************************************/

/******************************************************************************
 ******************************************************************************
 ** 1. ESP-IDF / FreeRTOS Core (Framework)
 ******************************************************************************
 ******************************************************************************/
#include <string.h>
#include <stdio.h>

/******************************************************************************
 ******************************************************************************
 ** 2. Managed Espressif Components (External Managed Components)
 ******************************************************************************
 ******************************************************************************/
// None (lvgl included via header)

/******************************************************************************
 ******************************************************************************
 ** 3. Project-Specific Components (Local)
 ******************************************************************************
 ******************************************************************************/
#include "screens/screen_settings.h"
#include "minigui.h"

// ============================================================================
//  TYPES & STATE
// ============================================================================

/**
 * @brief Enumeration of available settings sub-categories
 */
typedef enum {
    SETTINGS_CAT_SCREEN = 0,
    SETTINGS_CAT_NETWORK,
    SETTINGS_CAT_SYSTEM,
    SETTINGS_CAT_MONITOR,
    SETTINGS_CAT_COUNT
} settings_category_t;

/******************************************************************************
 ******************************************************************************
 * FILE GLOBALS
 ******************************************************************************
 ******************************************************************************/

/******************************************************************************
 ******************************************************************************
 ** @brief Dynamic content container for the right side of the split view.
 **
 ** @section scope Internal Scope:
 ** - Internal to screen_settings.c.
 **
 ** @section rationale Rationale:
 ** - Cleared and repopulated when switching categories.
 ******************************************************************************
 ******************************************************************************/
static lv_obj_t *content_pane = NULL;

/******************************************************************************
 ******************************************************************************
 ** @brief Shared on-screen keyboard.
 **
 ** @section scope Internal Scope:
 ** - Internal to screen_settings.c.
 **
 ** @section rationale Rationale:
 ** - Reused across any input fields in the settings screen.
 ******************************************************************************
 ******************************************************************************/
static lv_obj_t *kb = NULL;

// UI References for Network Panel
static lv_obj_t *dd_ssid = NULL;
static lv_obj_t *ta_pass = NULL;
static lv_obj_t *btn_scan = NULL;
static lv_obj_t *lbl_scan = NULL;

// UI References for Monitor Panel
static lv_timer_t *monitor_timer = NULL;
static lv_obj_t *lbl_voltage = NULL;
static lv_obj_t *lbl_cpu = NULL;
static lv_obj_t *lbl_flash = NULL;
static lv_obj_t *lbl_ram = NULL;

// UI References for System Panel
static lv_obj_t *lbl_fw_version = NULL;
static lv_obj_t *lbl_fw_status = NULL;
static lv_obj_t *btn_fw_update = NULL;
static bool update_available = false;

// State
static settings_category_t current_category = SETTINGS_CAT_SCREEN;

// ============================================================================
//  FORWARD DECLARATIONS
// ============================================================================

static void create_screen_panel(lv_obj_t *parent);
static void create_network_panel(lv_obj_t *parent);
static void create_system_panel(lv_obj_t *parent);
static void create_monitor_panel(lv_obj_t *parent);

// ============================================================================
//  SHARED EVENT HANDLERS (WiFi, Keyboard)
// ============================================================================

/******************************************************************************
 ******************************************************************************
 ** @brief Handle keyboard events (close on OK/Cancel).
 **
 ** @section call_site Called from:
 ** - Virtual keyboard (LV_EVENT_READY or LV_EVENT_CANCEL).
 **
 ** @section dependencies Required Headers:
 ** - lvgl.h (keyboard API)
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
 ** 1. Check if the "Done" (Ready) or "Cancel" button was pressed.
 ** 2. Hide the keyboard object.
 ** 3. Remove focus state from the associated text area (@c ta_pass).
 ******************************************************************************
 ******************************************************************************/
static void kb_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * keyboard = lv_event_get_target(e);

    if(code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
        if(ta_pass) lv_obj_remove_state(ta_pass, LV_STATE_FOCUSED);
    }
}

/******************************************************************************
 ******************************************************************************
 ** @brief Handle text area focus to show keyboard.
 **
 ** @section call_site Called from:
 ** - Text area (LV_EVENT_FOCUSED).
 **
 ** @section dependencies Required Headers:
 ** - lvgl.h (textarea and keyboard linking)
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
 ** 1. Link the global keyboard (@c kb) to the focused text area.
 ** 2. Unhide the keyboard.
 ******************************************************************************
 ******************************************************************************/
static void ta_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);

    if(code == LV_EVENT_FOCUSED) {
        if(kb) {
            lv_keyboard_set_textarea(kb, ta);
            lv_obj_remove_flag(kb, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

/******************************************************************************
 ******************************************************************************
 ** @brief Handle "Scan" button click for WiFi discovery.
 **
 ** @section call_site Called from:
 ** - Scan button LV_EVENT_CLICKED.
 **
 ** @section dependencies Required Headers:
 ** - minigui.h (for minigui_scan_wifi)
 **
 ** @param e (lv_event_t*): LVGL event object.
 **
 ** @section pointers 
 ** - e: Owned by LVGL.
 **
 ** @section variables Internal Variables:
 ** - @c networks (minigui_wifi_network_t[10]): Temp storage for scan results.
 **
 ** @return void
 **
 ** Implementation Steps:
 ** 1. Set UI state to "Scanning..." and disable button.
 ** 2. Trigger @c minigui_scan_wifi to fetch nearby networks.
 ** 3. Populate the SSID dropdown (@c dd_ssid) with found SSIDs.
 ** 4. Restore UI state (renable button).
 ******************************************************************************
 ******************************************************************************/
static void scan_wifi_event_cb(lv_event_t * e) {
    LV_LOG_USER("Scanning for WiFi networks...");
    lv_label_set_text(lbl_scan, "Scanning...");
    lv_obj_add_state(btn_scan, LV_STATE_DISABLED);
    lv_timer_handler();

    minigui_wifi_network_t networks[10];
    size_t count = minigui_scan_wifi(networks, 10);

    lv_dropdown_clear_options(dd_ssid);
    for (size_t i = 0; i < count; i++) {
        lv_dropdown_add_option(dd_ssid, networks[i].ssid, i);
    }

    if (count > 0) {
        lv_dropdown_set_selected(dd_ssid, 0);
    } else {
        lv_dropdown_add_option(dd_ssid, "No networks found", 0);
    }

    lv_label_set_text(lbl_scan, "Scan");
    lv_obj_remove_state(btn_scan, LV_STATE_DISABLED);
    LV_LOG_USER("Scan complete, found %d networks", (int)count);
}

/******************************************************************************
 ******************************************************************************
 ** @brief Handle "Save WiFi" button click.
 **
 ** @section call_site Called from:
 ** - Save button LV_EVENT_CLICKED.
 **
 ** @section dependencies Required Headers:
 ** - minigui.h (for saving credentials)
 **
 ** @param e (lv_event_t*): LVGL event object.
 **
 ** @section pointers 
 ** - e: Owned by LVGL.
 **
 ** @section variables Internal Variables:
 ** - @c creds (minigui_wifi_credentials_t): Buffer for SSID/Pass.
 **
 ** @return void
 **
 ** Implementation Steps:
 ** 1. Extract selected SSID from dropdown and text from password area.
 ** 2. Validate that a real network is selected.
 ** 3. Pass credentials structure to the bridge saver.
 ******************************************************************************
 ******************************************************************************/
static void save_wifi_event_cb(lv_event_t * e) {
    minigui_wifi_credentials_t creds;

    char ssid_buf[64];
    lv_dropdown_get_selected_str(dd_ssid, ssid_buf, sizeof(ssid_buf));
    strncpy(creds.ssid, ssid_buf, sizeof(creds.ssid) - 1);
    creds.ssid[sizeof(creds.ssid) - 1] = '\0';

    strncpy(creds.password, lv_textarea_get_text(ta_pass), sizeof(creds.password) - 1);
    creds.password[sizeof(creds.password) - 1] = '\0';

    if (strcmp(creds.ssid, "No networks found") == 0 || strlen(creds.ssid) == 0 || strcmp(creds.ssid, "Scan to see networks...") == 0) {
        LV_LOG_USER("Cannot save: No valid SSID selected");
        return;
    }

    LV_LOG_USER("Saving WiFi: SSID='%s'", creds.ssid);
    minigui_save_wifi_credentials(&creds);
}

// ============================================================================
//  UI HELPERS
// ============================================================================

/******************************************************************************
 ******************************************************************************
 ** @brief Helper to create a horizontal separator line.
 **
 ** @section call_site Called from:
 ** - Various panel builders to segment UI areas.
 **
 ** @section dependencies Required Headers:
 ** - lvgl.h (basic object styling)
 **
 ** @param parent (lv_obj_t*): Parent container.
 ** @param margin_top (uint16_t): Space above the line.
 ** @param margin_bottom (uint16_t): Space below the line.
 **
 ** @section pointers 
 ** - parent: Managed by caller.
 **
 ** @section variables 
 ** - None
 **
 ** @return void
 **
 ** Implementation Steps:
 ** 1. Create a 1px tall object spanning 100% width.
 ** 2. Configure background color (hex 0x555555) and cover opacity.
 ** 3. Apply the specified vertical margins.
 ******************************************************************************
 ******************************************************************************/
static void create_separator(lv_obj_t *parent, uint16_t margin_top, uint16_t margin_bottom) {
    lv_obj_t *sep = lv_obj_create(parent);
    lv_obj_set_size(sep, lv_pct(100), 1);
    lv_obj_set_style_bg_color(sep, lv_color_hex(0x555555), 0);
    lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sep, 0, 0);
    lv_obj_set_style_margin_top(sep, margin_top, 0);
    lv_obj_set_style_margin_bottom(sep, margin_bottom, 0);
}

// ============================================================================
//  CATEGORY PANEL BUILDERS
// ============================================================================

/******************************************************************************
 ******************************************************************************
 ** @brief Handle brilliance slider change.
 **
 ** @section call_site Called from:
 ** - Brightness slider LV_EVENT_VALUE_CHANGED.
 **
 ** @section dependencies Required Headers:
 ** - minigui.h (for brightness bridge)
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
 ** 1. Extract the current percentage value from the slider.
 ** 2. Forward the value to @c minigui_set_brightness.
 ******************************************************************************
 ******************************************************************************/
static void slider_event_cb(lv_event_t * e) {
    lv_obj_t * slider = lv_event_get_target(e);
    int brightness = (int)lv_slider_get_value(slider);
    LV_LOG_USER("Brightness changed to %d%%", brightness);
    minigui_set_brightness(brightness);
}

/******************************************************************************
 ******************************************************************************
 ** @brief Create the "Screen" settings panel.
 **
 ** @section call_site Called from:
 ** - switch_category() when Screen category is selected.
 **
 ** @section dependencies Required Headers:
 ** - lvgl.h (widgets and layout)
 **
 ** @param parent (lv_obj_t*): The content pane container.
 **
 ** @section pointers 
 ** - parent: Owned by screen_settings.
 **
 ** @section variables 
 ** - None
 **
 ** @return void
 **
 ** Implementation Steps:
 ** 1. Create Display Settings header label.
 ** 2. Create Brightness label and Slider widget.
 ** 3. Attach slider_event_cb for real-time updates.
 ******************************************************************************
 ******************************************************************************/
static void create_screen_panel(lv_obj_t *parent) {
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, "Display Settings");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_24, 0);
    lv_obj_set_style_margin_bottom(lbl, 15, 0);

    lv_obj_t *lbl_bright = lv_label_create(parent);
    lv_label_set_text(lbl_bright, "Screen Brightness");

    lv_obj_t *slider = lv_slider_create(parent);
    lv_obj_set_width(slider, lv_pct(100));
    lv_slider_set_value(slider, 70, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

/******************************************************************************
 * @brief Create the "Network" settings panel.
 *
 * @param parent Content pane.
 *
 * Implementation Steps
 * 1. Display current connection status (SSID/IP or "Disconnected").
 * 2. Add Scan button and SSID dropdown.
 * 3. Add Password field.
 * 4. Add Save button.
 ******************************************************************************/
static void create_network_panel(lv_obj_t *parent) {
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, "Network Configuration");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_margin_bottom(lbl, 15, 0);

    // Current Network Status Section
    lv_obj_t *lbl_status_hdr = lv_label_create(parent);
    lv_label_set_text(lbl_status_hdr, "Current Connection");
    lv_obj_set_style_text_font(lbl_status_hdr, &lv_font_montserrat_20, 0);
    lv_obj_set_style_margin_bottom(lbl_status_hdr, 8, 0);

    minigui_network_status_t net_status;
    minigui_get_network_status(&net_status);

    char status_buf[128];
    if (net_status.connected) {
        lv_obj_t *lbl_ssid_status = lv_label_create(parent);
        snprintf(status_buf, sizeof(status_buf), LV_SYMBOL_WIFI " SSID: %s", net_status.ssid);
        lv_label_set_text(lbl_ssid_status, status_buf);
        lv_obj_set_style_margin_bottom(lbl_ssid_status, 5, 0);

        lv_obj_t *lbl_ip = lv_label_create(parent);
        snprintf(status_buf, sizeof(status_buf), "IP Address: %s", net_status.ip_address);
        lv_label_set_text(lbl_ip, status_buf);
        lv_obj_set_style_margin_bottom(lbl_ip, 5, 0);

        lv_obj_t *lbl_mac = lv_label_create(parent);
        snprintf(status_buf, sizeof(status_buf), "MAC Address: %s", net_status.mac_address);
        lv_label_set_text(lbl_mac, status_buf);
        lv_obj_set_style_margin_bottom(lbl_mac, 5, 0);
    } else {
        lv_obj_t *lbl_disconnected = lv_label_create(parent);
        lv_label_set_text(lbl_disconnected, LV_SYMBOL_CLOSE " Not connected");
        lv_obj_set_style_margin_bottom(lbl_disconnected, 5, 0);
    }

    // Separator before scan section
    create_separator(parent, 15, 15);

    // WiFi Scan & Connect Section
    lv_obj_t *lbl_connect_hdr = lv_label_create(parent);
    lv_label_set_text(lbl_connect_hdr, "Connect to Network");
    lv_obj_set_style_text_font(lbl_connect_hdr, &lv_font_montserrat_20, 0);
    lv_obj_set_style_margin_bottom(lbl_connect_hdr, 8, 0);

    // SSID Selection with Scan
    lv_obj_t *lbl_ssid = lv_label_create(parent);
    lv_label_set_text(lbl_ssid, "WiFi Network (SSID)");

    lv_obj_t *ssid_row = lv_obj_create(parent);
    lv_obj_set_size(ssid_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(ssid_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ssid_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(ssid_row, 0, 0);
    lv_obj_set_style_border_width(ssid_row, 0, 0);
    lv_obj_set_style_pad_all(ssid_row, 0, 0);
    lv_obj_set_style_pad_gap(ssid_row, 10, 0);

    dd_ssid = lv_dropdown_create(ssid_row);
    lv_obj_set_flex_grow(dd_ssid, 1);
    lv_dropdown_set_options(dd_ssid, "Scan to see networks...");

    btn_scan = lv_button_create(ssid_row);
    lbl_scan = lv_label_create(btn_scan);
    lv_label_set_text(lbl_scan, "Scan");
    lv_obj_set_style_text_font(lbl_scan, &lv_font_montserrat_20, 0);
    lv_obj_add_event_cb(btn_scan, scan_wifi_event_cb, LV_EVENT_CLICKED, NULL);

    // Password
    lv_obj_t *lbl_pass = lv_label_create(parent);
    lv_label_set_text(lbl_pass, "Password");
    lv_obj_set_style_margin_top(lbl_pass, 15, 0);

    ta_pass = lv_textarea_create(parent);
    lv_obj_set_width(ta_pass, lv_pct(100));
    lv_textarea_set_one_line(ta_pass, true);
    lv_textarea_set_password_mode(ta_pass, true);
    lv_textarea_set_placeholder_text(ta_pass, "Enter Password...");
    lv_obj_add_event_cb(ta_pass, ta_event_cb, LV_EVENT_FOCUSED, NULL);

    // Save Button
    lv_obj_t *btn_save = lv_button_create(parent);
    lv_obj_set_style_margin_top(btn_save, 20, 0);
    lv_obj_t *lbl_save = lv_label_create(btn_save);
    lv_label_set_text(lbl_save, "Save WiFi");
    lv_obj_set_style_text_font(lbl_save, &lv_font_montserrat_20, 0);
    lv_obj_add_event_cb(btn_save, save_wifi_event_cb, LV_EVENT_CLICKED, NULL);
}

static void reboot_event_cb(lv_event_t * e) {
    LV_LOG_USER("Reboot requested (placeholder)");
}

static void firmware_update_event_cb(lv_event_t * e) {
    LV_LOG_USER("Firmware update requested (placeholder)");
}

/******************************************************************************
 ******************************************************************************
 ** @brief Handle "Check for Updates" button click.
 **
 ** @section call_site Called from:
 ** - Check Updates button LV_EVENT_CLICKED.
 **
 ** @section dependencies Required Headers:
 ** - None (Simulation)
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
 ** 1. Simulate a network check.
 ** 2. Toggle the visibility of the "Install Update" button based on result.
 ** 3. Update status text label.
 ******************************************************************************
 ******************************************************************************/
static void check_firmware_event_cb(lv_event_t * e) {
    LV_LOG_USER("Checking for firmware updates...");
    lv_label_set_text(lbl_fw_status, "Checking for updates...");

    // Simulate update check (in real app, this would query a server)
    // For demo purposes, alternate between "up to date" and "update available"
    static bool mock_update_found = false;
    mock_update_found = !mock_update_found;

    if (mock_update_found) {
        update_available = true;
        lv_label_set_text(lbl_fw_status, LV_SYMBOL_WARNING " Update available: v1.2.0");
        lv_obj_remove_flag(btn_fw_update, LV_OBJ_FLAG_HIDDEN);
    } else {
        update_available = false;
        lv_label_set_text(lbl_fw_status, LV_SYMBOL_OK " Firmware is up to date");
        lv_obj_add_flag(btn_fw_update, LV_OBJ_FLAG_HIDDEN);
    }
}

/******************************************************************************
 ******************************************************************************
 ** @brief Create the "System" settings panel.
 **
 ** @section call_site Called from:
 ** - switch_category() when System category is selected.
 **
 ** @section dependencies Required Headers:
 ** - lvgl.h (layout and control widgets)
 **
 ** @param parent (lv_obj_t*): The content pane container.
 **
 ** @section pointers 
 ** - parent: Owned by screen_settings.
 **
 ** @section variables 
 ** - None
 **
 ** @return void
 **
 ** Implementation Steps:
 ** 1. Add Reboot button at the top.
 ** 2. Construct Firmware section with version info and action buttons.
 ******************************************************************************
 ******************************************************************************/
static void create_system_panel(lv_obj_t *parent) {
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, "System Management");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_24, 0);
    lv_obj_set_style_margin_bottom(lbl, 15, 0);

    // Reboot section
    lv_obj_t *btn_reboot = lv_button_create(parent);
    lv_obj_set_width(btn_reboot, lv_pct(100));
    lv_obj_t *lbl_reboot = lv_label_create(btn_reboot);
    lv_label_set_text(lbl_reboot, LV_SYMBOL_POWER " Reboot Device");
    lv_obj_set_style_text_font(lbl_reboot, &lv_font_montserrat_20, 0);
    lv_obj_add_event_cb(btn_reboot, reboot_event_cb, LV_EVENT_CLICKED, NULL);

    // Separator
    create_separator(parent, 20, 20);

    // Firmware Update section
    lv_obj_t *lbl_fw_hdr = lv_label_create(parent);
    lv_label_set_text(lbl_fw_hdr, "Firmware");
    lv_obj_set_style_text_font(lbl_fw_hdr, &lv_font_montserrat_20, 0);
    lv_obj_set_style_margin_bottom(lbl_fw_hdr, 8, 0);

    lbl_fw_version = lv_label_create(parent);
    lv_label_set_text(lbl_fw_version, "Current Version: v1.0.0");
    lv_obj_set_style_margin_bottom(lbl_fw_version, 8, 0);

    lv_obj_t *btn_check = lv_button_create(parent);
    lv_obj_set_width(btn_check, lv_pct(100));
    lv_obj_t *lbl_check = lv_label_create(btn_check);
    lv_label_set_text(lbl_check, LV_SYMBOL_REFRESH " Check for Updates");
    lv_obj_set_style_text_font(lbl_check, &lv_font_montserrat_20, 0);
    lv_obj_add_event_cb(btn_check, check_firmware_event_cb, LV_EVENT_CLICKED, NULL);

    lbl_fw_status = lv_label_create(parent);
    lv_label_set_text(lbl_fw_status, "");
    lv_obj_set_style_margin_top(lbl_fw_status, 8, 0);
    lv_obj_set_style_margin_bottom(lbl_fw_status, 8, 0);

    btn_fw_update = lv_button_create(parent);
    lv_obj_set_width(btn_fw_update, lv_pct(100));
    lv_obj_t *lbl_fw = lv_label_create(btn_fw_update);
    lv_label_set_text(lbl_fw, LV_SYMBOL_DOWNLOAD " Install Update");
    lv_obj_set_style_text_font(lbl_fw, &lv_font_montserrat_20, 0);
    lv_obj_add_event_cb(btn_fw_update, firmware_update_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(btn_fw_update, LV_OBJ_FLAG_HIDDEN);  // Hidden until update is found
}

/******************************************************************************
 ******************************************************************************
 ** @brief Monitor refresh timer.
 **
 ** @section call_site Called from:
 ** - lv_timer every 1000ms when Monitor panel is active.
 **
 ** @section dependencies Required Headers:
 ** - minigui.h (for stats bridge fetch)
 **
 ** @param timer (lv_timer_t*): The trigger timer.
 **
 ** @section pointers 
 ** - timer: Owned by LVGL.
 **
 ** @section variables Internal Variables:
 ** - @c stats (minigui_system_stats_t): Freshly fetched data.
 **
 ** @return void
 **
 ** Implementation Steps:
 ** 1. Call @c minigui_get_system_stats.
 ** 2. Format and update labels for Voltage, CPU, Flash, and RAM.
 ******************************************************************************
 ******************************************************************************/
static void monitor_timer_cb(lv_timer_t *timer) {
    if (!lbl_voltage || !lbl_cpu || !lbl_flash || !lbl_ram) return;

    minigui_system_stats_t stats;
    minigui_get_system_stats(&stats);

    char buf[64];
    snprintf(buf, sizeof(buf), "Voltage: %.2fV", stats.voltage);
    lv_label_set_text(lbl_voltage, buf);

    snprintf(buf, sizeof(buf), "CPU Usage: %d%%", stats.cpu_usage);
    lv_label_set_text(lbl_cpu, buf);

    snprintf(buf, sizeof(buf), "Flash: %lu / %lu KB (%d%%)",
             (unsigned long)stats.flash_used_kb,
             (unsigned long)stats.flash_total_kb,
             (int)((stats.flash_used_kb * 100) / stats.flash_total_kb));
    lv_label_set_text(lbl_flash, buf);

    snprintf(buf, sizeof(buf), "RAM: %lu / %lu KB (%d%%)",
             (unsigned long)stats.ram_used_kb,
             (unsigned long)stats.ram_total_kb,
             (int)((stats.ram_used_kb * 100) / stats.ram_total_kb));
    lv_label_set_text(lbl_ram, buf);
}

/******************************************************************************
 ******************************************************************************
 ** @brief Clean up monitor panel resources.
 **
 ** @section call_site Called from:
 ** - Content container LV_EVENT_DELETE.
 **
 ** @section dependencies Required Headers:
 ** - None
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
 ** 1. Delete @c monitor_timer to stop polling.
 ** 2. Nullify all UI pointers associated with the monitor panel.
 ******************************************************************************
 ******************************************************************************/
static void monitor_panel_delete_cb(lv_event_t * e) {
    if (monitor_timer) {
        lv_timer_del(monitor_timer);
        monitor_timer = NULL;
    }
    lbl_voltage = NULL;
    lbl_cpu = NULL;
    lbl_flash = NULL;
    lbl_ram = NULL;
}

/******************************************************************************
 ******************************************************************************
 ** @brief Create the "Monitor" settings panel.
 **
 ** @section call_site Called from:
 ** - switch_category() when Monitor category is selected.
 **
 ** @section dependencies Required Headers:
 ** - lvgl.h (widgets)
 **
 ** @param parent (lv_obj_t*): The content pane container.
 **
 ** @section pointers 
 ** - parent: Owned by screen_settings.
 **
 ** @section variables 
 ** - None
 **
 ** @return void
 **
 ** Implementation Steps:
 ** 1. Create a dedicated @c monitor_cont to leverage LV_EVENT_DELETE.
 ** 2. Populate container with statistics labels.
 ** 3. Start high-frequency refresh timer (1s).
 ******************************************************************************
 ******************************************************************************/
static void create_monitor_panel(lv_obj_t *parent) {
    // Create a container specifically for the monitor contents
    // This allows us to handle deletion of exactly these components
    lv_obj_t * monitor_cont = lv_obj_create(parent);
    lv_obj_set_size(monitor_cont, lv_pct(100), lv_pct(100));
    lv_obj_set_flex_flow(monitor_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(monitor_cont, 0, 0);
    lv_obj_set_style_pad_gap(monitor_cont, 10, 0);
    lv_obj_set_style_bg_opa(monitor_cont, 0, 0);
    lv_obj_set_style_border_width(monitor_cont, 0, 0);

    // Add delete event to the container
    lv_obj_add_event_cb(monitor_cont, monitor_panel_delete_cb, LV_EVENT_DELETE, NULL);

    lv_obj_t *lbl = lv_label_create(monitor_cont);
    lv_label_set_text(lbl, "System Monitor");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_24, 0);
    lv_obj_set_style_margin_bottom(lbl, 15, 0);

    lbl_voltage = lv_label_create(monitor_cont);
    lv_label_set_text(lbl_voltage, "Voltage: --");
    lv_obj_set_style_margin_bottom(lbl_voltage, 8, 0);

    create_separator(monitor_cont, 0, 8);

    lbl_cpu = lv_label_create(monitor_cont);
    lv_label_set_text(lbl_cpu, "CPU Usage: --");
    lv_obj_set_style_margin_bottom(lbl_cpu, 8, 0);

    create_separator(monitor_cont, 0, 8);

    lbl_flash = lv_label_create(monitor_cont);
    lv_label_set_text(lbl_flash, "Flash: --");
    lv_obj_set_style_margin_bottom(lbl_flash, 8, 0);

    create_separator(monitor_cont, 0, 8);

    lbl_ram = lv_label_create(monitor_cont);
    lv_label_set_text(lbl_ram, "RAM: --");

    // Create 1-second update timer
    if (!monitor_timer) {
        monitor_timer = lv_timer_create(monitor_timer_cb, 1000, NULL);
    }

    // Initial update
    monitor_timer_cb(NULL);
}

// ============================================================================
//  CATEGORY NAVIGATION
// ============================================================================

/******************************************************************************
 ******************************************************************************
 ** @brief Switches the active settings panel.
 **
 ** @section call_site Called from:
 ** - create_screen_settings() (default).
 ** - category_event_cb()
 **
 ** @section dependencies Required Headers:
 ** - None
 **
 ** @param cat (settings_category_t): The category to switch to.
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
 ** 1. Log the navigation action.
 ** 2. Call lv_obj_clean() on content_pane.
 ** 3. Call the appropriate panel factory function.
 ******************************************************************************
 ******************************************************************************/
static void switch_category(settings_category_t cat) {
    current_category = cat;

    // Log user navigation
    const char* cat_names[] = {"Screen", "Network", "System", "Monitor"};
    if (cat < SETTINGS_CAT_COUNT) {
        LV_LOG_USER("Settings: Switching to %s panel", cat_names[cat]);
    }

    // Clean content pane
    lv_obj_clean(content_pane);

    // Create new panel
    switch(cat) {
        case SETTINGS_CAT_SCREEN:
            create_screen_panel(content_pane);
            break;
        case SETTINGS_CAT_NETWORK:
            create_network_panel(content_pane);
            break;
        case SETTINGS_CAT_SYSTEM:
            create_system_panel(content_pane);
            break;
        case SETTINGS_CAT_MONITOR:
            create_monitor_panel(content_pane);
            break;
        default:
            break;
    }
}

/******************************************************************************
 ******************************************************************************
 ** @brief Handle navigation button clicks.
 **
 ** @section call_site Called from:
 ** - Category button LV_EVENT_CLICKED.
 **
 ** @section dependencies Required Headers:
 ** - None (Internal Router)
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
 ** 1. Extract category ID from the button's user data.
 ** 2. Delegate to @c switch_category.
 ******************************************************************************
 ******************************************************************************/
static void category_event_cb(lv_event_t * e) {
    lv_obj_t * btn = lv_event_get_target(e);
    settings_category_t cat = (settings_category_t)(uintptr_t)lv_event_get_user_data(e);
    switch_category(cat);
}

// ============================================================================
//  MAIN SCREEN CREATION
// ============================================================================

/******************************************************************************
 ******************************************************************************
 ** @brief Handle screen delete to clean up globals.
 **
 ** @section call_site Called from:
 ** - Main settings object LV_EVENT_DELETE.
 **
 ** @section dependencies Required Headers:
 ** - None
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
 ** 1. Zero out global pointers that reference destroyed objects.
 ******************************************************************************
 ******************************************************************************/
static void settings_screen_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_DELETE) {
        content_pane = NULL;
        kb = NULL;
    }
}

/******************************************************************************
 ******************************************************************************
 ** @brief Creates the Settings screen object with its multi-pane layout.
 **
 ** @section call_site Called from:
 ** - minigui_switch_screen() via mapping.
 **
 ** @section dependencies Required Headers:
 ** - lvgl.h (advanced layouts)
 **
 ** @param parent (lv_obj_t*): The content_area container.
 **
 ** @section pointers 
 ** - parent: Owned by minigui.c.
 **
 ** @section variables 
 ** - None
 **
 ** @return void
 **
 ** Implementation Steps:
 ** 1. Define split layout (Nav/Content).
 ** 2. Populate left pane with category routing buttons.
 ** 3. Initialize keyboard for user input.
 ** 4. Trigger default (Screen) category view.
 ******************************************************************************
 ******************************************************************************/
void create_screen_settings(lv_obj_t *parent) {
    lv_obj_add_event_cb(parent, settings_screen_event_cb, LV_EVENT_DELETE, NULL);
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    // Main horizontal container (two-pane layout)
    lv_obj_t *main_cont = lv_obj_create(parent);
    lv_obj_set_size(main_cont, lv_pct(100), lv_pct(100));
    lv_obj_set_flex_flow(main_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(main_cont, 0, 0);
    lv_obj_set_style_pad_gap(main_cont, 0, 0);
    lv_obj_set_style_border_width(main_cont, 0, 0);
    lv_obj_set_style_bg_opa(main_cont, 0, 0);

    // LEFT PANE: Navigation (fixed 200px)
    lv_obj_t *nav_pane = lv_obj_create(main_cont);
    lv_obj_set_size(nav_pane, 200, lv_pct(100));
    lv_obj_set_style_bg_color(nav_pane, lv_color_hex(0x2a2a2a), 0);
    lv_obj_set_style_border_width(nav_pane, 1, 0);
    lv_obj_set_style_border_side(nav_pane, LV_BORDER_SIDE_RIGHT, 0);
    lv_obj_set_style_border_color(nav_pane, lv_color_hex(0x444444), 0);
    lv_obj_set_flex_flow(nav_pane, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(nav_pane, 10, 0);
    lv_obj_set_style_pad_gap(nav_pane, 8, 0);

    const char *category_names[] = {
        LV_SYMBOL_IMAGE " Screen",
        LV_SYMBOL_WIFI " Network",
        LV_SYMBOL_SETTINGS " System",
        LV_SYMBOL_EYE_OPEN " Monitor"
    };

    for (int i = 0; i < SETTINGS_CAT_COUNT; i++) {
        lv_obj_t *btn = lv_button_create(nav_pane);
        lv_obj_set_width(btn, lv_pct(100));
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, category_names[i]);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_24, 0);
        lv_obj_add_event_cb(btn, category_event_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
    }

    // RIGHT PANE: Content (flexible)
    content_pane = lv_obj_create(main_cont);
    lv_obj_set_flex_grow(content_pane, 1);
    lv_obj_set_height(content_pane, lv_pct(100));
    lv_obj_set_flex_flow(content_pane, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(content_pane, 20, 0);
    lv_obj_set_style_pad_gap(content_pane, 10, 0);

    // Shared Keyboard (hidden by default)
    kb = lv_keyboard_create(parent);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(kb, kb_event_cb, LV_EVENT_ALL, NULL);

    // Load default category
    switch_category(SETTINGS_CAT_SCREEN);
}
