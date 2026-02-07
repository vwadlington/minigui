#include "minigui.h"
#include "minigui_menu.h"
#include "screens/screen_home.h"
#include "screens/screen_logs.h"
#include "screens/screen_settings.h"
#include <time.h>
#include <stdio.h>

// UI Structure Globals
static lv_obj_t *main_container = NULL;
static lv_obj_t *status_bar = NULL;
static lv_obj_t *content_area = NULL;
static lv_obj_t *lbl_title = NULL;
static lv_obj_t *lbl_clock = NULL;

// Callback hooks
static minigui_brightness_cb_t brightness_cb = NULL;
static minigui_time_provider_t global_time_provider = NULL;

// Screen Creator Array
static const ui_screen_creator_t screen_creators[MINIGUI_SCREEN_COUNT] = {
    [MINIGUI_SCREEN_HOME]     = create_screen_home,
    [MINIGUI_SCREEN_LOGS]     = create_screen_logs,
    [MINIGUI_SCREEN_SETTINGS] = create_screen_settings
};

// ============================================================================
// PRIVATE HELPER FUNCTIONS
// ============================================================================

/**
 * Updates the clock label with current system time.
 */
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

// Event wrapper to open the menu
static void menu_btn_event_cb(lv_event_t * e) {
    LV_LOG_USER("Hamburger menu toggled");
    minigui_menu_toggle();
}

/**
 * Ensures the hamburger button is a perfect square.
 */
static void sync_square_size_cb(lv_event_t * e) {
    lv_obj_t * btn = lv_event_get_target(e);
    lv_obj_t * parent = lv_obj_get_parent(btn);
    int32_t h = lv_obj_get_height(parent);
    if (h > 0) {
        lv_obj_set_width(btn, h);
    }
}

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
void minigui_register_brightness_cb(minigui_brightness_cb_t cb) { brightness_cb = cb; }

void minigui_set_time_provider(minigui_time_provider_t provider) {
    global_time_provider = provider;
    // Update immediately if possible
    if (lbl_clock) update_clock_cb(NULL);
}

void minigui_set_brightness(uint8_t brightness) { if (brightness_cb) brightness_cb(brightness); }
lv_obj_t *minigui_get_content_area(void) { return content_area; }
