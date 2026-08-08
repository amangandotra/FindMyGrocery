#ifndef SPEECH_H
#define SPEECH_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize wake‑word and command recognition.
 *        Starts the background speech task.
 */
esp_err_t speech_init(void);

/**
 * @brief Get the current speech recognition state.
 */
bool speech_is_listening(void);

#ifdef __cplusplus
}
#endif

#endif // SPEECH_H