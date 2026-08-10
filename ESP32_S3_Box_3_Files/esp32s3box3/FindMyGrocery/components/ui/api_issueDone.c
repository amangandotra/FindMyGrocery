#include "api_issueDone.h"
#include "esp_http_client.h"
#include "issue_state.h"
#include "esp_log.h"
#include <string.h>
#include "ui.h"
static const char *TAG = "API_COMMIT";

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    return ESP_OK;
}

static void commit_task(void *arg)
{
    char post_data[256];
    snprintf(post_data, sizeof(post_data),
        "{\"copy_uid\":\"%s\",\"enrollment_no\":\"%s\"}",
        g_issue_session.copy_uid,
        g_issue_session.enrollment_no);

    char url[256];
    snprintf(url, sizeof(url), "%s/api/issue/commit", API_BASE_SERVER);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .event_handler = http_event_handler,
        .timeout_ms = 8000
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);
    if(err == ESP_OK)
    {
        ESP_LOGI(TAG, "Issue committed successfully");
    }
    else
    {
        ESP_LOGE(TAG, "Commit failed");
    }

    esp_http_client_cleanup(client);
    vTaskDelete(NULL);
}

void start_issue_commit_task(void)
{
    xTaskCreate(commit_task, "issue_commit", 8192, NULL, 8, NULL);
}
