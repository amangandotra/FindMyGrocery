#pragma once
#include <stdbool.h>

typedef enum {
    RFID_MODE_NONE,
    RFID_MODE_ISSUE,
    RFID_MODE_RETURN
} rfid_mode_t;

typedef enum {
    RFID_SRC_COUNTER,
    RFID_SRC_DOOR
} rfid_source_t;

typedef void (*rfid_callback_t)(const char *uid);

/* initialize the UART + rfid task (call once on startup) */
void rfid_init(void);

/* set scanning mode (NONE / ISSUE / RETURN) */
void rfid_set_mode(rfid_mode_t mode);

/* set callback to be called when tag scanned (called in LVGL thread) */
void rfid_set_callback(rfid_callback_t cb);
