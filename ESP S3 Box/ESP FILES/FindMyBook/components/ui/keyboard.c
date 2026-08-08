// keyboard.c
#include "keyboard.h"
#include "esp_log.h"

static const char *TAG = "KEYBOARD";
lv_obj_t * ui_keyboard = NULL;

void keyboard_create(lv_obj_t * parent)
{
    // If a keyboard exists, remove it first (ensures correct parent)
    if(ui_keyboard) {
        ESP_LOGI(TAG, "Keyboard exists - deleting and recreating");
        lv_obj_del(ui_keyboard);
        ui_keyboard = NULL;
    }

    // Use provided parent if non-NULL, otherwise use the currently active screen
    lv_obj_t *par = parent ? parent : lv_scr_act();

    ESP_LOGI(TAG, "Creating keyboard (parent = %p)", par);

    ui_keyboard = lv_keyboard_create(par);
    lv_obj_set_size(ui_keyboard, 320, 120);
    lv_obj_align(ui_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    // Start hidden until a textarea asks for it
    lv_obj_add_flag(ui_keyboard, LV_OBJ_FLAG_HIDDEN);
}

void keyboard_hide(void)
{
    if(ui_keyboard) {
        ESP_LOGI(TAG, "Hiding & deleting keyboard");
        lv_obj_del(ui_keyboard);
        ui_keyboard = NULL;
    }
}
