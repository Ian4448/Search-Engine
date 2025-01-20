#ifndef CRAWLER_H
#define CRAWLER_H

#include "main.h"
#include <stdio.h>
#include <curl/curl.h>
#include <tidy/tidy.h>
#include <tidy/buffio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread/pthread.h>

#define TRUE 1
#define NUM_OF_THREADS 8

/* Type for storing the HTTP response */
typedef struct
{
    char *string;
    size_t size;
} Response;

/* Struct for tracking crawler depth */
typedef struct
{
    int current_depth;
    int urls_at_current_depth;
    int urls_at_next_depth;
} CrawlerDepthData;

/* Struct for thread arguments */
typedef struct {
    Set *set;
    Queue *queue;
    TidyDoc tidy_doc;
    CrawlerDepthData *c_depth;
    Response *response;
} ThreadArgs;

/* Global synchronization primitives */
extern pthread_mutex_t queue_mutex;
extern pthread_mutex_t set_mutex;
extern pthread_cond_t queue_cond;

/* Function declarations */
double run_crawler(Set *set, Queue *queue, TidyDoc tidy_doc, CURL *curl);
static size_t write_chunk(void *data, size_t size, size_t nmemb, void *userdata);
static void parse_html_data(Queue *queue, Set *set, const TidyNode tnode, CrawlerDepthData *c_depth);
void *thread_function(void *arg);

#endif // CRAWLER_H
