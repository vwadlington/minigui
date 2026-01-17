#include "screens/screen_logs.h"
#include "minigui.h"
#include "app_bridge.h"
#include "lvgl.h"
#include <stdlib.h>  // For malloc/free
#include <string.h>  // For strcpy, strcmp, etc.

// ============================================================================
// STATIC VARIABLES
// ============================================================================

static lv_obj_t *data_table = NULL;
static lv_obj_t *filter_dropdown = NULL;
static lv_obj_t *log_screen_parent = NULL; // Store the parent container

// ============================================================================
// PRIVATE HELPER FUNCTIONS
// ============================================================================

static void clear_table_cells(void) {
    if (!data_table) return;
    
    // Clear all existing cells
    uint16_t row_count = lv_table_get_row_cnt(data_table);
    for (uint16_t row = 0; row < row_count; row++) {
        lv_table_set_cell_value(data_table, row, 0, "");
        lv_table_set_cell_value(data_table, row, 1, "");
        lv_table_set_cell_value(data_table, row, 2, "");
        lv_table_set_cell_value(data_table, row, 3, "");
    }
}

static void update_table_with_logs(const char *filter) {
    if (!data_table) return;
    
    LV_LOG_USER("Refreshing log table with filter: %s", filter ? filter : "ALL");
    
    // Clear the table first for better UX
    clear_table_cells();
    lv_table_set_row_cnt(data_table, 1);
    lv_table_set_cell_value(data_table, 0, 3, "Loading...");
    
    // Force LVGL to update immediately
    lv_refr_now(NULL);
    
    // Allocate formatted logs on HEAP
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
    
    LV_LOG_USER("Log table refreshed with %d entries", count);
}

