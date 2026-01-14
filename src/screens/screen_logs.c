#include "screens/screen_logs.h"
#include "minigui.h"
#include <stdio.h>

static lv_obj_t *data_table = NULL;

// This function does the heavy lifting
void refresh_log_table(const char *filter) {
    if (!data_table || !lv_obj_is_valid(data_table)) return;

    // Clear existing data
    lv_table_set_row_cnt(data_table, 0); 
    
    // Fetch new data via bridge
    minigui_log_provider_t provider = minigui_get_log_provider();
    if (provider) {
        provider(filter, data_table, 100); 
    }
}

// Timer callback to load data AFTER the UI has rendered
static void deferred_load_cb(lv_timer_t * t) {
    refresh_log_table("ALL");
    // We don't need the timer anymore
    // Note: If you use lv_timer_set_repeat_count(t, 1), it auto-deletes.
}

static void filter_event_cb(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *label = lv_obj_get_child(btn, 0);
    refresh_log_table(lv_label_get_text(label));
}

void create_screen_logs(lv_obj_t *parent) {
    // 1. Configure Parent (Content Area) for Vertical Stacking
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(parent, 0, 0);
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    // 2. Filter Button Row
    lv_obj_t *filter_cont = lv_obj_create(parent);
    lv_obj_set_size(filter_cont, LV_PCT(100), 50);
    lv_obj_set_flex_flow(filter_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_bg_opa(filter_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(filter_cont, 0, 0);
    
    const char *filters[] = {"ALL", "ESP_LOG", "LVGL", "SYS"};
    for(int i=0; i<4; i++) {
        lv_obj_t *btn = lv_button_create(filter_cont);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, filters[i]);
        lv_obj_center(lbl);
        lv_obj_add_event_cb(btn, filter_event_cb, LV_EVENT_CLICKED, NULL);
    }

    // 3. Header Table (Fixed)
    lv_obj_t *header_table = lv_table_create(parent);
    lv_table_set_col_cnt(header_table, 3);
    lv_table_set_row_cnt(header_table, 1);
    lv_table_set_col_width(header_table, 0, 70);
    lv_table_set_col_width(header_table, 1, 80);
    lv_table_set_col_width(header_table, 2, 600);
    lv_table_set_cell_value(header_table, 0, 0, "MS");
    lv_table_set_cell_value(header_table, 0, 1, "TAG");
    lv_table_set_cell_value(header_table, 0, 2, "MESSAGE");
    lv_obj_set_style_bg_color(header_table, lv_palette_main(LV_PALETTE_GREY), LV_PART_ITEMS);

    // 4. Data Table (Scrollable)
    data_table = lv_table_create(parent);
    lv_obj_set_width(data_table, lv_pct(100));
    lv_obj_set_flex_grow(data_table, 1); // Fill rest of screen
    lv_table_set_col_cnt(data_table, 3);
    lv_table_set_col_width(data_table, 0, 70);
    lv_table_set_col_width(data_table, 1, 80);
    lv_table_set_col_width(data_table, 2, 600);
    
    // 5. DEFERRED LOAD: Fixes the lag
    // Wait 50ms for screen to draw, then load the heavy file
    lv_timer_t * t = lv_timer_create(deferred_load_cb, 50, NULL);
    lv_timer_set_repeat_count(t, 1);
}