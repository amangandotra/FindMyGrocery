#ifndef BLINK_TASK_H
#define BLINK_TASK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void start_blink_task(const char *rack, int row, int col, const char *side);

#ifdef __cplusplus
}
#endif

#endif // BLINK_TASK_H
