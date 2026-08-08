#pragma once
#include <stdbool.h>

typedef struct {
    char book_type_id[16];
    char name[128];        
    char rack[8];
    int  row;
    int  column;
    char side[8];          
} book_location_t;


/**
 * Query the /search?name=... endpoint.
 * Returns true on success and writes into out_location.
 */
bool api_find_book(const char *book_name, book_location_t *out_location);
