#include "api_findbook.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include <string.h>
#include <stdlib.h>
#include "ui.h"

static const char *TAG = "api_findbook";

bool api_find_book(const char *book_name, book_location_t *out_location)
{
    if (!book_name || !out_location) return false;

    char url[256];
    // URL-encode book_name minimally: spaces -> %20 (for simple names). For robust, use proper urlencode.
    // Simple encode (replace spaces)
    char qname[128];
    size_t j = 0;
    for (size_t i = 0; i < strlen(book_name) && j + 4 < sizeof(qname); ++i) {
        char c = book_name[i];
        if (c == ' ') {
            qname[j++] = '%'; qname[j++] = '2'; qname[j++] = '0';
        } else {
            qname[j++] = c;
        }
    }
    qname[j] = '\0';

    snprintf(url, sizeof(url), "%s/search?name=%s", API_BASE_SERVER, qname);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 5000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to init http client");
        return false;
    }

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return false;
    }

    /* Fetch headers (IMPORTANT for chunked responses) */
    int status = esp_http_client_fetch_headers(client);
    ESP_LOGI(TAG, "HTTP status = %d", esp_http_client_get_status_code(client));

    char buffer[2048];
    int total_read = 0;
    int read_len;

    memset(buffer, 0, sizeof(buffer));

    /* Read body */
    while ((read_len = esp_http_client_read(
                client,
                buffer + total_read,
                sizeof(buffer) - total_read - 1)) > 0)
    {
        total_read += read_len;
    }

    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    if (total_read <= 0) {
        ESP_LOGE(TAG, "No response body (total_read=%d)", total_read);
        return false;
    }

    buffer[total_read] = '\0';
    ESP_LOGI(TAG, "Response: %s", buffer);

    /* Parse JSON */
    cJSON *root = cJSON_Parse(buffer);
    if (!root) {
        ESP_LOGE(TAG, "JSON parse error");
        return false;
    }


    /* ---- Parse required fields ---- */
    cJSON *rack    = cJSON_GetObjectItemCaseSensitive(root, "rack");
    cJSON *row     = cJSON_GetObjectItemCaseSensitive(root, "row");
    cJSON *column  = cJSON_GetObjectItemCaseSensitive(root, "column");
    cJSON *type_id = cJSON_GetObjectItemCaseSensitive(root, "book_type_id");
    cJSON *name = cJSON_GetObjectItemCaseSensitive(root, "name");
    cJSON *side = cJSON_GetObjectItemCaseSensitive(root, "side");


    /* ---- Validate mandatory fields ---- */
    if (!cJSON_IsString(rack) ||
        !cJSON_IsNumber(row) ||
        !cJSON_IsNumber(column) ||
        !cJSON_IsString(type_id) ||
        !cJSON_IsString(name) ||
        !cJSON_IsString(side))
    {
        ESP_LOGE(TAG, "Invalid or missing JSON fields");
        cJSON_Delete(root);
        return false;
    }

    /* ---- Copy values safely ---- */
    memset(out_location, 0, sizeof(book_location_t));

    strncpy(out_location->rack,
            rack->valuestring,
            sizeof(out_location->rack) - 1);

    strncpy(out_location->book_type_id,
            type_id->valuestring,
            sizeof(out_location->book_type_id) - 1);
    strncpy(out_location->name,
            name->valuestring,
            sizeof(out_location->name) - 1);
    strncpy(out_location->side,
        side->valuestring,
        sizeof(out_location->side) - 1);

    out_location->row    = row->valueint;
    out_location->column = column->valueint;

    cJSON_Delete(root);
    return true;
}
