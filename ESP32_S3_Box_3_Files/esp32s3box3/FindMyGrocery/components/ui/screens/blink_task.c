// blink_task.c
#include "blink_task.h"

#include "api_blinkbook.h"  
#include "../ui.h"
#include "lvgl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BLINK_UI";

/* tiny structs used for passing data between tasks / LVGL callback */
typedef struct {
    char rack[8];
    int row;
    int col;
    char side[8];
} blink_req_t;

/* LVGL callback receives blink_color_t pointer (allocated on heap by blink_task). */
static void blink_ui_cb(void *p);

/* create or return the overlay object used to display blink color */
static lv_obj_t *ensure_blink_overlay(void)
{
    static lv_obj_t *overlay = NULL;
    if (!overlay) {
        overlay = lv_obj_create(lv_scr_act());
        lv_obj_remove_style_all(overlay);
        lv_obj_set_size(overlay, lv_pct(100), lv_pct(100));
        lv_obj_align(overlay, LV_ALIGN_CENTER, 0, 0);
        lv_obj_clear_flag(overlay, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        // default invisible
        lv_obj_set_style_bg_opa(overlay, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(overlay, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);
        // put behind UI interactions but above other things as needed
    }
    return overlay;
}

/* animation exec: set background opacity */
static void _anim_set_opa(void *target, int32_t v)
{
    if (!target) return;
    lv_obj_t *obj = (lv_obj_t *)target;
    lv_obj_set_style_bg_opa(obj, (lv_opa_t)v, LV_PART_MAIN | LV_STATE_DEFAULT);
}

/* fade-out timer callback (starts fade out animation) */
static void _start_fade_out_cb(lv_timer_t *timer)
{
    lv_obj_t *overlay = (lv_obj_t *)timer->user_data;
    if (!overlay) {
        lv_timer_del(timer);
        return;
    }
    // Fade out  -> from current (255 or hold_opa) to 0
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, overlay);
    lv_anim_set_exec_cb(&a, _anim_set_opa);
    lv_anim_set_values(&a, 255, 0);
    lv_anim_set_time(&a, 400); // fade out in 400ms
    lv_anim_start(&a);

    lv_timer_del(timer);
}

/* The LVGL async callback: shows color and runs animation (fade-in, hold, fade-out) */
static void blink_ui_cb(void *p)
{
    if (!p) return;
    blink_color_t *color = (blink_color_t *)p;
    lv_obj_t *overlay = ensure_blink_overlay();

    // Set color
    lv_color_t bg = lv_color_make((uint8_t)color->r, (uint8_t)color->g, (uint8_t)color->b);
    lv_obj_set_style_bg_color(overlay, bg, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Force overlay to top
    lv_obj_move_foreground(overlay);

    // Fade in quickly to visible (0 -> 255)
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, overlay);
    lv_anim_set_exec_cb(&a, _anim_set_opa);
    lv_anim_set_values(&a, 0, 255);
    lv_anim_set_time(&a, 300);
    lv_anim_start(&a);

    // after hold_ms, start fade-out via a timer
    const uint32_t hold_ms = 1500; // how long the overlay stays fully visible (adjust as desired)

    // create one-shot timer that will trigger fade out
    lv_timer_t *t = lv_timer_create(_start_fade_out_cb, hold_ms, overlay);
    // no need to keep pointer; timer deletes itself in _start_fade_out_cb

    // free the heap color struct
    free(color);
}

/* FreeRTOS task: calls API (blocking) and if successful sends LVGL async call to update UI */
static void blink_task(void *arg)
{
    blink_req_t *req = (blink_req_t *)arg;
    if (!req) {
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "blink_task: rack=%s row=%d col=%d side=%s",
             req->rack, req->row, req->col, req->side);

    blink_color_t out_color;
    memset(&out_color, 0, sizeof(out_color));

    bool ok = api_blink_book(req->rack, req->row, req->col, req->side, &out_color);
    if (ok) {
        ESP_LOGI(TAG, "api_blink_book returned color: R:%d G:%d B:%d",
                 out_color.r, out_color.g, out_color.b);

        // pass color to LVGL via heap struct (lv_async_call requires pointer lifetime)
        blink_color_t *heap_color = malloc(sizeof(blink_color_t));
        if (heap_color) {
            *heap_color = out_color;
            // schedule LVGL UI update safely
            lv_async_call(blink_ui_cb, heap_color);
        } else {
            ESP_LOGW(TAG, "malloc failed for heap_color");
        }
    } else {
        ESP_LOGE(TAG, "api_blink_book failed");
    }

    // cleanup
    free(req);
    vTaskDelete(NULL);
}

/* public starter - call this from LVGL event handler rather than calling api directly */
void start_blink_task(const char *rack, int row, int col, const char *side)
{
    if (!rack || !side) return;

    blink_req_t *req = calloc(1, sizeof(blink_req_t));
    if (!req) {
        ESP_LOGE(TAG, "start_blink_task: malloc failed");
        return;
    }

    // sanitize small copies
    snprintf(req->rack, sizeof(req->rack), "%s", rack);
    req->row = row;
    req->col = col;
    snprintf(req->side, sizeof(req->side), "%s", side);

    BaseType_t rc = xTaskCreate(
        blink_task,
        "blink_task",
        4096,
        req,
        tskIDLE_PRIORITY + 5,
        NULL
    );

    if (rc != pdPASS) {
        ESP_LOGE(TAG, "start_blink_task: xTaskCreate failed");
        free(req);
    }
}
