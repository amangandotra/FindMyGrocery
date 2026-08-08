#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Callback called when an RFID UID is scanned
 * while Return Book screen is active.
 */
void return_rfid_callback(const char *uid);
void return_show_error(const char *msg);

#ifdef __cplusplus
}
#endif
