
/******************************************************************************
 ******************************************************************************
 * 1. ESP-IDF / FreeRTOS Core (Framework)
 ******************************************************************************
 ******************************************************************************/
#include <stdlib.h>  // For malloc/free
#include <string.h>  // For strcpy, strcmp, etc.
#include <time.h>    // For mock timestamps

/******************************************************************************
 ******************************************************************************
 * 2. Managed Espressif Components (External Managed Components)
 ******************************************************************************
 ******************************************************************************/
#include "lvgl.h"

/******************************************************************************
 ******************************************************************************
 * 3. Project-Specific Components (Local)
 ******************************************************************************
 ******************************************************************************/
#include "screens/screen_logs.h"
#include "minigui.h"

/******************************************************************************
 ******************************************************************************
 * FILE GLOBALS
 ******************************************************************************
 ******************************************************************************/

/******************************************************************************
 * @brief Reference to the main data table
 * @section scope
 * Internal to screen_logs.c
 * @section rationale
 * Displays the list of log entries. Need handle to update data.
 ******************************************************************************/
static lv_obj_t *data_table = NULL;

/******************************************************************************
 * @brief Reference to the source filter dropdown
 * @section scope
 * Internal to screen_logs.c
 * @section rationale
 * Allows user to filter logs by source (e.g., wifi, system).
 ******************************************************************************/
static lv_obj_t *filter_dropdown = NULL;

/******************************************************************************
 * @brief Reference to the parent screen object
 * @section scope
 * Internal to screen_logs.c
 * @section rationale
 * Used to calculate available width for the table.
 ******************************************************************************/
static lv_obj_t *log_screen_parent = NULL;

/******************************************************************************
 * @brief Registered callback to fetch logs
 * @section scope
 * Internal to screen_logs.c
 * @section rationale
 * Decouples UI from the backend logging system.
 ******************************************************************************/
static minigui_log_provider_t global_log_provider = NULL;

// ============================================================================
// PUBLIC SETTERS
// ============================================================================

/******************************************************************************
 * @brief Register a log provider.
 *
 * @param provider The function pointer to retrieve logs.
 *
 * Implementation Steps
 * 1. Store the provider in `global_log_provider`.
 ******************************************************************************/
void minigui_set_log_provider(minigui_log_provider_t provider) {
    global_log_provider = provider;
}

// ============================================================================
// PRIVATE HELPER FUNCTIONS
// ============================================================================

/******************************************************************************
 * @brief Clears all text from the table cells.
 *
 * @section call_site
 * Called by `update_table_with_logs` before repopulating.
 *
 * @return void
 *
 * Implementation Steps
 * 1. iterate through all rows.
 * 2. Set columns 0-3 to empty string.
 ******************************************************************************/
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

/******************************************************************************
 * @brief Internal mock data provider for standalone UI development/simulator.
 * This is only compiled if MINIGUI_USE_MOCK_LOGS is defined.
 *
 * @param logs Output buffer
 * @param max_count Max logs
 * @param filter Data filter
 * @return Number of logs returned
 *
 * Implementation Steps
 * 1. Define a static array of mock log entries.
 * 2. Get current time.
 * 3. Loop through mock entries.
 * 4. Filter based on source.
 * 5. Formatting timestamp and copy data to output buffer.
 * 6. Return count of added logs.
 ******************************************************************************/
static size_t internal_get_logs(minigui_log_entry_t *logs, size_t max_count, const char *filter) {
#ifdef MINIGUI_USE_MOCK_LOGS
    static const struct {
        const char *src;
        const char *lvl;
        const char *msg;
    } mock_data[] = {
        {"LVGL", "INFO", "Standalone MiniGUI initialized"},
        {"USER", "DEBUG", "Mock log provider active"},
        {"ESP", "INFO", "System starting (simulator mode)"},
        {"USER", "WARN", "External app_bridge not detected"},
        {"LVGL", "DEBUG", "Table layout recalculated for 800px"},
        {"USER", "INFO", "Iterating on standalone UI..."}
    };

    size_t count = sizeof(mock_data) / sizeof(mock_data[0]);
    size_t added = 0;
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    for (size_t i = 0; i < count && added < max_count; i++) {
        if (filter && strcmp(filter, "ALL") != 0 && strcmp(mock_data[i].src, filter) != 0) continue;

        strftime(logs[added].timestamp, sizeof(logs[added].timestamp), "%H:%M:%S", tm_info);
        strncpy(logs[added].source, mock_data[i].src, sizeof(logs[added].source) - 1);
        strncpy(logs[added].level, mock_data[i].lvl, sizeof(logs[added].level) - 1);
        strncpy(logs[added].message, mock_data[i].msg, sizeof(logs[added].message) - 1);
        added++;
    }
    return added;
#else
    (void)logs; (void)max_count; (void)filter;
    return 0; // No logs if no provider and no mock
#endif
}

