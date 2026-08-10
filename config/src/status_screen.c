/*
 * Custom status screen - step 0b (static label test).
 *
 * Step 0 (solid white background fill, no content) confirmed the
 * override mechanism itself is safe on this hardware - keyboard stayed
 * functional. But the screen came out blank/black instead of the
 * expected lit white, most likely a color-polarity thing (this display's
 * devicetree has `inversion-on` set, and the known-working built-in
 * screen look is dark background + light text, the opposite of what a
 * white *background fill* would need).
 *
 * Rather than guess at colors again, this step drops the background fill
 * and uses a single static text label instead - the same rendering
 * primitive the built-in screen's text already renders correctly with,
 * using default (theme-provided) styling instead of overriding colors.
 * Still no events/timers/dynamic data - this is also the first real
 * piece of step 1 (left-side text widgets).
 */

#include <lvgl.h>

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    lv_obj_t *label = lv_label_create(screen);
    lv_label_set_text(label, "status screen ok");
    lv_obj_center(label);

    return screen;
}
