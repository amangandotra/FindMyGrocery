#include "api_returnBook.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "ui.h"
#include <string.h>
#include "returnHandler.h"
static const char *TAG = "API_RETURN";

static void open_success(void *arg)
{
    _ui_screen_change(&ui_screenReturnSuccess,
                      LV_SCR_LOAD_ANIM_FADE_ON,
                      200, 0,
                      &ui_screenReturnSuccess_screen_init);
}

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    return ESP_OK;
}

static void return_task(void *arg)
{
    const char *uid = (const char *)arg;

    char post[128];
    snprintf(post, sizeof(post), "{\"copy_uid\":\"%s\"}", uid);

    char url[128];
    snprintf(url, sizeof(url), "%s/api/return", API_BASE_SERVER);

    esp_http_client_config_t cfg = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .event_handler = http_event_handler,
        .timeout_ms = 6000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post, strlen(post));

    esp_err_t err = esp_http_client_perform(client);

    if(err == ESP_OK && esp_http_client_get_status_code(client) == 200)
    {
        ESP_LOGI(TAG, "Return successful");
        lv_async_call(open_success, NULL);
    }
    else
    {
        ESP_LOGE(TAG, "Return failed");
        return_show_error("Book not issued / invalid");
    }

    esp_http_client_cleanup(client);
    free(arg);
    vTaskDelete(NULL);
}

void start_return_task(const char *copy_uid)
{
    char *uid = strdup(copy_uid);
    xTaskCreate(return_task, "return_task", 8192, uid, 8, NULL);
}
