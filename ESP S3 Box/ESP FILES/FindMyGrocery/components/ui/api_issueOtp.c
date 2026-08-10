#include "api_issueOtp.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"
#include "issue_state.h"
#include <string.h>
#include "ui.h"
static const char *TAG = "API_OTP";

#define MAX_HTTP_OUTPUT 512

static char http_response_buffer[MAX_HTTP_OUTPUT];
static int  http_response_len = 0;

/* HTTP event handler */
static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id)
    {
        case HTTP_EVENT_ON_DATA:
            if(http_response_len + evt->data_len < MAX_HTTP_OUTPUT)
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

static void otp_request_task(void *arg)
{
    http_response_len = 0;
    memset(http_response_buffer, 0, sizeof(http_response_buffer));

    char post_data[256];
    snprintf(post_data, sizeof(post_data),
        "{\"copy_uid\":\"%s\",\"enrollment_no\":\"%s\"}",
        g_issue_session.copy_uid,
        g_issue_session.enrollment_no);

    char url[256];
    snprintf(url, sizeof(url), "%s/api/issue/request_otp", API_BASE_SERVER);

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
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "HTTP failed");
        goto cleanup;
    }

    http_response_buffer[http_response_len] = 0;

    ESP_LOGI(TAG, "OTP response: %s", http_response_buffer);

    /* JSON parse */
    cJSON *root = cJSON_Parse(http_response_buffer);
    if(!root)
    {
        ESP_LOGE(TAG, "JSON parse failed");
        goto cleanup;
    }

    cJSON *otp = cJSON_GetObjectItem(root, "otp");
    cJSON *email = cJSON_GetObjectItem(root, "email");

    if(cJSON_IsString(otp) && cJSON_IsString(email))
    {
        strncpy(g_issue_session.otp, otp->valuestring, sizeof(g_issue_session.otp)-1);
        strncpy(g_issue_session.email, email->valuestring, sizeof(g_issue_session.email)-1);

        ESP_LOGI(TAG, "Stored OTP: %s", g_issue_session.otp);
    }
    else
    {
        ESP_LOGE(TAG, "Invalid OTP JSON");
    }

    cJSON_Delete(root);

cleanup:
    esp_http_client_cleanup(client);
    vTaskDelete(NULL);
}

void start_issue_otp_request_task(void)
{
    xTaskCreate(otp_request_task, "otp_req", 8192, NULL, 8, NULL);
}
