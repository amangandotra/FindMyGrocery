#include "api_blinkbook.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string.h>
#include <stdlib.h>
#include "ui.h"

static const char *TAG = "api_blink";

bool api_blink_book(const char *rack, int row, int col, const char *side, blink_color_t *out_color)
{
    char url[128];
    snprintf(url, sizeof(url), "%s/blink", API_BASE_SERVER);

    char body[128];
    snprintf(body, sizeof(body),
        "{\"rack\":\"%s\",\"row\":%d,\"column\":%d,\"side\":\"%s\"}",
        rack, row, col, side);

    esp_http_client_config_t cfg = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 8000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) return false;

    esp_http_client_set_header(client, "Content-Type", "application/json");

    if (esp_http_client_open(client, strlen(body)) != ESP_OK) {
        ESP_LOGE(TAG, "HTTP open failed");
        esp_http_client_cleanup(client);
        return false;
    }

    esp_http_client_write(client, body, strlen(body));

    esp_http_client_fetch_headers(client);

    char resp[512];
    int total = 0;
    int read_len;

    memset(resp, 0, sizeof(resp));
    ESP_LOGI(TAG, "Blink response: %s", resp);

    while ((read_len = esp_http_client_read(client,
                                            resp + total,
                                            sizeof(resp) - total - 1)) > 0)
    {
        total += read_len;
    }

    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    if (total <= 0) {
        ESP_LOGE(TAG, "No response body");
        return false;
    }

    resp[total] = 0;
    ESP_LOGI(TAG, "Blink response: %s", resp);

    cJSON *root = cJSON_Parse(resp);
    if (!root) {
        ESP_LOGE(TAG, "JSON parse failed");
        return false;
    }

    cJSON *color = cJSON_GetObjectItem(root, "color");
    if (!color) {
        ESP_LOGE(TAG, "No color field");
        cJSON_Delete(root);
        return false;
    }

    out_color->r = cJSON_GetObjectItem(color, "r")->valueint;
    out_color->g = cJSON_GetObjectItem(color, "g")->valueint;
    out_color->b = cJSON_GetObjectItem(color, "b")->valueint;

    cJSON_Delete(root);
    return true;
}
