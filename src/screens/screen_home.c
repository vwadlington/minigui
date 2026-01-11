#include "screens/screen_home.h"

// Helper to create simple info cards
static void create_info_card(lv_obj_t *parent, const char* title, const char* value, lv_color_t color) {
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, 220, 150);
    lv_obj_set_style_bg_color(card, color, 0);
    
    lv_obj_t *lbl_title = lv_label_create(card);
    lv_label_set_text(lbl_title, title);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_LEFT, 0, 0);
    
    lv_obj_t *lbl_val = lv_label_create(card);
    lv_label_set_text(lbl_val, value);
    lv_obj_set_style_text_font(lbl_val, &lv_font_montserrat_48, 0);
    lv_obj_center(lbl_val);
}

lv_obj_t* create_screen_home(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    
    // Header
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "Smart Home Dashboard");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    // Grid or Flex container for cards
    lv_obj_t *cont = lv_obj_create(screen);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(80));
    lv_obj_align(cont, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_border_width(cont, 0, 0); // clean look

    // Create Cards
    create_info_card(cont, "Indoor", "72°F", lv_palette_lighten(LV_PALETTE_BLUE, 4));
    create_info_card(cont, "Outdoor", "85°F", lv_palette_lighten(LV_PALETTE_ORANGE, 4));
    create_info_card(cont, "Weather", "Sunny", lv_palette_lighten(LV_PALETTE_GREY, 4));

    return screen;
}