#include "api_issueBook.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"
#include "issue_state.h"
#include "ui.h"
#include <string.h>

static const char *TAG = "API_ISSUE";

#define MAX_HTTP_OUTPUT 1024

static char http_response_buffer[MAX_HTTP_OUTPUT];
static int  http_response_len = 0;

/* ---------- UI jump ---------- */

static void open_final_confirm_screen(void *arg)
{
    _ui_screen_change(&ui_screenIssueFinalConfirm,
                      LV_SCR_LOAD_ANIM_FADE_ON,
                      200,
                      0,
                      &ui_screenIssueFinalConfirm_screen_init);
}

/* ---------- HTTP event handler ---------- */

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
        case HTTP_EVENT_ON_DATA:
            if (http_response_len + evt->data_len < MAX_HTTP_OUTPUT)
            {
                memcpy(http_response_buffer + http_response_len,
                       evt->data,
                       evt->data_len);
                http_response_len += evt->data_len;
            }
            break;

        default:
            break;
    }
    return ESP_OK;
}

/* ---------- Task ---------- */

static void issue_fetch_task(void *arg)
{
    http_response_len = 0;
    memset(http_response_buffer, 0, sizeof(http_response_buffer));

    char post_data[256];
    snprintf(post_data, sizeof(post_data),
             "{\"copy_uid\":\"%s\",\"enrollment_no\":\"%s\"}",
             g_issue_session.copy_uid,
             g_issue_session.enrollment_no);

    ESP_LOGI(TAG, "POST: %s", post_data);

    char url[256];
    snprintf(url, sizeof(url), "%s/api/issue/preview", API_BASE_SERVER);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .event_handler = http_event_handler,
        .timeout_ms = 8000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
        goto cleanup;
    }

    int status = esp_http_client_get_status_code(client);
    if (status != 200)
    {
        ESP_LOGE(TAG, "Server returned status %d", status);
        goto cleanup;
    }

    http_response_buffer[http_response_len] = 0;

    ESP_LOGI(TAG, "Response: %s", http_response_buffer);

    /* ---------- JSON parse ---------- */

    cJSON *root = cJSON_Parse(http_response_buffer);
    if (!root)
    {
        ESP_LOGE(TAG, "JSON parse failed");
        goto cleanup;
    }

    cJSON *book_name = cJSON_GetObjectItem(root, "book_name");
    cJSON *book_code = cJSON_GetObjectItem(root, "book_code");
    cJSON *user_name = cJSON_GetObjectItem(root, "user_name");
    cJSON *due_date  = cJSON_GetObjectItem(root, "due_date");
    cJSON *email     = cJSON_GetObjectItem(root, "email");

    if (!cJSON_IsString(book_name) ||
        !cJSON_IsString(book_code) ||
        !cJSON_IsString(user_name) ||
        !cJSON_IsString(email) ||
        !cJSON_IsString(due_date))
    {
        ESP_LOGE(TAG, "Invalid JSON fields");
        cJSON_Delete(root);
        goto cleanup;
    }

    strncpy(g_issue_session.book_name, book_name->valuestring, sizeof(g_issue_session.book_name)-1);
    strncpy(g_issue_session.book_code, book_code->valuestring, sizeof(g_issue_session.book_code)-1);
    strncpy(g_issue_session.user_name, user_name->valuestring, sizeof(g_issue_session.user_name)-1);
    strncpy(g_issue_session.email, email->valuestring, sizeof(g_issue_session.email)-1);
    strncpy(g_issue_session.due_date,  due_date->valuestring,  sizeof(g_issue_session.due_date)-1);

    cJSON_Delete(root);

    ESP_LOGI(TAG, "Preview data stored successfully");

    lv_async_call(open_final_confirm_screen, NULL);

cleanup:
    esp_http_client_cleanup(client);
    vTaskDelete(NULL);
}

/* ---------- Public API ---------- */

void start_issue_fetch_task(void)
{
    xTaskCreate(issue_fetch_task, "issue_fetch", 8192, NULL, 8, NULL);
}
