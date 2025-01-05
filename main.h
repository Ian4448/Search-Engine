#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <curl/curl.h>
#include <tidy/tidy.h>
#include <tidy/buffio.h>
#include <stdlib.h>
#include <string.h>

#include "hash_set.h"

/* Type for storing the HTTP response */
typedef struct
{
    char *string;
    size_t size;
} Response;

/* Function declarations */
size_t write_chunk(void *data, size_t size, size_t nmemb, void *userdata);
void parse_html_data(Set *set, TidyNode tnode);

#endif // MAIN_H
