#pragma once
#include <stdbool.h>

typedef struct {
    int r;
    int g;
    int b;
} blink_color_t;

bool api_blink_book(const char *rack, int row, int col, const char *side, blink_color_t *out_color);
