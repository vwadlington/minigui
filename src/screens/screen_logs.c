/******************************************************************************
 ******************************************************************************
 ** @brief     Logs Screen Implementation.
 **
 **            Displays a scrollable table of system logs with source filtering
 **            and manual refresh capabilities.
 **
 **            @section screen_logs.c - Log dashboard implementation.
 ******************************************************************************
 ******************************************************************************/

/******************************************************************************
 ******************************************************************************
 ** 1. ESP-IDF / FreeRTOS Core (Framework)
 ******************************************************************************
 ******************************************************************************/
#include <stdlib.h>  // For malloc/free
#include <string.h>  // For strcpy, strcmp, etc.
#include <time.h>    // For mock timestamps

/******************************************************************************
 ******************************************************************************
 ** 2. Managed Espressif Components (External Managed Components)
 ******************************************************************************
 ******************************************************************************/
#include "lvgl.h"

/******************************************************************************
 ******************************************************************************
 ** 3. Project-Specific Components (Local)
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
 ******************************************************************************
 ** @brief Reference to the main data table.
 **
 ** @section scope Internal Scope:
 ** - Internal to screen_logs.c.
 **
 ** @section rationale Rationale:
 ** - Displays the list of log entries. Need handle to update data.
 ******************************************************************************
 ******************************************************************************/
static lv_obj_t *data_table = NULL;

/******************************************************************************
 ******************************************************************************
 ** @brief Reference to the source filter dropdown.
 **
 ** @section scope Internal Scope:
 ** - Internal to screen_logs.c.
 **
 ** @section rationale Rationale:
 ** - Allows user to filter logs by source (e.g., wifi, system).
 ******************************************************************************
 ******************************************************************************/
static lv_obj_t *filter_dropdown = NULL;

/******************************************************************************
 ******************************************************************************
 ** @brief Reference to the parent screen object.
 **
 ** @section scope Internal Scope:
 ** - Internal to screen_logs.c.
 **
 ** @section rationale Rationale:
 ** - Used to calculate available width for the table.
 ******************************************************************************
 ******************************************************************************/
static lv_obj_t *log_screen_parent = NULL;

/******************************************************************************
 ******************************************************************************
 ** @brief Registered callback to fetch logs.
 **
 ** @section scope Internal Scope:
 ** - Internal to screen_logs.c.
 **
 ** @section rationale Rationale:
 ** - Decouples UI from the backend logging system.
 ******************************************************************************
 ******************************************************************************/
static minigui_log_provider_t global_log_provider = NULL;

// ============================================================================
// PUBLIC SETTERS
// ============================================================================

/******************************************************************************
 ******************************************************************************
 ** @brief Register a log provider.
 **
 ** @section call_site Called from:
 ** - Application initialization.
 **
 ** @section dependencies Required Headers:
 ** - None
 **
 ** @param provider (minigui_log_provider_t): Callback to fetch logs.
 **
 ** @section pointers 
 ** - provider: Owned by caller.
 **
 ** @section variables 
 ** - None
 **
 ** @return void
 **
 ** Implementation Steps:
 ** 1. Store the provider in @c global_log_provider.
 ******************************************************************************
 ******************************************************************************/
void minigui_set_log_provider(minigui_log_provider_t provider) {
    global_log_provider = provider;
}

// ============================================================================
// PRIVATE HELPER FUNCTIONS
// ============================================================================

