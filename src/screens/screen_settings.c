
/******************************************************************************
 ******************************************************************************
 * 1. ESP-IDF / FreeRTOS Core (Framework)
 ******************************************************************************
 ******************************************************************************/
#include <string.h>
#include <stdio.h>

/******************************************************************************
 ******************************************************************************
 * 2. Managed Espressif Components (External Managed Components)
 ******************************************************************************
 ******************************************************************************/
// None (lvgl included via header)

/******************************************************************************
 ******************************************************************************
 * 3. Project-Specific Components (Local)
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
 * @brief Dynamic content container for the right side of the split view
 * @section scope
 * Internal to screen_settings.c
 * @section rationale
 * Cleared and repopulated when switching categories.
 ******************************************************************************/
static lv_obj_t *content_pane = NULL;

/******************************************************************************
 * @brief Shared on-screen keyboard
 * @section scope
 * Internal to screen_settings.c
 * @section rationale
 * Reused across any input fields in the settings screen.
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
 * @brief Handle keyboard events (close on OK/Cancel).
 *
 * @param e LVGL event
 *
 * Implementation Steps
 * 1. Hide keyboard if READY or CANCEL is pressed.
 * 2. Unfocus the text area.
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
 * @brief Handle text area focus to show keyboard.
 *
 * @param e LVGL event
 *
 * Implementation Steps
 * 1. If focused, attach keyboard to this text area.
 * 2. Show the keyboard.
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
 * @brief Handle "Scan" button click.
 *
 * @param e LVGL event
 *
 * Implementation Steps
 * 1. Udpate UI to show scanning state.
 * 2. Call `minigui_scan_wifi` to get network list.
 * 3. Populate dropdown with results or message if none found.
 * 4. Restore UI state.
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
 * @brief Handle "Save WiFi" button click.
 *
 * @param e LVGL event
 *
 * Implementation Steps
 * 1. Get selected SSID and entered password.
 * 2. Validate input.
 * 3. Call `minigui_save_wifi_credentials`.
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
 * @brief Helper to create a horizontal separator line.
 *
 * @param parent Parent object.
 * @param margin_top Top margin in pixels.
 * @param margin_bottom Bottom margin in pixels.
 *
 * Implementation Steps
 * 1. Create 1px tall object.
 * 2. Set color to grey.
 * 3. Apply margins.
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
 * @brief Handle brilliance slider change.
 *
 * @param e LVGL event
 *
 * Implementation Steps
 * 1. Get value.
 * 2. Call `minigui_set_brightness`.
 ******************************************************************************/
static void slider_event_cb(lv_event_t * e) {
    lv_obj_t * slider = lv_event_get_target(e);
    int brightness = (int)lv_slider_get_value(slider);
    LV_LOG_USER("Brightness changed to %d%%", brightness);
    minigui_set_brightness(brightness);
}

/******************************************************************************
 * @brief Create the "Screen" settings panel.
 *
 * @param parent Content pane.
 *
 * Implementation Steps
 * 1. Add title.
 * 2. Add slider for brightness.
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
 * @brief Handle "Check for Updates" button click.
 *
 * @param e LVGL event
 *
 * Implementation Steps
 * 1. Simulate check.
 * 2. Toggle mock update status.
 * 3. Show/hide update button.
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
 * @brief Create the "System" settings panel.
 *
 * @param parent Content pane.
 *
 * Implementation Steps
 * 1. Add Reboot button.
 * 2. Add Firmware section (version, check button, update button).
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
 * @brief Monitor refresh timer.
 *
 * @param timer LVGL timer
 *
 * Implementation Steps
 * 1. Get system stats.
 * 2. Update labels (Voltage, CPU, Flash, RAM).
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
 * @brief Clean up monitor panel resources.
 *
 * @param e LVGL event
 *
 * Implementation Steps
 * 1. Delete timer.
 * 2. Nullify pointers.
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
 * @brief Create the "Monitor" settings panel.
 *
 * @param parent Content pane.
 *
 * Implementation Steps
 * 1. Create a container for monitor elements (to handle cleanup).
 * 2. Add Voltage, CPU, Flash, RAM labels with separators.
 * 3. Start 1s timer to refresh stats.
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
 * @brief Switches the active settings panel.
 *
 * @param cat The category to switch to.
 *
 * Implementation Steps
 * 1. Clean current content pane.
 * 2. Log navigation.
 * 3. Invoke specific panel creator based on category.
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
 * @brief Handle navigation button clicks.
 *
 * @param e LVGL event
 *
 * Implementation Steps
 * 1. Get category from user data.
 * 2. Call `switch_category`.
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
 * @brief Handle screen delete to clean up globals.
 *
 * @param e LVGL event
 *
 * Implementation Steps
 * 1. Set global pointers (content_pane, kb) to NULL.
 ******************************************************************************/
static void settings_screen_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_DELETE) {
        content_pane = NULL;
        kb = NULL;
    }
}

/******************************************************************************
 * @brief Creates the Settings screen object with its multi-pane layout.
 *
 * @param parent The `content_area` object.
 *
 * Implementation Steps
 * 1. Setup screen styles.
 * 2. Create Main Horizontal Container (Split View).
 * 3. Create Left Nav Pane (Fixed 200px).
 * 4. Create Nav Buttons (Screen, Network, System, Monitor).
 * 5. Create Right Content Pane (Flexible).
 * 6. Create Hidden Keyboard.
 * 7. Default to Screen category.
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
