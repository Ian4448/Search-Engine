#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <curl/curl.h>
#include <tidy/tidy.h>
#include <tidy/buffio.h>
#include <stdlib.h>
#include <string.h>

#include "util/hash_set.h"
#include "util/queue.h"

#define MAXIMUM_DEPTH 2

/* Variables to track search-depth */
int current_depth = 0;
int urls_at_current_depth = 1;
int urls_at_next_depth = 0;

/* Type for storing the HTTP response */
typedef struct
{
    char *string;
    size_t size;
} Response;

/* Function declarations */
size_t write_chunk(void *data, size_t size, size_t nmemb, void *userdata);
void parse_html_data(Queue *queue, Set *set, TidyNode tnode);

#endif // MAIN_H