/******************************************************************************
 * @brief Fetches logs and updates the table UI.
 *
 * @section call_site
 * Called by refresh button, filter change, or init timer.
 *
 * @param filter The source string to filter by (or "ALL").
 *
 * @return void
 *
 * Implementation Steps
 * 1. Show "Loading..." message in table.
 * 2. Allocate memory for temp log buffer.
 * 3. Fetch logs from provider (or mock).
 * 4. Resize table rows to match log count.
 * 5. Populate table cells with log data.
 * 6. Free temp buffer.
 ******************************************************************************/
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
    minigui_log_entry_t *logs = (minigui_log_entry_t*)malloc(
        MINIGUI_MAX_LOGS * sizeof(minigui_log_entry_t));

    if (!logs) {
        LV_LOG_ERROR("Failed to allocate memory for logs");
        lv_table_set_row_cnt(data_table, 1);
        lv_table_set_cell_value(data_table, 0, 3, "Memory error");
        return;
    }

    // 1. Try the external registered provider first
    size_t count = 0;
    if (global_log_provider) {
        count = global_log_provider(logs, MINIGUI_MAX_LOGS, filter);
    }
    // 2. Fall back to internal mock (only compiled/active if MINIGUI_USE_MOCK_LOGS is defined)
    else {
        count = internal_get_logs(logs, MINIGUI_MAX_LOGS, filter);
    }

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

    LV_LOG_USER("Log table refreshed with %zu entries", count);
}

/******************************************************************************
 * @brief Handle refresh button click
 *
 * @param e LVGL event
 *
 * Implementation Steps
 * 1. Get current filter value from dropdown.
 * 2. Call `update_table_with_logs`.
 ******************************************************************************/
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

/******************************************************************************
 * @brief Handle filter dropdown change
 *
 * @param e LVGL event
 *
 * Implementation Steps
 * 1. Get new filter str.
 * 2. Call `update_table_with_logs`.
 ******************************************************************************/
static void filter_event_cb(lv_event_t * e) {
    lv_obj_t * dropdown = lv_event_get_target(e);
    char filter_buf[16];
    lv_dropdown_get_selected_str(dropdown, filter_buf, sizeof(filter_buf));

    update_table_with_logs(filter_buf);
}

/******************************************************************************
 * @brief Timer callback for initial load
 *
 * @param t Timer
 *
 * Implementation Steps
 * 1. Verify "ALL" logs.
 * 2. Delete the timer.
 ******************************************************************************/
static void deferred_load_cb(lv_timer_t * t) {
    update_table_with_logs("ALL");
    lv_timer_del(t);
}

// ============================================================================
// PUBLIC API
// ============================================================================

/******************************************************************************
 * @brief Manually triggers a refresh of the log table.
 *
 * @param filter The source filter string (e.g., "WIFI") or NULL/ALL.
 *
 * Implementation Steps
 * 1. Call `update_table_with_logs`.
 ******************************************************************************/
void refresh_log_table(const char *filter) {
    update_table_with_logs(filter ? filter : "ALL");
}

// ============================================================================
// CORRECTED FIXED LAYOUT (No percentages for LVGL table columns)
// ============================================================================

/******************************************************************************
 * @brief Calculate optimal column widths for 800px display
 *
 * @section call_site
 * Called on init and resize.
 *
 * Implementation Steps
 * 1. Determine available width (default 800px).
 * 2. Subtract padding.
 * 3. Set fixed widths for Time, Source, Level.
 * 4. Give remaining width to Message.
 ******************************************************************************/
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

/******************************************************************************
 * @brief Recalculate layout when parent size changes
 *
 * @param e LVGL event
 *
 * Implementation Steps
 * 1. Call `calculate_table_layout`.
 ******************************************************************************/
static void parent_size_changed_cb(lv_event_t * e) {
    (void)e; // Unused parameter
    calculate_table_layout();
}

// ============================================================================
// SIMPLIFIED SCREEN CREATION WITH PROPER SIZING
// ============================================================================

/******************************************************************************
 * @brief Creates the Logs screen object.
 *
 * @param parent The main content area.
 *
 * Implementation Steps
 * 1. Set styles (black bg) on parent.
 * 2. Create Header Container (40px height).
 * 3. Create Header Label ("TIME | FROM | ...").
 * 4. Create Filter Dropdown (aligned right).
 * 5. Create Refresh Button (aligned right).
 * 6. Create Data Table (filling remaining height).
 * 7. Configure Table columns and styling.
 * 8. Set initial "Loading..." state.
 * 9. Start timer for deferred data load.
 ******************************************************************************/
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
    lv_obj_set_style_text_font(header_lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(header_lbl, lv_color_white(), 0);
    lv_obj_set_pos(header_lbl, 5, 5); // Fixed position
    lv_obj_set_size(header_lbl, 400, 30);

    // FILTER DROPDOWN (right side)
    filter_dropdown = lv_dropdown_create(header_cont);
    lv_dropdown_set_options(filter_dropdown, "ALL\nESP\nLVGL\nUSER");
    lv_obj_set_size(filter_dropdown, 100, 30);
    lv_obj_set_pos(filter_dropdown, 600, 5); // Right-aligned
    lv_obj_set_style_text_font(filter_dropdown, &lv_font_montserrat_16, 0);
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
    lv_obj_set_style_text_font(refresh_label, &lv_font_montserrat_20, 0);
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
    lv_obj_set_style_text_font(data_table, &lv_font_montserrat_16, 0);
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
