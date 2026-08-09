/*
 * Custom status screen, step 1 of an incremental rebuild. Overrides
 * ZMK's weak zmk_display_status_screen() (see zmkfirmware/zmk
 * app/src/display/main.c). Text info only for now - no cat widget, no
 * timers/animation - added back incrementally once this is confirmed
 * stable on hardware. See NOTES.md.
 */

#include <zephyr/kernel.h>
#include <lvgl.h>

#if (!IS_ENABLED(CONFIG_ZMK_SPLIT)) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include "widgets/info.h"
static struct zmk_widget_info info_widget;
#else
#include "widgets/link_status.h"
static struct zmk_widget_link_status link_status_widget;
#endif

lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);

#if (!IS_ENABLED(CONFIG_ZMK_SPLIT)) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    zmk_widget_info_init(&info_widget, screen);
    lv_obj_align(zmk_widget_info_obj(&info_widget), LV_ALIGN_CENTER, 0, 0);
#else
    zmk_widget_link_status_init(&link_status_widget, screen);
    lv_obj_align(zmk_widget_link_status_obj(&link_status_widget), LV_ALIGN_CENTER, 0, 0);
#endif

    return screen;
}
