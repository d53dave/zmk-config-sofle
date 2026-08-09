#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

struct zmk_widget_info {
    lv_obj_t *obj;
};

int zmk_widget_info_init(struct zmk_widget_info *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_info_obj(struct zmk_widget_info *widget);
