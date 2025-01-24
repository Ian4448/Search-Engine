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
#include <stdbool.h>
#include "pthread/pthread.h"
#include "sqlite3.h"

#define TRUE 1
#define NUM_OF_THREADS 8
#define MAXIMUM_DEPTH 1

/* Type for storing the HTTP response */
typedef struct
{
    char *string;
    size_t size;
} Response;

/* Type for storing SQLite DB*/
typedef struct
{
    sqlite3 *db;
    char *err_msg;
} Database;

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
    CrawlerDepthData *c_depth;
    Response *response;
    Database *database;
} ThreadArgs;

/* Global synchronization primitives */
pthread_mutex_t queue_mutex;
pthread_mutex_t set_mutex;
pthread_cond_t queue_cond;

/* Function declarations */
double run_crawler(Set *set, Queue *queue);
static size_t write_chunk(void *data, size_t size, size_t nmemb, void *userdata);
static void parse_html_data(Queue *queue, Set *set, TidyNode tnode, CrawlerDepthData *c_depth, Database *db, TidyDoc doc, const char *current_url);
void *thread_function(void *arg);
int store_page(Database *db, const char *url, const char *title, const char *content);
int db_init(Database *database);


#endif // CRAWLER_H
