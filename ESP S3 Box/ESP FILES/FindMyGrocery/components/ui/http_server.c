#include "http_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "ui.h"
#include "findbook_task.h"
#include "ui_screenVoiceOverlay.h"
static const char *TAG = "HTTP_SERVER";

static httpd_handle_t server = NULL;

/* ================= ISSUE ================= */

static esp_err_t open_issue_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG,"Open ISSUE screen");

    _ui_screen_change(
        &ui_screenIssueScan,
        LV_SCR_LOAD_ANIM_FADE_IN,
        350,
        50,
        &ui_screenIssueScan_screen_init
    );


    httpd_resp_sendstr(req,"OK");
    return ESP_OK;
}

/* ================= RETURN ================= */

static esp_err_t open_return_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG,"Open RETURN screen");

    _ui_screen_change(
        &ui_screenReturnScan,
        LV_SCR_LOAD_ANIM_FADE_IN,
        350,
        50,
        &ui_screenReturnScan_screen_init
    );


    httpd_resp_sendstr(req,"OK");
    return ESP_OK;
}

/* ================= FIND BOOK ================= */

static esp_err_t find_book_handler(httpd_req_t *req)
{
    char query[64];
    char name[32];

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {

        if (httpd_query_key_value(query, "name", name, sizeof(name)) == ESP_OK) {

            ESP_LOGI(TAG,"Find Book: %s", name);

            // Change screen first
            _ui_screen_change(
                &ui_screenFindBook,
                LV_SCR_LOAD_ANIM_FADE_IN,
                350,
                50,
                &ui_screenFindBook_screen_init
            );

            const char *book_name = name;
            start_findbook_task(book_name);
        }
    }

    httpd_resp_sendstr(req,"OK");
    return ESP_OK;
}

static esp_err_t voice_start_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG,"Voice START");

    voice_overlay_show("Listening from your device...");

    httpd_resp_sendstr(req,"OK");
    return ESP_OK;
}
static esp_err_t voice_error_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG,"Voice ERROR");

    voice_overlay_show("Please try again");

    httpd_resp_sendstr(req,"OK");
    return ESP_OK;
}
static esp_err_t voice_close_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG,"Voice CLOSE");

    // Return to home screen
    _ui_screen_change(
        &ui_screenHome,
        LV_SCR_LOAD_ANIM_FADE_IN,
        250,
        0,
        &ui_screenHome_screen_init
    );

    httpd_resp_sendstr(req,"OK");
    return ESP_OK;
}

/* ================= START SERVER ================= */

void start_http_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    ESP_LOGI(TAG,"Starting HTTP Server");

    if (httpd_start(&server, &config) == ESP_OK) {

        httpd_uri_t issue_uri = {
            .uri = "/open_issue",
            .method = HTTP_GET,
            .handler = open_issue_handler
        };

        httpd_uri_t return_uri = {
            .uri = "/open_return",
            .method = HTTP_GET,
            .handler = open_return_handler
        };

        httpd_uri_t find_uri = {
            .uri = "/find_book",
            .method = HTTP_GET,
            .handler = find_book_handler
        };
        httpd_uri_t voice_start_uri = {
            .uri = "/voice_start",
            .method = HTTP_GET,
            .handler = voice_start_handler
        };

        httpd_uri_t voice_error_uri = {
            .uri = "/voice_error",
            .method = HTTP_GET,
            .handler = voice_error_handler
        };

        httpd_uri_t voice_close_uri = {
            .uri = "/voice_close",
            .method = HTTP_GET,
            .handler = voice_close_handler
        };

        httpd_register_uri_handler(server, &voice_start_uri);
        httpd_register_uri_handler(server, &voice_error_uri);
        httpd_register_uri_handler(server, &voice_close_uri);

        httpd_register_uri_handler(server, &issue_uri);
        httpd_register_uri_handler(server, &return_uri);
        httpd_register_uri_handler(server, &find_uri);

        ESP_LOGI(TAG,"HTTP Server Started");
    }
}
