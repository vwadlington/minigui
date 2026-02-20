
/******************************************************************************
 ******************************************************************************
 * 1. ESP-IDF / FreeRTOS Core (Framework)
 ******************************************************************************
 ******************************************************************************/
// None

/******************************************************************************
 ******************************************************************************
 * 2. Managed Espressif Components (External Managed Components)
 ******************************************************************************
 ******************************************************************************/
// None (lvgl included via header)

/******************************************************************************
 ******************************************************************************
 * 3. Project-Specific Components (Local)
 ******************************************************************************
 ******************************************************************************/
#include "screens/screen_home.h"
#include "minigui.h" // For access if needed

// ============================================================================
// PRIVATE HELPER FUNCTIONS
// ============================================================================

/******************************************************************************
 * @brief Helper to create a stylized info card.
 *
 * @section call_site
 * Called multiple times by `create_screen_home`.
 *
 * @section dependencies
 * - `lvgl`: For object creation and styling.
 *
 * @param parent The container object.
 * @param title Title text (e.g., "Indoor").
 * @param value Value text (e.g., "72°F").
 * @param color Card background color.
 *
 * @section pointers
 * - `parent`: Managed by caller.
 * - `title`/`value`: Read-only string literals.
 *
 * @section variables
 * - `card`: The container object for this individual card. Rationale: Groups title and value.
 * - `lbl_title`: The label for the card's title. Rationale: Displays "Indoor", "Outdoor", etc.
 * - `lbl_val`: The label for the main data value. Rationale: Displays temperature or status.
 *
 * @return void
 *
 * Implementation Steps
 * 1. Create a container object (`card`) on the parent.
 * 2. Set the card's size, background color, and border style.
 * 3. Create the title label, set its text, and align it to the top-left.
 * 4. Create the value label, set its text, apply a large font, and center it in the card.
 ******************************************************************************/
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

// ============================================================================
// PUBLIC API FUNCTIONS
// ============================================================================

/******************************************************************************
 * @brief Creates the Home screen object
 *
 * @section call_site
 * Called by `minigui_switch_screen` via the screen creator array.
 *
 * @param parent The main content area.
 *
 * @return void
 *
 * Implementation Steps
 * 1. Set the background color and opacity of the parent to ensure a clean slate.
 * 2. Create a flex container (`cont`) to hold the info cards.
 * 3. Configure the container to center children and wrap them if necessary.
 * 4. Create an "Indoor" temperature card (Blue).
 * 5. Create an "Outdoor" temperature card (Orange).
 * 6. Create a "Status" card (Green).
 ******************************************************************************/
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
