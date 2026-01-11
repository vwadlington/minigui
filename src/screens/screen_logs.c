#include "screens/screen_logs.h"

static void filter_event_cb(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target(e);
    // TODO: Add logic here to reload the table data based on the filter
    LV_LOG_USER("Filter clicked: %s", lv_label_get_text(lv_obj_get_child(btn, 0)));
}

lv_obj_t* create_screen_logs(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    // 1. Filter Area (Top)
    lv_obj_t *filter_cont = lv_obj_create(screen);
    lv_obj_set_size(filter_cont, LV_PCT(100), 60);
    lv_obj_align(filter_cont, LV_ALIGN_TOP_MID, 0, 60); // Below hamburger
    lv_obj_set_flex_flow(filter_cont, LV_FLEX_FLOW_ROW);
    
    const char *filters[] = {"ALL", "ESP_LOG", "LVGL", "USER"};
    for(int i=0; i<4; i++) {
        lv_obj_t *btn = lv_btn_create(filter_cont);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, filters[i]);
        lv_obj_center(lbl);
        lv_obj_add_event_cb(btn, filter_event_cb, LV_EVENT_CLICKED, NULL);
    }

    // 2. Log Table
    lv_obj_t *table = lv_table_create(screen);
    lv_obj_set_size(table, LV_PCT(95), LV_PCT(70));
    lv_obj_align(table, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    // Set headers
    lv_table_set_col_cnt(table, 2);
    lv_table_set_col_width(table, 0, 150); // Timestamp/Tag
    lv_table_set_col_width(table, 1, 600); // Message
    
    lv_table_set_cell_value(table, 0, 0, "TAG");
    lv_table_set_cell_value(table, 0, 1, "MESSAGE");

    // Dummy Data (In reality, you would read this from your file)
    lv_table_set_cell_value(table, 1, 0, "[WIFI]");
    lv_table_set_cell_value(table, 1, 1, "Connected to SSID 'HomeNet'");
    
    lv_table_set_cell_value(table, 2, 0, "[SENSOR]");
    lv_table_set_cell_value(table, 2, 1, "Temperature update: 72.5F");

    return screen;
}