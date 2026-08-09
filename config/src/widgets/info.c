/*
 * Central-half info panel: active layer, WPM, and output (USB/BLE)
 * status as plain text/font-symbol labels - no bitmap icons, so this
 * can't suffer the corruption the default status screen's icons had.
 * Central-only: layer/WPM/endpoint state doesn't exist on the
 * peripheral half.
 */

#include <string.h>
#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/event_manager.h>

#include <zmk/events/layer_state_changed.h>
#include <zmk/keymap.h>

#include <zmk/events/wpm_state_changed.h>
#include <zmk/wpm.h>

#include <zmk/events/endpoint_changed.h>
#include <zmk/usb.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>
#if IS_ENABLED(CONFIG_ZMK_BLE)
#include <zmk/events/ble_active_profile_changed.h>
#endif

#include "widgets/info.h"

static lv_obj_t *layer_label;
static lv_obj_t *wpm_label;
static lv_obj_t *output_label;

/* --- layer --- */

struct info_layer_state {
    zmk_keymap_layer_index_t index;
    const char *label;
};

static struct info_layer_state info_layer_get_state(const zmk_event_t *eh) {
    zmk_keymap_layer_index_t index = zmk_keymap_highest_layer_active();
    return (struct info_layer_state){
        .index = index, .label = zmk_keymap_layer_name(zmk_keymap_layer_index_to_id(index))};
}

static void info_layer_update_cb(struct info_layer_state state) {
    char text[16] = {};

    if (state.label == NULL || strlen(state.label) == 0) {
        snprintf(text, sizeof(text), "Layer %i", state.index);
    } else {
        snprintf(text, sizeof(text), "%s", state.label);
    }

    lv_label_set_text(layer_label, text);
}

ZMK_DISPLAY_WIDGET_LISTENER(info_layer_listener, struct info_layer_state, info_layer_update_cb,
                            info_layer_get_state)
ZMK_SUBSCRIPTION(info_layer_listener, zmk_layer_state_changed);

/* --- wpm --- */

struct info_wpm_state {
    uint8_t wpm;
};

static struct info_wpm_state info_wpm_get_state(const zmk_event_t *eh) {
    return (struct info_wpm_state){.wpm = zmk_wpm_get_state()};
}

static void info_wpm_update_cb(struct info_wpm_state state) {
    char text[12] = {};

    snprintf(text, sizeof(text), "%i wpm", state.wpm);
    lv_label_set_text(wpm_label, text);
}

ZMK_DISPLAY_WIDGET_LISTENER(info_wpm_listener, struct info_wpm_state, info_wpm_update_cb,
                            info_wpm_get_state)
ZMK_SUBSCRIPTION(info_wpm_listener, zmk_wpm_state_changed);

/* --- output status --- */

struct info_output_state {
    struct zmk_endpoint_instance selected_endpoint;
    enum zmk_transport preferred_transport;
    bool active_profile_connected;
    bool active_profile_bonded;
};

static struct info_output_state info_output_get_state(const zmk_event_t *_eh) {
    return (struct info_output_state){
        .selected_endpoint = zmk_endpoint_get_selected(),
        .preferred_transport = zmk_endpoint_get_preferred_transport(),
        .active_profile_connected = zmk_ble_active_profile_is_connected(),
        .active_profile_bonded = !zmk_ble_active_profile_is_open(),
    };
}

static void info_output_update_cb(struct info_output_state state) {
    char text[20] = {};

    enum zmk_transport transport = state.selected_endpoint.transport;
    bool connected = transport != ZMK_TRANSPORT_NONE;

    if (!connected) {
        transport = state.preferred_transport;
    }

    switch (transport) {
    case ZMK_TRANSPORT_NONE:
        strcat(text, LV_SYMBOL_CLOSE);
        break;

    case ZMK_TRANSPORT_USB:
        strcat(text, LV_SYMBOL_USB);
        if (!connected) {
            strcat(text, " " LV_SYMBOL_CLOSE);
        }
        break;

    case ZMK_TRANSPORT_BLE:
        if (state.active_profile_bonded) {
            snprintf(text, sizeof(text), LV_SYMBOL_WIFI " %i %s",
                     state.selected_endpoint.ble.profile_index + 1,
                     state.active_profile_connected ? LV_SYMBOL_OK : LV_SYMBOL_CLOSE);
        } else {
            snprintf(text, sizeof(text), LV_SYMBOL_WIFI " %i " LV_SYMBOL_SETTINGS,
                     state.selected_endpoint.ble.profile_index + 1);
        }
        break;
    }

    lv_label_set_text(output_label, text);
}

ZMK_DISPLAY_WIDGET_LISTENER(info_output_listener, struct info_output_state,
                            info_output_update_cb, info_output_get_state)
ZMK_SUBSCRIPTION(info_output_listener, zmk_endpoint_changed);
#if IS_ENABLED(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(info_output_listener, zmk_ble_active_profile_changed);
#endif

/* --- init --- */

int zmk_widget_info_init(struct zmk_widget_info *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(widget->obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(widget->obj, 0, 0);
    lv_obj_set_style_pad_all(widget->obj, 0, 0);
    lv_obj_clear_flag(widget->obj, LV_OBJ_FLAG_SCROLLABLE);

    layer_label = lv_label_create(widget->obj);
    lv_obj_align(layer_label, LV_ALIGN_TOP_LEFT, 0, 0);

    wpm_label = lv_label_create(widget->obj);
    lv_obj_set_style_text_font(wpm_label, &lv_font_montserrat_16, 0);
    lv_obj_align(wpm_label, LV_ALIGN_RIGHT_MID, 0, 0);

    output_label = lv_label_create(widget->obj);
    lv_obj_align(output_label, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    info_layer_listener_init();
    info_wpm_listener_init();
    info_output_listener_init();

    return 0;
}

lv_obj_t *zmk_widget_info_obj(struct zmk_widget_info *widget) { return widget->obj; }
