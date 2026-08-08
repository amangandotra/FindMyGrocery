#include "returnHandler.h"

#include "lvgl.h"
#include "ui.h"
#include "esp_log.h"
#include "api_returnBook.h"
static const char *TAG = "RETURN_HANDLER";

/* UI label from screen file */
extern lv_obj_t *ui_Label42;

/* Timer handle */
static lv_timer_t *reset_timer = NULL;

/* Timer callback */
static void reset_label_cb(lv_timer_t *t)
{
    if(ui_Label42)
    {
        lv_label_set_text(ui_Label42, "Waiting for RFID....");
        lv_obj_set_style_text_color(ui_Label42, lv_color_hex(0x125400), LV_PART_MAIN);
    }

    reset_timer = NULL;
}

/* Public: show temporary error */
void return_show_error(const char *msg)
{
    if(!ui_Label42) return;

    lv_label_set_text(ui_Label42, msg);
    lv_obj_set_style_text_color(ui_Label42, lv_color_hex(0xB91C1C), LV_PART_MAIN);

    if(reset_timer)
        lv_timer_del(reset_timer);

    reset_timer = lv_timer_create(reset_label_cb, 3000, NULL);
}

void return_rfid_callback(const char *uid)
{
    ESP_LOGI(TAG, "Return RFID scanned: %s", uid);

    start_return_task(uid);
}
