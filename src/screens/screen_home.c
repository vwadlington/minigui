/******************************************************************************
 ******************************************************************************
 ** @brief     Home Screen Implementation.
 **
 **            Provides a dashboard view with environmental and status data
 **            cards. This is the default landing screen for the UI.
 **
 **            @section screen_home.c - Dashboard UI implementation.
 ******************************************************************************
 ******************************************************************************/

/******************************************************************************
 ******************************************************************************
 ** 1. ESP-IDF / FreeRTOS Core (Framework)
 ******************************************************************************
 ******************************************************************************/
// None

/******************************************************************************
 ******************************************************************************
 ** 2. Managed Espressif Components (External Managed Components)
 ******************************************************************************
 ******************************************************************************/
// None (lvgl included via header)

/******************************************************************************
 ******************************************************************************
 ** 3. Project-Specific Components (Local)
 ******************************************************************************
 ******************************************************************************/
#include "screens/screen_home.h"
#include "minigui.h"

// ============================================================================
// PRIVATE HELPER FUNCTIONS
// ============================================================================

/******************************************************************************
 ******************************************************************************
 ** @brief Helper to create a stylized info card.
 **
 ** @section call_site Called from:
 ** - create_screen_home() multiple times (Indoor, Outdoor, Status).
 **
 ** @section dependencies Required Headers:
 ** - lvgl.h (for widget construction)
 **
 ** @param parent (lv_obj_t*): The container object to hold the card.
 ** @param title (const char*): Title text (e.g., "Indoor").
 ** @param value (const char*): Value text (e.g., "72°F").
 ** @param color (lv_color_t): Card background color.
 **
 ** @section pointers 
 ** - parent: Managed by caller.
 ** - title/value: Read-only string literals.
 **
 ** @section variables Internal Variables:
 ** - @c card (lv_obj_t*): The primary container for the card.
 ** - @c lbl_title (lv_obj_t*): Label for the card heading.
 ** - @c lbl_val (lv_obj_t*): Label for the main data value.
 **
 ** @return void
 **
 ** Implementation Steps:
 ** 1. Create a container object (@c card) on the parent.
 ** 2. Configure card geometry (220x150) and background color.
 ** 3. Create the title label using Montserrat-24 and align top-left.
 ** 4. Create the value label using Montserrat-36 and center it.
 ******************************************************************************
 ******************************************************************************/
static void create_info_card(lv_obj_t *parent, const char* title, const char* value, lv_color_t color) {
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, 220, 150);
    lv_obj_set_style_bg_color(card, color, 0);
    lv_obj_set_style_border_width(card, 0, 0);

    lv_obj_t *lbl_title = lv_label_create(card);
    lv_label_set_text(lbl_title, title);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_24, 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *lbl_val = lv_label_create(card);
    lv_label_set_text(lbl_val, value);
    lv_obj_set_style_text_font(lbl_val, &lv_font_montserrat_36, 0);
    lv_obj_center(lbl_val);
}

// ============================================================================
// PUBLIC API FUNCTIONS
// ============================================================================

/******************************************************************************
 ******************************************************************************
 ** @brief Creates the Home screen object.
 **
 ** @section call_site Called from:
 ** - minigui_switch_screen() via the @c screen_creators mapping.
 **
 ** @section dependencies Required Headers:
 ** - lvgl.h (for flex layout and styling)
 **
 ** @param parent (lv_obj_t*): The main content area container.
 **
 ** @section pointers 
 ** - parent: Owned by minigui.c.
 **
 ** @section variables Internal Variables:
 ** - @c cont (lv_obj_t*): Flex container for the cards.
 **
 ** @return void
 **
 ** Implementation Steps:
 ** 1. Set background color to solid black (clean slate).
 ** 2. Create a full-size flex container to wrap cards.
 ** 3. Configure space-evenly alignment for cards.
 ** 4. Instantiate Indoor (Blue), Outdoor (Orange), and Status (Green) cards.
 ******************************************************************************
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
