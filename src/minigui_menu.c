#include "minigui_menu.h"
#include "minigui.h" // Needs access to minigui_switch_screen

static lv_obj_t *menu_drawer = NULL;
static lv_obj_t *menu_blocker = NULL;
static bool is_menu_open = false;

// --- Callback Prototypes ---
static void toggle_menu(bool open);
static void hamburger_cb(lv_event_t *e);
static void blocker_cb(lv_event_t *e);
static void nav_btn_cb(lv_event_t *e);

void minigui_menu_init(void) {
    // 1. Hamburger Button (Always visible on top layer)
    lv_obj_t *btn = lv_btn_create(lv_layer_top());
    lv_obj_set_size(btn, 60, 60);
    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0); // Transparent background
    lv_obj_set_style_shadow_width(btn, 0, 0);       // No shadow
    
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_LIST); // 3-line icon
    lv_obj_set_style_text_font(label, &lv_font_montserrat_28, 0); // Make it large
    lv_obj_center(label);
    
    lv_obj_add_event_cb(btn, hamburger_cb, LV_EVENT_CLICKED, NULL);

    // 2. Blocker (The dark background behind the menu)
    menu_blocker = lv_obj_create(lv_layer_top());
    lv_obj_set_size(menu_blocker, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(menu_blocker, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(menu_blocker, LV_OPA_50, 0);
    lv_obj_add_flag(menu_blocker, LV_OBJ_FLAG_HIDDEN); // Hidden by default
    lv_obj_add_event_cb(menu_blocker, blocker_cb, LV_EVENT_CLICKED, NULL);

    // 3. Menu Drawer
    menu_drawer = lv_obj_create(lv_layer_top());
    lv_obj_set_size(menu_drawer, 280, LV_PCT(100));
    lv_obj_align(menu_drawer, LV_ALIGN_LEFT_MID, -280, 0); // Start off-screen
    lv_obj_set_flex_flow(menu_drawer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_top(menu_drawer, 80, 0); // Space for hamburger button
    
    // 4. Navigation Buttons
    const char *labels[] = {"Home Dashboard", "System Logs", "Settings"};
    screen_type_t screens[] = {SCREEN_HOME, SCREEN_LOGS, SCREEN_SETTINGS};

    for (int i = 0; i < 3; i++) {
        lv_obj_t *nav_btn = lv_btn_create(menu_drawer);
        lv_obj_set_width(nav_btn, LV_PCT(100));
        lv_obj_set_height(nav_btn, 60);
        
        lv_obj_t *lbl = lv_label_create(nav_btn);
        lv_label_set_text(lbl, labels[i]);
        lv_obj_center(lbl);

        // Store target screen in user_data
        lv_obj_add_event_cb(nav_btn, nav_btn_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)screens[i]);
    }
}

static void toggle_menu(bool open) {
    is_menu_open = open;
    
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, menu_drawer);
    lv_anim_set_time(&a, 300);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);

    if (open) {
        lv_obj_clear_flag(menu_blocker, LV_OBJ_FLAG_HIDDEN);
        lv_anim_set_values(&a, -280, 0);
    } else {
        lv_obj_add_flag(menu_blocker, LV_OBJ_FLAG_HIDDEN);
        lv_anim_set_values(&a, 0, -280);
    }
    lv_anim_start(&a);
}

static void hamburger_cb(lv_event_t *e) { toggle_menu(!is_menu_open); }
static void blocker_cb(lv_event_t *e)   { toggle_menu(false); }

static void nav_btn_cb(lv_event_t *e) {
    screen_type_t target = (screen_type_t)(uintptr_t)lv_event_get_user_data(e);
    minigui_switch_screen(target);
    toggle_menu(false); // Close menu after selection
}