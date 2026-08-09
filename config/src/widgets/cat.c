/*
 * Small animated "bongo cat" style widget. Built from plain LVGL
 * shapes (a circle head, two rectangle paws) instead of bitmap images,
 * so it can't suffer the bitmap/color-format rendering corruption the
 * default status screen's icons had.
 *
 * On the central half it reacts to typing speed (WPM). On the
 * peripheral half (no WPM data available) it reacts to split link
 * status instead.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/event_manager.h>

#include "widgets/cat.h"

#define CAT_W 34
#define CAT_H 32
#define HEAD_D 16
#define PAW_W 9
#define PAW_H 6
#define PAW_Y_DOWN 22
#define PAW_Y_UP 15
#define PAW_LEFT_X 1
#define PAW_RIGHT_X (CAT_W - PAW_W - 1)

static lv_obj_t *paw_left;
static lv_obj_t *paw_right;
static lv_timer_t *anim_timer;
static bool paws_up;

static void style_paw(lv_obj_t *paw) {
    lv_obj_set_size(paw, PAW_W, PAW_H);
    lv_obj_set_style_radius(paw, 2, 0);
    lv_obj_set_style_bg_color(paw, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(paw, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(paw, 0, 0);
    lv_obj_clear_flag(paw, LV_OBJ_FLAG_SCROLLABLE);
}

static void anim_timer_cb(lv_timer_t *timer) {
    paws_up = !paws_up;
    lv_obj_set_y(paw_left, paws_up ? PAW_Y_UP : PAW_Y_DOWN);
    lv_obj_set_y(paw_right, paws_up ? PAW_Y_DOWN : PAW_Y_UP);
}

/* period_ms == 0 means "stop and rest both paws down". */
static void set_anim_period(uint32_t period_ms) {
    if (anim_timer == NULL) {
        return;
    }

    if (period_ms == 0) {
        lv_timer_pause(anim_timer);
        paws_up = false;
        lv_obj_set_y(paw_left, PAW_Y_DOWN);
        lv_obj_set_y(paw_right, PAW_Y_DOWN);
        return;
    }

    lv_timer_set_period(anim_timer, period_ms);
    lv_timer_resume(anim_timer);
}

#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

/* Central half (or non-split build): react to typing speed. */

#include <zmk/events/wpm_state_changed.h>
#include <zmk/wpm.h>

struct cat_event_state {
    uint8_t wpm;
};

static struct cat_event_state cat_event_get_state(const zmk_event_t *eh) {
    return (struct cat_event_state){.wpm = zmk_wpm_get_state()};
}

static void cat_event_update_cb(struct cat_event_state state) {
    uint32_t period;

    if (state.wpm == 0) {
        period = 0;
    } else if (state.wpm < 20) {
        period = 700;
    } else if (state.wpm < 50) {
        period = 350;
    } else if (state.wpm < 80) {
        period = 200;
    } else {
        period = 120;
    }

    set_anim_period(period);
}

ZMK_DISPLAY_WIDGET_LISTENER(cat_event_listener, struct cat_event_state, cat_event_update_cb,
                            cat_event_get_state)
ZMK_SUBSCRIPTION(cat_event_listener, zmk_wpm_state_changed);

#define CAT_EVENT_LISTENER_INIT() cat_event_listener_init()

#else

/* Peripheral half: react to split link status instead - no WPM data
 * is available here. */

#include <zmk/split/bluetooth/peripheral.h>
#include <zmk/events/split_peripheral_status_changed.h>

struct cat_event_state {
    bool connected;
};

static struct cat_event_state cat_event_get_state(const zmk_event_t *eh) {
    return (struct cat_event_state){.connected = zmk_split_bt_peripheral_is_connected()};
}

static void cat_event_update_cb(struct cat_event_state state) {
    /* Slow ambient bounce while linked, frozen paws when not. */
    set_anim_period(state.connected ? 900 : 0);
}

ZMK_DISPLAY_WIDGET_LISTENER(cat_event_listener, struct cat_event_state, cat_event_update_cb,
                            cat_event_get_state)
ZMK_SUBSCRIPTION(cat_event_listener, zmk_split_peripheral_status_changed);

#define CAT_EVENT_LISTENER_INIT() cat_event_listener_init()

#endif

int zmk_widget_cat_init(struct zmk_widget_cat *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, CAT_W, CAT_H);
    lv_obj_set_style_bg_opa(widget->obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(widget->obj, 0, 0);
    lv_obj_set_style_pad_all(widget->obj, 0, 0);
    lv_obj_clear_flag(widget->obj, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *head = lv_obj_create(widget->obj);
    lv_obj_set_size(head, HEAD_D, HEAD_D);
    lv_obj_set_pos(head, (CAT_W - HEAD_D) / 2, 0);
    lv_obj_set_style_radius(head, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(head, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(head, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(head, 0, 0);
    lv_obj_clear_flag(head, LV_OBJ_FLAG_SCROLLABLE);

    paw_left = lv_obj_create(widget->obj);
    style_paw(paw_left);
    lv_obj_set_pos(paw_left, PAW_LEFT_X, PAW_Y_DOWN);

    paw_right = lv_obj_create(widget->obj);
    style_paw(paw_right);
    lv_obj_set_pos(paw_right, PAW_RIGHT_X, PAW_Y_DOWN);

    anim_timer = lv_timer_create(anim_timer_cb, 500, NULL);
    lv_timer_pause(anim_timer);

    CAT_EVENT_LISTENER_INIT();

    return 0;
}

lv_obj_t *zmk_widget_cat_obj(struct zmk_widget_cat *widget) { return widget->obj; }
