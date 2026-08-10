#include "api_blinkRack.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string.h>
#include "ui.h"
#define TAG "API_BLINK_RACK"
// #define API_URL API_BASE_SERVER "/blink_rack"

typedef struct {
    char rack[8];
} blink_ctx_t;

static void blink_task(void *arg)
{
    blink_ctx_t *ctx = (blink_ctx_t *)arg;

    char post_data[64];
    snprintf(post_data, sizeof(post_data),
             "{\"rack\":\"%s\"}", ctx->rack);
    char API_URL[128];
    snprintf(API_URL, sizeof(API_URL), "%sblink_rack", API_BASE_SERVER);

    esp_http_client_config_t config = {
        .url = API_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 4000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "Status = %d", status);
    } else {
        ESP_LOGE(TAG, "HTTP failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    free(ctx);
    vTaskDelete(NULL);
}

void start_blink_rack_task(const char *rack)
{
    blink_ctx_t *ctx = malloc(sizeof(blink_ctx_t));
    strncpy(ctx->rack, rack, sizeof(ctx->rack) - 1);

    xTaskCreate(blink_task, "blink_rack", 4096, ctx, 5, NULL);
}
