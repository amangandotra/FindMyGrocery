// FindMyGrocery Main Application File

#include "bsp/esp-bsp.h"
#include "esp_log.h"
#include "ui.h"
#include "lvgl.h"
#include "keyboard.h"
#include "wifi.h"
#include "rfid_service.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting FindMyGrocery UI");
    wifi_init_sta();   
    /* Initialize I2C (for touch and audio) */
    bsp_i2c_init();
    rfid_init();

    /* Initialize display and LVGL */
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = BSP_LCD_H_RES * CONFIG_BSP_LCD_DRAW_BUF_HEIGHT,
        .double_buffer = 0,
        .flags = {
            .buff_dma = true,
        }
    };
    bsp_display_start_with_config(&cfg);
    lv_indev_t * indev = lv_indev_get_next(NULL);
    if(indev == NULL) {
        ESP_LOGE(TAG, "❌ No LVGL input device found!");
    } else {
        ESP_LOGI(TAG, "✅ LVGL input device detected");
    }

    /* Set display brightness to 100% */
    bsp_display_backlight_on();
    ui_init();
    ESP_LOGI(TAG, "UI started successfully");
    // keyboard_init();
    // ESP_LOGI(TAG, "Keyboard initialized successfully");

}