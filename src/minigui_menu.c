#include "minigui_menu.h"
#include "minigui.h"

static lv_obj_t *menu_drawer = NULL;
static lv_obj_t *menu_blocker = NULL;

/**
 * Callback when the background "dimmer" is clicked.
 * Closes the menu.
 */
static void blocker_cb(lv_event_t *e) {
    minigui_menu_toggle();
}

/**
 * Callback for navigation buttons inside the drawer.
 */
static void nav_btn_cb(lv_event_t *e) {
    minigui_screen_t target = (minigui_screen_t)(uintptr_t)lv_event_get_user_data(e);
    
    LV_LOG_INFO("Menu: Navigating to screen %d", target);
    
    // Switch the content area screen
    minigui_switch_screen(target);
    
    // Close the drawer
    minigui_menu_toggle(); 
}

void minigui_menu_init(void) {
    // We use the top layer so the menu slides OVER the status bar
    lv_obj_t *top = lv_layer_top();

    // 1. BLOCKER (Background Dimming)
    menu_blocker = lv_obj_create(top);
    lv_obj_set_size(menu_blocker, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(menu_blocker, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(menu_blocker, LV_OPA_50, 0);
    lv_obj_set_style_radius(menu_blocker, 0, 0); // Square
    lv_obj_set_style_border_width(menu_blocker, 0, 0);
    lv_obj_add_flag(menu_blocker, LV_OBJ_FLAG_HIDDEN); // Hidden by default
    lv_obj_add_event_cb(menu_blocker, blocker_cb, LV_EVENT_CLICKED, NULL);

    // 2. DRAWER (The sliding panel)
    menu_drawer = lv_obj_create(top);
    lv_obj_set_size(menu_drawer, 250, lv_pct(100));
    lv_obj_set_x(menu_drawer, -250); // Start off-screen to the left
    lv_obj_set_style_bg_color(menu_drawer, lv_color_hex(0x222222), 0);
    lv_obj_set_style_border_width(menu_drawer, 0, 0);
    lv_obj_set_style_radius(menu_drawer, 0, 0); // Square corners
    lv_obj_set_scrollbar_mode(menu_drawer, LV_SCROLLBAR_MODE_OFF);

    // 3. NAVIGATION BUTTONS
    const char *btn_texts[] = {"Home", "Logs", "Settings"};
    minigui_screen_t screen_ids[] = {MINIGUI_SCREEN_HOME, MINIGUI_SCREEN_LOGS, MINIGUI_SCREEN_SETTINGS};

    for (int i = 0; i < 3; i++) {
        lv_obj_t *btn = lv_button_create(menu_drawer);
        lv_obj_set_size(btn, lv_pct(90), 50);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 40 + (i * 60));
        lv_obj_set_style_radius(btn, 4, 0); // Slight rounding on buttons only for aesthetics
        
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, btn_texts[i]);
        lv_obj_center(lbl);
        
        // Pass the screen ID as user data
        lv_obj_add_event_cb(btn, nav_btn_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)screen_ids[i]);
    }
}

void minigui_menu_toggle(void) {
    if (!menu_drawer || !menu_blocker) return;

    bool is_hidden = lv_obj_has_flag(menu_blocker, LV_OBJ_FLAG_HIDDEN);

    // Create the animation for the drawer
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, menu_drawer);
    lv_anim_set_time(&a, 300);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);

    if (is_hidden) {
        // OPENING
        lv_obj_remove_flag(menu_blocker, LV_OBJ_FLAG_HIDDEN);
        lv_anim_set_values(&a, -250, 0);
    } else {
        // CLOSING
        lv_obj_add_flag(menu_blocker, LV_OBJ_FLAG_HIDDEN);
        lv_anim_set_values(&a, 0, -250);
    }
    
    lv_anim_start(&a);
}