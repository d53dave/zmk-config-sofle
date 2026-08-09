/*
 * Sets the underglow color based on the highest active layer. Colors
 * below are arbitrary defaults - tune to taste.
 *
 * This only calls the existing RGB underglow API
 * (zmk_rgb_underglow_set_hsb) from a plain event listener - no LVGL,
 * no display, no timers - deliberately kept separate from the
 * (currently paused, see NOTES.md) custom OLED status screen work so
 * it can't be affected by whatever's wrong there.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/keymap.h>
#include <zmk/rgb_underglow.h>

/* Index = layer number (default_layer=0, lower_layer=1, raise_layer=2
 * in sofle.keymap). Default matches the purple already bound to
 * &rgb_ug RGB_COLOR_HSB(292,71,51) in raise_layer. */
static const struct zmk_led_hsb layer_colors[] = {
    {.h = 292, .s = 71, .b = 51}, /* 0: default - purple */
    {.h = 30, .s = 90, .b = 35},  /* 1: lower - amber */
    {.h = 190, .s = 60, .b = 30}, /* 2: raise - teal */
};

static int rgb_layer_color_listener_cb(const zmk_event_t *eh) {
    zmk_keymap_layer_index_t index = zmk_keymap_highest_layer_active();

    if (index >= ARRAY_SIZE(layer_colors)) {
        index = 0;
    }

    zmk_rgb_underglow_set_hsb(layer_colors[index]);

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(rgb_layer_color_listener, rgb_layer_color_listener_cb);
ZMK_SUBSCRIPTION(rgb_layer_color_listener, zmk_layer_state_changed);
