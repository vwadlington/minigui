#include "minigui.h"
#include "minigui_menu.h"
#include "screens/screen_home.h"
#include "screens/screen_logs.h"
#include "screens/screen_settings.h"

static lv_obj_t *current_screen_obj = NULL;

// Array of screen creation functions mapped to the enum
static screen_create_fn_t screen_creators[SCREEN_COUNT] = {
    [SCREEN_HOME]     = create_screen_home,
    [SCREEN_LOGS]     = create_screen_logs,
    [SCREEN_SETTINGS] = create_screen_settings
};

void minigui_init(void) {
    // 1. Initialize the global overlay menu (exists on lv_layer_top)
    minigui_menu_init();

    // 2. Load the default screen (Home)
    minigui_switch_screen(SCREEN_HOME);
}

void minigui_switch_screen(screen_type_t screen_type) {
    if (screen_type >= SCREEN_COUNT) return;

    // 1. Clean up the old screen to save RAM (non-persistent)
    if (current_screen_obj != NULL) {
        lv_obj_del(current_screen_obj);
        current_screen_obj = NULL;
    }

    // 2. Create the new screen
    current_screen_obj = screen_creators[screen_type]();

    // 3. Load it immediately (since we deleted the previous one, no transition is needed, 
    //    but you can use lv_scr_load_anim if you keep the old one alive briefly)
    lv_scr_load(current_screen_obj);
}