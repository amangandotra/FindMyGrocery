#include "api_theft_check.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string.h>
#include <stdlib.h>
#include "ui.h"
#include "driver/uart.h"
#define ARDUINO_UART UART_NUM_1



static const char *TAG = "API_THEFT";
#define MAX_HTTP_OUTPUT 256
static char response_buffer[MAX_HTTP_OUTPUT];
static int response_len = 0;

/* HTTP event handler */

void send_led_command(const char *cmd) {
    if (cmd) {
        uart_write_bytes(ARDUINO_UART, cmd, strlen(cmd));
        uart_write_bytes(ARDUINO_UART, "\n", 1);
    }
}


static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
        case HTTP_EVENT_ON_DATA:
            if (response_len + evt->data_len < MAX_HTTP_OUTPUT)
            {
                memcpy(response_buffer + response_len, evt->data, evt->data_len);
                response_len += evt->data_len;
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}

static void theft_task(void *arg)
{
    char *uid = (char *)arg;

    response_len = 0;
    memset(response_buffer, 0, sizeof(response_buffer));

    char post_data[128];
    snprintf(post_data, sizeof(post_data),
             "{\"copy_uid\":\"%s\"}", uid);

    char url[256];
    snprintf(url, sizeof(url), "%s/api/book/status", API_BASE_SERVER);

    esp_http_client_config_t cfg = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,
        .event_handler = http_event_handler
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "HTTP failed: %s", esp_err_to_name(err));
        goto cleanup;
    }

    response_buffer[response_len] = 0;

    ESP_LOGI(TAG, "Response: %s", response_buffer);

    cJSON *root = cJSON_Parse(response_buffer);
    if (!root)
    {
        ESP_LOGE(TAG, "JSON parse failed");
        goto cleanup;
    }

    cJSON *issued = cJSON_GetObjectItem(root, "issued");

    if (cJSON_IsBool(issued) && issued->valueint == 0)
    {
        ESP_LOGE(TAG, "🚨🚨🚨 THEFT DETECTED for UID: %s", uid);
        send_led_command("LED:THEFT");
        
    }
    else
    {
        send_led_command("LED:ISSUED");
        ESP_LOGI(TAG, "✅ Book is issued – allowed: %s", uid);
    }

    cJSON_Delete(root);

cleanup:
    esp_http_client_cleanup(client);
    free(uid);
    vTaskDelete(NULL);
}

void start_theft_check_task(const char *uid)
{
    char *heap_uid = strdup(uid);
    xTaskCreate(theft_task, "theft_check", 4096, heap_uid, 8, NULL);
}