static void refresh_button_cb(lv_event_t * e) {
    (void)e; // Unused parameter
    
    // Get current filter
    char filter_buf[16];
    if (filter_dropdown) {
        lv_dropdown_get_selected_str(filter_dropdown, filter_buf, sizeof(filter_buf));
    } else {
        strcpy(filter_buf, "ALL");
    }
    
    // Refresh the table immediately
    update_table_with_logs(filter_buf);
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

// ============================================================================
// PUBLIC API
// ============================================================================

void refresh_log_table(const char *filter) {
    update_table_with_logs(filter ? filter : "ALL");
}

// ============================================================================
// CORRECTED FIXED LAYOUT (No percentages for LVGL table columns)
// ============================================================================

/**
 * @brief Calculate optimal column widths for 800px display
 */
static void calculate_table_layout(void) {
    if (!data_table || !log_screen_parent) return;
    
    // Get the actual width available for the table
    int32_t parent_width = lv_obj_get_width(log_screen_parent);
    if (parent_width <= 0) {
        parent_width = 800; // Default for ESP32-S3-LCD-EV-Board2
    }
    
    // Subtract padding (5px left + 5px right = 10px)
    int32_t available_width = parent_width - 10;
    
    // Fixed column widths that fit 800px screen perfectly:
    // 800px - 10px padding = 790px available
    // Column breakdown:
    // - Time: 100px (12.7%)
    // - Source: 80px (10.1%)
    // - Level: 60px (7.6%)
    // - Message: 550px (69.6%)
    // Total: 790px
    
    // Set column widths
    lv_table_set_col_width(data_table, 0, 100);   // Time
    lv_table_set_col_width(data_table, 1, 80);    // Source
    lv_table_set_col_width(data_table, 2, 60);    // Level
    lv_table_set_col_width(data_table, 3, available_width - 240); // Message (remaining)
}

/**
 * @brief Recalculate layout when parent size changes
 */
static void parent_size_changed_cb(lv_event_t * e) {
    (void)e; // Unused parameter
    calculate_table_layout();
}

// ============================================================================
// SIMPLIFIED SCREEN CREATION WITH PROPER SIZING
// ============================================================================

void create_screen_logs(lv_obj_t *parent) {
    log_screen_parent = parent; // Store for later calculations
    
    // SIMPLIFY: Use simple vertical layout without flex complications
    lv_obj_set_style_pad_all(parent, 0, 0);
    lv_obj_set_style_radius(parent, 0, 0);
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_OFF); // No scroll on parent

    // ========== HEADER CONTAINER (Fixed height at top) ==========
    lv_obj_t *header_cont = lv_obj_create(parent);
    lv_obj_set_size(header_cont, lv_pct(100), 40);  // Full width, 40px height
    lv_obj_set_style_bg_color(header_cont, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(header_cont, 0, 0);
    lv_obj_set_style_radius(header_cont, 0, 0);
    lv_obj_set_style_pad_all(header_cont, 5, 0);
    lv_obj_set_style_pad_gap(header_cont, 0, 0);
    lv_obj_set_scrollbar_mode(header_cont, LV_SCROLLBAR_MODE_OFF);

    // HEADER LABEL (left side)
    lv_obj_t *header_lbl = lv_label_create(header_cont);
    lv_label_set_text(header_lbl, "TIME | FROM | LVL | MESSAGE");
    lv_obj_set_style_text_font(header_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(header_lbl, lv_color_white(), 0);
    lv_obj_set_pos(header_lbl, 5, 5); // Fixed position
    lv_obj_set_size(header_lbl, 400, 30);

    // FILTER DROPDOWN (right side)
    filter_dropdown = lv_dropdown_create(header_cont);
    lv_dropdown_set_options(filter_dropdown, "ALL\nESP\nLVGL\nUSER");
    lv_obj_set_size(filter_dropdown, 100, 30);
    lv_obj_set_pos(filter_dropdown, 600, 5); // Right-aligned
    lv_obj_set_style_text_font(filter_dropdown, &lv_font_montserrat_14, 0);
    lv_obj_set_style_radius(filter_dropdown, 4, 0);
    lv_obj_set_style_bg_color(filter_dropdown, lv_color_hex(0x444444), 0);
    lv_obj_set_style_text_color(filter_dropdown, lv_color_white(), 0);
    lv_obj_add_event_cb(filter_dropdown, filter_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // REFRESH BUTTON (next to filter)
    lv_obj_t *refresh_btn = lv_button_create(header_cont);
    lv_obj_set_size(refresh_btn, 30, 30);
    lv_obj_set_pos(refresh_btn, 705, 5); // Right of filter
    lv_obj_set_style_radius(refresh_btn, 4, 0);
    lv_obj_set_style_bg_color(refresh_btn, lv_color_hex(0x444444), 0);
    lv_obj_set_style_bg_color(refresh_btn, lv_color_hex(0x555555), LV_STATE_PRESSED);
    
    lv_obj_t *refresh_label = lv_label_create(refresh_btn);
    lv_label_set_text(refresh_label, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_color(refresh_label, lv_color_white(), 0);
    lv_obj_center(refresh_label);
    
    lv_obj_add_event_cb(refresh_btn, refresh_button_cb, LV_EVENT_CLICKED, NULL);

    // ========== DATA TABLE (Fixed size below header) ==========
    data_table = lv_table_create(parent);
    
    // Calculate position and size: below header, full remaining height
    int32_t parent_height = lv_obj_get_height(parent);
    int32_t table_height = parent_height - 40; // Header is 40px
    
    lv_obj_set_pos(data_table, 0, 40); // Below header
    lv_obj_set_size(data_table, lv_pct(100), table_height);
    
    // Table styling
    lv_obj_set_style_bg_color(data_table, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(data_table, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(data_table, 0, 0);
    lv_obj_set_style_radius(data_table, 0, 0);
    lv_obj_set_style_pad_all(data_table, 5, 0);
    lv_obj_set_scrollbar_mode(data_table, LV_SCROLLBAR_MODE_AUTO);
    
    // Set column count
    lv_table_set_col_cnt(data_table, 4);
    
    // Calculate and set optimal column widths
    calculate_table_layout();
    
    // Set text properties
    lv_obj_set_style_text_font(data_table, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(data_table, lv_color_white(), 0);
    
    // Cell styling
    lv_obj_set_style_pad_all(data_table, 4, LV_PART_ITEMS);
    lv_obj_set_style_border_width(data_table, 1, LV_PART_ITEMS);
    lv_obj_set_style_border_color(data_table, lv_color_hex(0x444444), LV_PART_ITEMS);
    
    // Initial loading message
    lv_table_set_row_cnt(data_table, 1);
    lv_table_set_cell_value(data_table, 0, 0, "Loading...");
    lv_table_set_cell_value(data_table, 0, 1, "");
    lv_table_set_cell_value(data_table, 0, 2, "");
    lv_table_set_cell_value(data_table, 0, 3, "Retrieving logs");

    // Listen for parent size changes (if screen rotates or resizes)
    lv_obj_add_event_cb(parent, parent_size_changed_cb, LV_EVENT_SIZE_CHANGED, NULL);

    // Create timer to load logs (delayed to ensure UI is ready)
    lv_timer_t * t = lv_timer_create(deferred_load_cb, 100, NULL);
    lv_timer_set_repeat_count(t, 1);
}