/******************************************************************************
 ******************************************************************************
 ** @brief Clears all text from the table cells.
 **
 ** @section call_site Called from:
 ** - update_table_with_logs() before repopulating.
 **
 ** @section dependencies Required Headers:
 ** - lvgl.h (table manipulation)
 **
 ** @param None
 **
 ** @section pointers 
 ** - None
 **
 ** @section variables 
 ** - None
 **
 ** @return void
 **
 ** Implementation Steps:
 ** 1. Iterate through all rows in the data_table.
 ** 2. Set columns 0-3 (Time, Source, Level, Message) to empty strings.
 ******************************************************************************
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
 ******************************************************************************
 ** @brief Internal mock data provider for standalone UI development.
 **
 ** @section call_site Called from:
 ** - update_table_with_logs() if no provider is registered.
 **
 ** @section dependencies Required Headers:
 ** - time.h (for simulation)
 **
 ** @param logs (minigui_log_entry_t*): Output buffer.
 ** @param max_count (size_t): Buffer capacity.
 ** @param filter (const char*): Source filter.
 **
 ** @section pointers 
 ** - logs: Pointer owned by caller.
 ** - filter: Read-only string.
 **
 ** @section variables Internal Variables:
 ** - @c mock_data (static struct[]): Simulated database.
 ** - @c now (time_t): Simulated timestamp source.
 **
 ** @return size_t: Number of logs returned.
 **
 ** Implementation Steps:
 ** 1. Return 0 if MINIGUI_USE_MOCK_LOGS is not defined.
 ** 2. Iterate mock_data and apply filter.
 ** 3. Copy formatted timestamp and strings to output buffer.
 ******************************************************************************
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
 ******************************************************************************
 ** @brief Fetches logs and updates the table UI.
 **
 ** @section call_site Called from:
 ** - refresh_button_cb()
 ** - filter_event_cb()
 ** - deferred_load_cb()
 **
 ** @section dependencies Required Headers:
 ** - stdlib.h (malloc/free)
 **
 ** @param filter (const char*): The source string to filter by (or "ALL").
 **
 ** @section pointers 
 ** - filter: Read-only string.
 **
 ** @section variables Internal Variables:
 ** - @c logs (minigui_log_entry_t*): Heap-allocated temp buffer for fetch.
 **
 ** @return void
 **
 ** Implementation Steps:
 ** 1. Show "Loading..." message and force immediate screen refresh.
 ** 2. Allocate heap buffer for log retrieval.
 ** 3. Fetch data from global provider or fallback mock.
 ** 4. Update table row count and populate cells.
 ** 5. Free temporary heap memory.
 ******************************************************************************
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
 ******************************************************************************
 ** @brief Handle refresh button click.
 **
 ** @section call_site Called from:
 ** - Refresh button LV_EVENT_CLICKED.
 **
 ** @section dependencies Required Headers:
 ** - None (Standard UI handler)
 **
 ** @param e (lv_event_t*): LVGL event object.
 **
 ** @section pointers 
 ** - e: Owned by LVGL.
 **
 ** @section variables 
 ** - None
 **
 ** @return void
 **
 ** Implementation Steps:
 ** 1. Retrieve current filter string from the dropdown.
 ** 2. Trigger @c update_table_with_logs immediately.
 ******************************************************************************
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
 ******************************************************************************
 ** @brief Handle filter dropdown change.
 **
 ** @section call_site Called from:
 ** - Filter dropdown LV_EVENT_VALUE_CHANGED.
 **
 ** @section dependencies Required Headers:
 ** - None (Standard UI handler)
 **
 ** @param e (lv_event_t*): LVGL event object.
 **
 ** @section pointers 
 ** - e: Owned by LVGL.
 **
 ** @section variables 
 ** - None
 **
 ** @return void
 **
 ** Implementation Steps:
 ** 1. Get the new selected string from the dropdown target.
 ** 2. Refresh the table with the new filter.
 ******************************************************************************
 ******************************************************************************/
static void filter_event_cb(lv_event_t * e) {
    lv_obj_t * dropdown = lv_event_get_target(e);
    char filter_buf[16];
    lv_dropdown_get_selected_str(dropdown, filter_buf, sizeof(filter_buf));

    update_table_with_logs(filter_buf);
}

