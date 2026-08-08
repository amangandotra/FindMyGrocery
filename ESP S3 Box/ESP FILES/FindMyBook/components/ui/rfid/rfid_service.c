#include "rfid_service.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>
#include "lvgl.h"          // for lv_async_call

#include "theft_handler.h" 
#define UART_NUM UART_NUM_1
#define RX_PIN 44
#define TX_PIN 43

static const char *TAG = "RFID";

static rfid_mode_t current_mode = RFID_MODE_NONE;
static rfid_callback_t user_callback = NULL;

/* wrapper that runs in LVGL task via lv_async_call
   `p` is a heap-allocated char* (uid). We call user_callback(uid) then free it. */
static void lvcb_invoke_user(void *p)
{
    char *uid = (char *)p;
    if (uid && user_callback) {
        user_callback(uid);
    }
    free(uid);
}

void rfid_set_mode(rfid_mode_t mode)
{
    current_mode = mode;
    ESP_LOGI(TAG, "Mode set: %d", mode);
}

void rfid_set_callback(rfid_callback_t cb)
{
    user_callback = cb;
    ESP_LOGI(TAG, "Callback %s", cb ? "set" : "cleared");
}

static void rfid_uart_task(void *arg)
{
    uint8_t buf[256];

    while (1) {
        int len = uart_read_bytes(UART_NUM, buf, sizeof(buf)-1, pdMS_TO_TICKS(500));
        if (len > 0) {
            buf[len] = 0;
            if (strncmp((char*)buf, "RFID:", 5) == 0)
            {
                char *p = (char*)buf + 5;

                char *source = strtok(p, ":");
                char *uid = strtok(NULL, "\r\n");

                if (!source || !uid) continue;

                ESP_LOGI(TAG, "RFID %s → %s", source, uid);

                // DOOR = theft detection (always active)
                if (strcmp(source, "DOOR") == 0)
                {
                    handle_theft_uid(uid);   // new function (see below)
                    continue;
                }

                // COUNTER = issue / return only
                if (strcmp(source, "COUNTER") == 0)
                {
                    if (current_mode != RFID_MODE_NONE && user_callback)
                    {
                        char *heap_uid = malloc(strlen(uid) + 1);
                        if (heap_uid)
                        {
                            strcpy(heap_uid, uid);
                            lv_async_call(lvcb_invoke_user, heap_uid);
                        }
                    }
                    else
                    {
                        ESP_LOGI(TAG, "Ignored counter tag: mode=%d", current_mode);
                    }
                }
            }
             else {
                /* may be noisy serial, ignore */
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void rfid_init(void)
{
    uart_config_t cfg = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_driver_install(UART_NUM, 2048, 0, 0, NULL, 0);
    uart_param_config(UART_NUM, &cfg);
    uart_set_pin(UART_NUM, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    xTaskCreate(rfid_uart_task, "rfid_uart", 4096, NULL, 10, NULL);
    ESP_LOGI(TAG, "RFID init done");
}
