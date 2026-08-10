#include "wifi.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/event_groups.h"
#include "ui.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "esp_netif.h"
#include "http_server.h"


#define WIFI_SSID "SmartLibrary"
#define WIFI_PASS ""

static EventGroupHandle_t wifi_event_group;
static const int WIFI_CONNECTED_BIT = BIT0;
static const char *TAG = "WIFI";

static void register_ip_task(void *arg)
{
    ESP_LOGI(TAG, "Starting register IP task");

    esp_netif_ip_info_t ip_info;
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");

    if (!netif || esp_netif_get_ip_info(netif, &ip_info) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get local IP");
        vTaskDelete(NULL);
        return;
    }

    char ip_str[16];
    esp_ip4addr_ntoa(&ip_info.ip, ip_str, sizeof(ip_str));

    ESP_LOGI(TAG, "Local IP: %s", ip_str);

    /* Create JSON */
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "device", "espbox");
    cJSON_AddStringToObject(json, "ip", ip_str);
    char *post_data = cJSON_PrintUnformatted(json);

    esp_http_client_config_t config = {
        .url = "http://fmb.local:5000/register_ip",
        .method = HTTP_METHOD_POST,
        .timeout_ms = 2000, // short timeout prevents freeze
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {

        char buf[128] = {0};
        esp_http_client_read_response(client, buf, sizeof(buf)-1);

        ESP_LOGI(TAG, "Register Response: %s", buf);

        /* Parse response JSON */
        cJSON *res = cJSON_Parse(buf);
        if (res) {
            cJSON *ip_field = cJSON_GetObjectItem(res, "rpi_ip");

            if (cJSON_IsString(ip_field)) {
                snprintf(API_BASE_SERVER,
                         sizeof(API_BASE_SERVER),
                         "http://%s:5000/",
                         ip_field->valuestring);

                ESP_LOGI(TAG, "API base set to: %s", API_BASE_SERVER);
            }

            cJSON_Delete(res);
        }
    }
    else {
        /* IMPORTANT: Just log error — NO FREEZE */
        ESP_LOGW(TAG, "⚠️ fmb.local not available or server offline");
        ESP_LOGW(TAG, "HTTP error: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    free(post_data);
    cJSON_Delete(json);

    vTaskDelete(NULL);
}

static void wifi_event_handler(void* arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void* event_data)
{
    if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {

        esp_wifi_connect();

    } else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {

        ESP_LOGI(TAG, "Disconnected, retrying...");
        esp_wifi_connect();

    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {

        ESP_LOGI(TAG, "Connected to WiFi");
        start_http_server();

        /* Start register task (NON BLOCKING) */
        xTaskCreate(register_ip_task,
                    "register_ip_task",
                    8192,
                    NULL,
                    5,
                    NULL);

        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID,
        &wifi_event_handler,
        NULL,
        NULL));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &wifi_event_handler,
        NULL,
        NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi init done");
}

bool wifi_is_connected(void)
{
    EventBits_t bits = xEventGroupGetBits(wifi_event_group);
    return (bits & WIFI_CONNECTED_BIT);
}
