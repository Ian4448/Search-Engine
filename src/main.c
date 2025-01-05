#include "main.h"
#include "time.h"

int main(void)
{
    Set *set = hash_set_init();
    Queue *queue = queue_init();
    CURL *curl = curl_easy_init();

    TidyDoc tidy_doc = tidyCreate();
    TidyBuffer errbuf = {0};
    tidyOptSetBool(tidy_doc, TidyQuiet, yes);
    tidyOptSetBool(tidy_doc, TidyShowWarnings, no);
    tidySetErrorBuffer(tidy_doc, &errbuf);

    queue_enqueue(queue, "https://www.google.com");

    if (curl == NULL)
    {
        fprintf(stderr, "Libcurl Initilization Failed");
        return -1;
    }

    Response response;
    response.string = malloc(1);
    response.size = 0;

    const time_t start = time(NULL);
    while (!queue_empty(queue)) {
        char *url = queue_dequeue(queue);

        if (current_depth > MAXIMUM_DEPTH) {
            free(url);
            break;
        }

        if (hash_set_contains(set, url)) {
            free(url);
            continue;
        }

        hash_set_add(set, url);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_chunk);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) &response);

        const CURLcode result = curl_easy_perform(curl);
        if (result == CURLE_OK) {
            tidyParseString(tidy_doc, response.string);
            parse_html_data(queue, set, tidyGetRoot(tidy_doc));
        }

        free(response.string);
        response.string = malloc(1);
        response.size = 0;

        urls_at_current_depth--;

        if (urls_at_current_depth == 0) {
            current_depth++;
            urls_at_current_depth = urls_at_next_depth;
            urls_at_next_depth = 0;
        }
    }
    const time_t end = time(NULL);
    const double total_search_time = difftime(end, start);

    //printf("%s\n", response.string);
    hash_set_display(set);
    printf("Total Run-Time Taken For Searching %.2f seconds", total_search_time);

    curl_easy_cleanup(curl);
    tidyBufFree(&errbuf);
    tidyRelease(tidy_doc);
    free(response.string);
    hash_set_free(set);
    queue_free(queue);

    return 0;
}

size_t write_chunk(void *data, size_t size, size_t nmemb, void *userdata)
{
    const size_t real_size = size * nmemb;

    Response *response = (Response *) userdata;

    char *ptr = realloc(response->string, response->size+real_size+1);

    if (ptr == NULL)
    {
        fprintf("Write Data Chunk Failed", stderr);
        return CURL_WRITEFUNC_ERROR;
    }

    response->string = ptr;
    memcpy(&(response->string[response->size]), data, real_size);
    response->size += real_size;
    response->string[response->size] = '\0';

    return real_size;
}

void parse_html_data(Queue *queue, Set *set, const TidyNode tnode)
{
    if (current_depth >= MAXIMUM_DEPTH) {
        return;
    }

    for (TidyNode child = tidyGetChild(tnode); child; child = tidyGetNext(child)) {
        const TidyTagId tag_id = tidyNodeGetId(child);

        if (tag_id == TidyTag_A) {
            const TidyAttr href_attr = tidyAttrGetById(child, TidyAttr_HREF);
            if (href_attr) {
                const char *url = tidyAttrValue(href_attr);
                if (url && strstr(url, "http") != NULL && !hash_set_contains(set, url)) {
                    //printf("Found URL: %s\n", url);
                    char *url_copy = malloc(strlen(url) + 1);
                    strcpy(url_copy, url);
                    queue_enqueue(queue, url_copy);
                    urls_at_next_depth++;
                }
            }
        }

        parse_html_data(queue, set, child);
    }
}
