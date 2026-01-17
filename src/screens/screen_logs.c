#include "screens/screen_logs.h"
#include "minigui.h"
#include "app_bridge.h"
#include "lvgl.h"
#include <stdlib.h>  // For malloc/free

static lv_obj_t *data_table = NULL;
static lv_obj_t *filter_dropdown = NULL;

static void update_table_with_logs(const char *filter) {
    if (!data_table) return;
    
    // Allocate 50 formatted logs on HEAP (6KB) - not stack!
    formatted_log_entry_t *logs = (formatted_log_entry_t*)malloc(
        APP_BRIDGE_MAX_LOGS * sizeof(formatted_log_entry_t));
    
    if (!logs) {
        LV_LOG_ERROR("Failed to allocate memory for logs");
        lv_table_set_row_cnt(data_table, 1);
        lv_table_set_cell_value(data_table, 0, 3, "Memory error");
        return;
    }
    
    // Get logs from bridge
    size_t count = app_bridge_get_formatted_logs(logs, APP_BRIDGE_MAX_LOGS, filter);
    
    // Clear the table first
    lv_table_set_row_cnt(data_table, 0);
    
    // Update table
    lv_table_set_row_cnt(data_table, count);
    
    // Fill table with data
    for (size_t i = 0; i < count; i++) {
        lv_table_set_cell_value(data_table, i, 0, logs[i].timestamp);
        lv_table_set_cell_value(data_table, i, 1, logs[i].source);
        lv_table_set_cell_value(data_table, i, 2, logs[i].level);
        lv_table_set_cell_value(data_table, i, 3, logs[i].message);
    }
    
    // Show message if no logs
    if (count == 0) {
        lv_table_set_row_cnt(data_table, 1);
        lv_table_set_cell_value(data_table, 0, 0, "No logs");
        lv_table_set_cell_value(data_table, 0, 1, "for");
        lv_table_set_cell_value(data_table, 0, 2, "filter");
        lv_table_set_cell_value(data_table, 0, 3, filter ? filter : "ALL");
    }
    
    // CRITICAL: Free heap memory
    free(logs);
}

static void filter_event_cb(lv_event_t * e) {
    lv_obj_t * dropdown = lv_event_get_target(e);
    char filter_buf[16];
    lv_dropdown_get_selected_str(dropdown, filter_buf, sizeof(filter_buf));
    
    update_table_with_logs(filter_buf);
}

static void deferred_load_cb(lv_timer_t * t) {
    update_table_with_logs("ALL");
    lv_timer_del(t);
}

// ... rest of create_screen_logs remains the same ...

void create_screen_logs(lv_obj_t *parent) {
    // IMPORTANT: No large stack allocations in screen creation
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(parent, 0, 0);
    lv_obj_set_style_radius(parent, 0, 0);
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    // HEADER CONTAINER (minimal objects)
    lv_obj_t *header_cont = lv_obj_create(parent);
    lv_obj_set_size(header_cont, lv_pct(100), 40);
    lv_obj_set_flex_flow(header_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(header_cont, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(header_cont, 0, 0);
    lv_obj_set_style_border_width(header_cont, 0, 0);
    lv_obj_set_style_pad_all(header_cont, 5, 0);

    // HEADER LABEL
    lv_obj_t *header_lbl = lv_label_create(header_cont);
    lv_label_set_text(header_lbl, "TIME | FROM | LVL | MESSAGE");
    lv_obj_set_style_text_font(header_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(header_lbl, lv_color_white(), 0);
    lv_obj_set_flex_grow(header_lbl, 1);

    // FILTER DROPDOWN
    filter_dropdown = lv_dropdown_create(header_cont);
    lv_dropdown_set_options(filter_dropdown, "ALL\nESP\nLVGL\nUSER");
    lv_obj_set_width(filter_dropdown, 100);
    lv_obj_set_height(filter_dropdown, 30);
    lv_obj_set_style_text_font(filter_dropdown, &lv_font_montserrat_14, 0);
    lv_obj_set_style_radius(filter_dropdown, 4, 0);
    lv_obj_set_style_bg_color(filter_dropdown, lv_color_hex(0x444444), 0);
    lv_obj_set_style_text_color(filter_dropdown, lv_color_white(), 0);
    lv_obj_add_event_cb(filter_dropdown, filter_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // DATA TABLE
    data_table = lv_table_create(parent);
    lv_obj_set_size(data_table, lv_pct(100), lv_pct(100) - 40);
    
    // Table styling
    lv_obj_set_style_bg_color(data_table, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(data_table, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(data_table, 1, 0);
    lv_obj_set_style_border_color(data_table, lv_color_hex(0x555555), 0);
    lv_obj_set_style_border_side(data_table, LV_BORDER_SIDE_FULL, 0);
    lv_obj_set_style_radius(data_table, 0, 0);
    lv_obj_set_style_pad_all(data_table, 5, 0);
    
    // Set column count and widths
    lv_table_set_col_cnt(data_table, 4);
    lv_table_set_col_width(data_table, 0, 100);
    lv_table_set_col_width(data_table, 1, 80);
    lv_table_set_col_width(data_table, 2, 60);
    lv_table_set_col_width(data_table, 3, 500);
    
    // Set text properties
    lv_obj_set_style_text_font(data_table, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(data_table, lv_color_white(), 0);
    
    // Initial loading message
    lv_table_set_row_cnt(data_table, 1);
    lv_table_set_cell_value(data_table, 0, 0, "Loading...");
    lv_table_set_cell_value(data_table, 0, 1, "");
    lv_table_set_cell_value(data_table, 0, 2, "");
    lv_table_set_cell_value(data_table, 0, 3, "Retrieving logs");

    // Create timer to load logs (delayed to ensure UI is ready)
    lv_timer_t * t = lv_timer_create(deferred_load_cb, 100, NULL);
    lv_timer_set_repeat_count(t, 1);
}