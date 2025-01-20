#include "web_crawler.h"

#include <stdbool.h>

#include "pthread/pthread.h"

pthread_mutex_t queue_mutex;
pthread_mutex_t set_mutex;
pthread_cond_t queue_cond;

void *thread_function(void *arg)
{
    ThreadArgs *args = (ThreadArgs *)arg;
    CURL *curl = curl_easy_init();
    if (curl == NULL) {
        fprintf(stderr, "Libcurl Initialization Failed");
        pthread_exit(NULL);
    }

    while (true) {
        pthread_mutex_lock(&queue_mutex);

        if (queue_empty(args->queue)) {
            pthread_mutex_unlock(&queue_mutex);
            break;
        }

        char *url = queue_dequeue(args->queue);
        pthread_mutex_unlock(&queue_mutex);

        if (args->c_depth->current_depth > MAXIMUM_DEPTH) {
            free(url);
            break;
        }

        pthread_mutex_lock(&set_mutex);
        if (hash_set_contains(args->set, url)) {
            pthread_mutex_unlock(&set_mutex);
            free(url);
            continue;
        }

        hash_set_add(args->set, url);
        pthread_mutex_unlock(&set_mutex);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_chunk);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) args->response);

        const CURLcode result = curl_easy_perform(curl);
        if (result == CURLE_OK) {
            tidyParseString(args->tidy_doc, args->response->string);

            pthread_mutex_lock(&queue_mutex);
            pthread_mutex_lock(&set_mutex);

            parse_html_data(args->queue, args->set, tidyGetRoot(args->tidy_doc), args->c_depth);

            pthread_mutex_unlock(&set_mutex);
            pthread_mutex_unlock(&queue_mutex);
        }

        free(args->response->string);
        args->response->string = malloc(1);
        args->response->size = 0;

        pthread_mutex_lock(&queue_mutex);
        args->c_depth->urls_at_current_depth--;

        if (args->c_depth->urls_at_current_depth == 0) {
            args->c_depth->current_depth++;
            args->c_depth->urls_at_current_depth = args->c_depth->urls_at_next_depth;
            args->c_depth->urls_at_next_depth = 0;
        }
        pthread_mutex_unlock(&queue_mutex);
    }
    curl_easy_cleanup(curl);
    return NULL;
}

double run_crawler(Set *set, Queue *queue, TidyDoc tidy_doc, CURL *curl)
{
    CrawlerDepthData c_depth = {0, 1, 0};
    queue_enqueue(queue, "https://www.google.com");

    pthread_cond_signal(&queue_cond);

    const time_t start = time(NULL);

    pthread_t threads[NUM_OF_THREADS];
    ThreadArgs thread_args[NUM_OF_THREADS];

    for (int i = 0; i < NUM_OF_THREADS; i++) {
        thread_args[i].set = set;
        thread_args[i].queue = queue;
        thread_args[i].tidy_doc = tidy_doc;
        thread_args[i].c_depth = &c_depth;

        thread_args[i].response = malloc(sizeof(Response));
        thread_args[i].response->string = malloc(1);
        thread_args[i].response->size = 0;

        pthread_create(&threads[i], NULL, thread_function, (void *)&thread_args[i]);
        printf("created thread %d", i);
    }

    for (int i = 0; i < NUM_OF_THREADS; i++) {
        pthread_join(threads[i], NULL);
        printf("joined thread %d", i);
    }

    // free thread data

    const time_t end = time(NULL);
    const double total_search_time = difftime(end, start);

    return total_search_time;
}

static size_t write_chunk(void *data, size_t size, size_t nmemb, void *userdata)
{
    const size_t real_size = size * nmemb;

    Response *response = (Response *) userdata;

    char *ptr = realloc(response->string, response->size + real_size + 1);

    if (ptr == NULL) {
        fprintf(stderr, "Write Data Chunk Failed");
        return CURL_WRITEFUNC_ERROR;
    }

    response->string = ptr;
    memcpy(&(response->string[response->size]), data, real_size);
    response->size += real_size;
    response->string[response->size] = '\0';

    return real_size;
}

static void parse_html_data(Queue *queue, Set *set, TidyNode tnode, CrawlerDepthData *c_depth)
{
    if (c_depth->current_depth >= MAXIMUM_DEPTH) {
        return;
    }

    for (TidyNode child = tidyGetChild(tnode); child; child = tidyGetNext(child)) {
        const TidyTagId tag_id = tidyNodeGetId(child);

        if (tag_id == TidyTag_A) {
            const TidyAttr href_attr = tidyAttrGetById(child, TidyAttr_HREF);
            if (href_attr) {
                const char *url = tidyAttrValue(href_attr);
                if (url && strstr(url, "http") != NULL && !hash_set_contains(set, url)) {
                    char *url_copy = malloc(strlen(url) + 1);
                    strcpy(url_copy, url);
                    queue_enqueue(queue, url_copy);
                    c_depth->urls_at_next_depth++;
                }
            }
        }

        parse_html_data(queue, set, child, c_depth);
    }
}
