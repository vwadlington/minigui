#include "screens/screen_settings.h"
#include "minigui.h"

static void slider_event_cb(lv_event_t * e) {
    lv_obj_t * slider = lv_event_get_target(e);
    int brightness = (int)lv_slider_get_value(slider);
    minigui_set_brightness(brightness);
}

void create_screen_settings(lv_obj_t *parent) {
    // 1. Style Parent
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x334455), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    // 2. Container for settings controls
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_set_size(panel, 600, 300);
    lv_obj_center(panel);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);

    // 3. Brightness Control
    lv_obj_t *lbl_bright = lv_label_create(panel);
    lv_label_set_text(lbl_bright, "Screen Brightness");
    
    lv_obj_t *slider = lv_slider_create(panel);
    lv_obj_set_width(slider, LV_PCT(90));
    lv_slider_set_value(slider, 70, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    // Initial set
    minigui_set_brightness(70);

    // 4. Placeholder for WiFi
    lv_obj_t *lbl_wifi = lv_label_create(panel);
    lv_label_set_text(lbl_wifi, "WiFi Settings (Coming Soon)");
    lv_obj_set_style_margin_top(lbl_wifi, 20, 0);
}