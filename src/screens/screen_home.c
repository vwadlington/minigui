#include "screens/screen_home.h"
#include "minigui.h" // For access if needed

static void create_info_card(lv_obj_t *parent, const char* title, const char* value, lv_color_t color) {
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, 220, 150);
    lv_obj_set_style_bg_color(card, color, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    
    lv_obj_t *lbl_title = lv_label_create(card);
    lv_label_set_text(lbl_title, title);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_LEFT, 0, 0);
    
    lv_obj_t *lbl_val = lv_label_create(card);
    lv_label_set_text(lbl_val, value);
    lv_obj_set_style_text_font(lbl_val, &lv_font_montserrat_48, 0);
    lv_obj_center(lbl_val);
}

void create_screen_home(lv_obj_t *parent) {
    // 1. Set styles on the content area specifically for Home
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    // 2. Create Layout Container for Cards
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_center(cont);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW_WRAP); // Wrap cards if they don't fit
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);

    // 3. Create Cards
    create_info_card(cont, "Indoor", "72°F", lv_palette_darken(LV_PALETTE_BLUE, 2));
    create_info_card(cont, "Outdoor", "85°F", lv_palette_darken(LV_PALETTE_ORANGE, 2));
    create_info_card(cont, "Status", "Good", lv_palette_darken(LV_PALETTE_GREEN, 2));
}