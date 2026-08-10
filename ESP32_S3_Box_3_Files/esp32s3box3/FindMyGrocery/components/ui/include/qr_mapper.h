#pragma once
#include "lvgl.h"

const lv_img_dsc_t * get_qr_image_for_location(
    const char *rack,
    int row,
    int column,
    const char *side
);
