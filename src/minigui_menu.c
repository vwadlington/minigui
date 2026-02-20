
/******************************************************************************
 ******************************************************************************
 * 1. ESP-IDF / FreeRTOS Core (Framework)
 ******************************************************************************
 ******************************************************************************/
// None

/******************************************************************************
 ******************************************************************************
 * 2. Managed Espressif Components (External Managed Components)
 ******************************************************************************
 ******************************************************************************/
// None, lvgl included via minigui_menu.h

/******************************************************************************
 ******************************************************************************
 * 3. Project-Specific Components (Local)
 ******************************************************************************
 ******************************************************************************/
#include "minigui_menu.h"
#include "minigui.h"

/******************************************************************************
 ******************************************************************************
 * FILE GLOBALS
 ******************************************************************************
 ******************************************************************************/

/******************************************************************************
 * @brief Reference to the sliding drawer object
 * @section scope
 * Internal to minigui_menu.c
 * @section rationale
 * Holds the navigation buttons and slides in from the left.
 ******************************************************************************/
static lv_obj_t *menu_drawer = NULL;

/******************************************************************************
 * @brief Reference to the full-screen background blocker
 * @section scope
 * Internal to minigui_menu.c
 * @section rationale
 * Dims the main content when menu is open and captures clicks to close the menu.
 ******************************************************************************/
static lv_obj_t *menu_blocker = NULL;

// ============================================================================
// PRIVATE HELPER FUNCTIONS
// ============================================================================

/******************************************************************************
 * @brief Callback when the background "dimmer" is clicked.
 *
 * @section call_site
 * Assigned to `menu_blocker`'s `LV_EVENT_CLICKED`.
 *
 * @section dependencies
 * - None
 *
 * @param e LVGL event object.
 *
 * @return void
 *
 * Implementation Steps
 * 1. Log the closure action.
 * 2. Call `minigui_menu_toggle()` to close the menu.
 ******************************************************************************/
static void blocker_cb(lv_event_t *e) {
    LV_LOG_USER("Menu closed via background dimmer");
    minigui_menu_toggle();
}

/******************************************************************************
 * @brief Callback for navigation buttons inside the drawer.
 *
 * @section call_site
 * Assigned to each navigation button in the drawer (`LV_EVENT_CLICKED`).
 *
 * @section dependencies
 * - `minigui.h`: To switch screens.
 *
 * @param e LVGL event object.
 *
 * @section keys
 * - `user_data`: Cast to `minigui_screen_t` to set the target screen.
 *
 * @return void
 *
 * Implementation Steps
 * 1. Retrieve the target screen ID from the event's user data.
 * 2. Log the navigation action.
 * 3. Call `minigui_switch_screen` to change the main view.
 * 4. Call `minigui_menu_toggle()` to close the drawer.
 ******************************************************************************/
static void nav_btn_cb(lv_event_t *e) {
    minigui_screen_t target = (minigui_screen_t)(uintptr_t)lv_event_get_user_data(e);

    // Use LV_LOG_USER for user actions - perfect separation!
    const char* screen_names[] = {"Home", "Logs", "Settings"};
    LV_LOG_USER("User navigating to %s screen", screen_names[target]);

    // Switch the content area screen
    minigui_switch_screen(target);

    // Close the drawer
    minigui_menu_toggle();
}

// ============================================================================
// PUBLIC API FUNCTIONS
// ============================================================================

/******************************************************************************
 * @brief Initializes the global navigation menu.
 *
 * @section call_site
 * Called from `minigui_init()`.
 *
 * @return void
 *
 * Implementation Steps
 * 1. Get the top layer of the screen to ensure menu floats above everything.
 * 2. Create the `menu_blocker`: full-screen, semi-transparent black, hidden by default.
 * 3. Attach `blocker_cb` to the blocker to handle outside clicks.
 * 4. Create the `menu_drawer`: 250px wide, 100% height, positioned off-screen (-250px).
 * 5. Define the list of navigation buttons and their corresponding screen IDs.
 * 6. Loop through the list to create buttons inside the drawer.
 * 7. For each button, create a label and attach `nav_btn_cb` with the screen ID as user data.
 ******************************************************************************/
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
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_24, 0);
        lv_obj_center(lbl);

        // Pass the screen ID as user data
        lv_obj_add_event_cb(btn, nav_btn_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)screen_ids[i]);
    }
}

/******************************************************************************
 * @brief Toggles the visibility of the navigation menu.
 *
 * @return void
 *
 * Implementation Steps
 * 1. Check if drawer or blocker are invalid; return if so.
 * 2. Check the current state (hidden/visible) via the blocker's hidden flag.
 * 3. Initialize an animation object for the drawer's X position.
 * 4. If hidden (opening):
 *    - Remove hidden flag from blocker.
 *    - Animate drawer from -250 to 0.
 * 5. If visible (closing):
 *    - Add hidden flag to blocker.
 *    - Animate drawer from 0 to -250.
 * 6. Start the animation.
 ******************************************************************************/
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
