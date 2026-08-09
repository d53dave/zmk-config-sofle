/*
 * Custom status screen. Overrides ZMK's weak zmk_display_status_screen()
 * (see zmkfirmware/zmk app/src/display/main.c). Layout is the same on
 * both halves: cat widget on the left, text info on the right - but
 * what drives the cat and what the text shows differs by split role
 * (see widgets/cat.c, widgets/info.c, widgets/link_status.c).
 */

#include <zephyr/kernel.h>
#include <lvgl.h>

#include "widgets/cat.h"

#if (!IS_ENABLED(CONFIG_ZMK_SPLIT)) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include "widgets/info.h"
static struct zmk_widget_info info_widget;
#else
#include "widgets/link_status.h"
static struct zmk_widget_link_status link_status_widget;
#endif

static struct zmk_widget_cat cat_widget;

lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);

    zmk_widget_cat_init(&cat_widget, screen);
    lv_obj_align(zmk_widget_cat_obj(&cat_widget), LV_ALIGN_LEFT_MID, 0, 0);

#if (!IS_ENABLED(CONFIG_ZMK_SPLIT)) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    zmk_widget_info_init(&info_widget, screen);
    lv_obj_set_size(zmk_widget_info_obj(&info_widget), 92, 32);
    lv_obj_align(zmk_widget_info_obj(&info_widget), LV_ALIGN_RIGHT_MID, 0, 0);
#else
    zmk_widget_link_status_init(&link_status_widget, screen);
    lv_obj_set_size(zmk_widget_link_status_obj(&link_status_widget), 92, 32);
    lv_obj_align(zmk_widget_link_status_obj(&link_status_widget), LV_ALIGN_RIGHT_MID, 0, 0);
#endif

    return screen;
}
