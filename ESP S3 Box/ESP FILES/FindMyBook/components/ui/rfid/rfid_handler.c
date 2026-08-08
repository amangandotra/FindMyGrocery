#include "rfid_handler.h"
#include "ui.h"
#include "esp_log.h"
#include "lvgl.h"
#include "rfid_state.h"     
#include "rfid_service.h"    
#include "issue_state.h"
static const char *TAG = "ISSUE_RFID";

void on_issue_rfid_scanned(const char *uid)
{
    // Already captured? Ignore
    if(g_issue_rfid_uid[0] != 0) {
        ESP_LOGI(TAG, "RFID already captured, ignoring");
        return;
    }

    strncpy(g_issue_rfid_uid, uid, sizeof(g_issue_rfid_uid)-1);

    ESP_LOGI(TAG, "Captured UID: %s", g_issue_rfid_uid);

    // Update UI
    if(ui_Label19) {
        lv_label_set_text(ui_Label19, uid);
        lv_obj_set_style_text_color(ui_Label19, lv_color_hex(0x00FF00), 0);
    }
    strncpy(g_issue_session.copy_uid, uid, sizeof(g_issue_session.copy_uid)-1);

    // Stop scanning
    rfid_set_mode(RFID_MODE_NONE);

    // Move to enrollment screen
    _ui_screen_change(&ui_screenIssueEnrollment, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, &ui_screenIssueEnrollment_screen_init);
}
