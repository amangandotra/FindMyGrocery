#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "api_findbook.h"
#include "ui.h"
#include "lvgl.h"

static const char *TAG = "FINDTASK";
static void go_to_findbook_info_async(void *arg)
{
    lv_label_set_text(ui_labelBDBookName, g_findbook_name);

    lv_label_set_text(ui_labelBDBookCode, g_findbook_type_id);

    char loc[64];
    snprintf(loc, sizeof(loc),
            "Rack %s  %s Side Row %d  Col %d",
             g_findbook_rack,
             g_findbook_side,
             g_findbook_row,
             g_findbook_column);

    lv_label_set_text(ui_labelBDLocation, loc);

    _ui_screen_change(
        &ui_screenFindBookInfo,
        LV_SCR_LOAD_ANIM_FADE_ON,
        300,
        0,
        &ui_screenFindBookInfo_screen_init
    );
}

static void findbook_task(void *param)
{
    char *name = (char *)param;
    book_location_t loc;

    ESP_LOGI(TAG, "Calling API for: %s", name);

    if (api_find_book(name, &loc)) {
        lv_obj_set_style_opa(ui_ErrorFBEBN, 0, LV_PART_MAIN);

        ESP_LOGI(TAG, "Book found");
        ESP_LOGI(TAG, "Type ID: %s", loc.book_type_id);
        ESP_LOGI(TAG, "Name: %s", loc.name);
        ESP_LOGI(TAG, "Rack: %s", loc.rack);
        ESP_LOGI(TAG, "Row: %d", loc.row);
        ESP_LOGI(TAG, "Column: %d", loc.column);
        ESP_LOGI(TAG, "Side: %s", loc.side);
        strcpy(g_findbook_rack, loc.rack);
        g_findbook_row = loc.row;
        g_findbook_column = loc.column;
        strcpy(g_findbook_name, loc.name);
        strcpy(g_findbook_type_id, loc.book_type_id);
        strcpy(g_findbook_side, loc.side);
        lv_async_call(go_to_findbook_info_async, NULL);
    } else {
        ESP_LOGE(TAG, "Book not found");
        lv_label_set_text(ui_ErrorFBEBN, "Book not found");
        lv_obj_set_style_opa(ui_ErrorFBEBN, 255, LV_PART_MAIN);

    }

    free(name);
    vTaskDelete(NULL);
}

void start_findbook_task(const char *book_name)
{
    char *copy = malloc(128);
    strcpy(copy, book_name);

    xTaskCreate(
        findbook_task,
        "findbook_task",
        8192,
        copy,
        5,
        NULL
    );
}
