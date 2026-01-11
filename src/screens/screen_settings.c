#include "screens/screen_settings.h"

static void slider_event_cb(lv_event_t * e) {
    lv_obj_t * slider = lv_event_get_target(e);
    int brightness = (int)lv_slider_get_value(slider);
    // TODO: Call bsp_display_brightness_set(brightness);
}

lv_obj_t* create_screen_settings(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    lv_obj_t *panel = lv_obj_create(screen);
    lv_obj_set_size(panel, 600, 400);
    lv_obj_center(panel);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);

    // 1. Brightness Slider
    lv_obj_t *lbl_bright = lv_label_create(panel);
    lv_label_set_text(lbl_bright, "Screen Brightness");
    
    lv_obj_t *slider = lv_slider_create(panel);
    lv_obj_set_width(slider, LV_PCT(90));
    lv_slider_set_value(slider, 70, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // Spacer
    lv_obj_t * spacer = lv_obj_create(panel);
    lv_obj_set_height(spacer, 20);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer, 0, 0);

    // 2. WiFi Toggle
    lv_obj_t *cont_wifi = lv_obj_create(panel);
    lv_obj_set_size(cont_wifi, LV_PCT(100), 60);
    lv_obj_set_style_border_width(cont_wifi, 0, 0);
    
    lv_obj_t *lbl_wifi = lv_label_create(cont_wifi);
    lv_label_set_text(lbl_wifi, "Enable WiFi");
    lv_obj_align(lbl_wifi, LV_ALIGN_LEFT_MID, 0, 0);
    
    lv_obj_t *sw = lv_switch_create(cont_wifi);
    lv_obj_add_state(sw, LV_STATE_CHECKED); // Default on
    lv_obj_align(sw, LV_ALIGN_RIGHT_MID, 0, 0);

    return screen;
}