/******************************************************************************
 ******************************************************************************
 ** @brief Timer callback for initial load.
 **
 ** @section call_site Called from:
 ** - Deferred timer created in create_screen_logs().
 **
 ** @section dependencies Required Headers:
 ** - None
 **
 ** @param t (lv_timer_t*): Pointer to the triggering timer.
 **
 ** @section pointers 
 ** - t: Managed by LVGL.
 **
 ** @section variables 
 ** - None
 **
 ** @return void
 **
 ** Implementation Steps:
 ** 1. Call update_table_with_logs("ALL").
 ** 2. Self-destruct the timer handle.
 ******************************************************************************
 ******************************************************************************/
static void deferred_load_cb(lv_timer_t * t) {
    update_table_with_logs("ALL");
    lv_timer_del(t);
}

// ============================================================================
// PUBLIC API
// ============================================================================

/******************************************************************************
 ******************************************************************************
 ** @brief Manually triggers a refresh of the log table.
 **
 ** @section call_site Called from:
 ** - External modules to force a sync.
 **
 ** @section dependencies Required Headers:
 ** - None
 **
 ** @param filter (const char*): Source filter string or NULL/ALL.
 **
 ** @section pointers 
 ** - filter: Read-only string.
 **
 ** @section variables 
 ** - None
 **
 ** @return void
 **
 ** Implementation Steps:
 ** 1. Transparently proxy callers to internal update logic.
 ******************************************************************************
 ******************************************************************************/
void refresh_log_table(const char *filter) {
    update_table_with_logs(filter ? filter : "ALL");
}

// ============================================================================
// CORRECTED FIXED LAYOUT (No percentages for LVGL table columns)
// ============================================================================

/******************************************************************************
 ******************************************************************************
 ** @brief Calculate optimal column widths based on available width.
 **
 ** @section call_site Called from:
 ** - create_screen_logs()
 ** - parent_size_changed_cb()
 **
 ** @section dependencies Required Headers:
 ** - lvgl.h (geometry API)
 **
 ** @param None
 **
 ** @section pointers 
 ** - None
 **
 ** @section variables 
 ** - None
 **
 ** @return void
 **
 ** Implementation Steps:
 ** 1. Determine actual width (defaulting to 800px if unknown).
 ** 2. Subtract padding for available drawing area.
 ** 3. Apply static widths for small columns and flexible remainder for message.
 ******************************************************************************
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
 ******************************************************************************
 ** @brief Recalculate layout when parent size changes.
 **
 ** @section call_site Called from:
 ** - parent object LV_EVENT_SIZE_CHANGED.
 **
 ** @section dependencies Required Headers:
 ** - None
 **
 ** @param e (lv_event_t*): LVGL event object.
 **
 ** @section pointers 
 ** - e: Owned by LVGL.
 **
 ** @section variables 
 ** - None
 **
 ** @return void
 **
 ** Implementation Steps:
 ** 1. Invoke @c calculate_table_layout with new parent context.
 ******************************************************************************
 ******************************************************************************/
static void parent_size_changed_cb(lv_event_t * e) {
    (void)e; // Unused parameter
    calculate_table_layout();
}

// ============================================================================
// SIMPLIFIED SCREEN CREATION WITH PROPER SIZING
// ============================================================================

/******************************************************************************
 ******************************************************************************
 ** @brief Creates the Logs screen object.
 **
 ** @section call_site Called from:
 ** - minigui_switch_screen() via mapping.
 **
 ** @section dependencies Required Headers:
 ** - lvgl.h (for table and dropdown widgets)
 **
 ** @param parent (lv_obj_t*): The content area container.
 **
 ** @section pointers 
 ** - parent: Owned by minigui.c.
 **
 ** @section variables Internal Variables:
 ** - @c header_cont (lv_obj_t*): Top control bar.
 ** - @c data_table (lv_obj_t*): Core log display widget.
 **
 ** @return void
 **
 ** Implementation Steps:
 ** 1. Style the parent with 100% black background.
 ** 2. Construct the 40px fixed header with filter dropdown and refresh button.
 ** 3. Create and configure the LVGL table widget for the remainder.
 ** 4. Initialize "Loading" state and trigger deferred data fetch.
 ******************************************************************************
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
