#include "theft_handler.h"
#include "esp_log.h"
#include "api_theft_check.h"
static const char *TAG = "THEFT";


void handle_theft_uid(const char *uid)
{
    ESP_LOGW(TAG, "DOOR RFID detected: %s", uid);

    start_theft_check_task(uid);
}
