/*
 * Peripheral-half link status text. The peripheral doesn't have
 * layer/WPM/output state (that only exists on the central half), so
 * this is the one thing it can meaningfully show: whether it's
 * connected to the central half.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/split/bluetooth/peripheral.h>
#include <zmk/events/split_peripheral_status_changed.h>

#include "link_status.h"

static lv_obj_t *label;

struct link_status_state {
    bool connected;
};

static struct link_status_state link_status_get_state(const zmk_event_t *_eh) {
    return (struct link_status_state){.connected = zmk_split_bt_peripheral_is_connected()};
}

static void link_status_update_cb(struct link_status_state state) {
    lv_label_set_text(label, state.connected ? LV_SYMBOL_OK " linked" : LV_SYMBOL_CLOSE " no link");
}

ZMK_DISPLAY_WIDGET_LISTENER(link_status_listener, struct link_status_state,
                            link_status_update_cb, link_status_get_state)
ZMK_SUBSCRIPTION(link_status_listener, zmk_split_peripheral_status_changed);

int zmk_widget_link_status_init(struct zmk_widget_link_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(widget->obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(widget->obj, 0, 0);
    lv_obj_set_style_pad_all(widget->obj, 0, 0);
    lv_obj_clear_flag(widget->obj, LV_OBJ_FLAG_SCROLLABLE);

    label = lv_label_create(widget->obj);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    link_status_listener_init();

    return 0;
}

lv_obj_t *zmk_widget_link_status_obj(struct zmk_widget_link_status *widget) { return widget->obj; }
