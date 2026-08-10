/*
 * Custom status screen - step 0 (mechanism test).
 *
 * Solid fill, no widgets, no labels, no events, no timers. The last two
 * custom status screen attempts (see NOTES.md) both crashed hardware
 * identically (keyboard input dead, display showing static noise), and
 * that was never root-caused - even a text-labels-only version crashed
 * the same way. This step tests just the override mechanism itself
 * (config/zephyr/module.yml + CMakeLists.txt + Kconfig +
 * zmk_display_status_screen() weak-symbol override) in isolation, before
 * adding any widget content back in.
 *
 * A solid white fill is deliberately used instead of black/off, so a
 * successful boot is visually unambiguous: fully-lit screen means this
 * step worked, blank/dark means something didn't initialize, static
 * noise means the same crash as before.
 */

#include <lvgl.h>

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    lv_obj_set_style_bg_color(screen, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    return screen;
}
