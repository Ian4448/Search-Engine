#include "main.h"
#include "hash_set.h"

int main(void)
{
    Set *set = hash_set_init();
    const TidyDoc tidy_doc = tidyCreate();
    CURL *curl = curl_easy_init();
    TidyBuffer errbuf = {0};

    tidyOptSetBool(tidy_doc, TidyQuiet, yes);
    tidyOptSetBool(tidy_doc, TidyShowWarnings, no);
    tidySetErrorBuffer(tidy_doc, &errbuf);

    if (curl == NULL)
    {
        fprintf(stderr, "HTTP Request Failed");
        return -1;
    }

    Response response;
    response.string = malloc(1);
    response.size = 0;

    curl_easy_setopt(curl, CURLOPT_URL, "https://www.google.com");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_chunk);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) &response);

    const CURLcode result = curl_easy_perform(curl);

    if (result != CURLE_OK)
    {
        fprintf(stderr, "Error:", curl_easy_strerror(result));
        return -1;
    }

    int rc = tidyParseString(tidy_doc, response.string);
    if (rc >= 0) {
        rc = tidyCleanAndRepair(tidy_doc);
        if (rc >= 0) {
            const TidyNode root = tidyGetRoot(tidy_doc);
            parse_html_data(set, root);
        }
    }

    //printf("%s\n", response.string);
    hash_set_display(set);

    curl_easy_cleanup(curl);
    tidyBufFree(&errbuf);
    tidyRelease(tidy_doc);
    free(response.string);
    hash_set_free(set);

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

void parse_html_data(Set *set, const TidyNode tnode)
{
    for (TidyNode child = tidyGetChild(tnode); child; child = tidyGetNext(child)) {
        const TidyTagId tag_id = tidyNodeGetId(child);

        if (tag_id == TidyTag_A) {
            const TidyAttr href_attr = tidyAttrGetById(child, TidyAttr_HREF);
            if (href_attr) {
                const char *url = tidyAttrValue(href_attr);
                if (url && strstr(url, "http") != NULL && !hash_set_contains(set, url)) {
                    //printf("Found URL: %s\n", url);
                    hash_set_add(set, url);
                }
            }
        }

        parse_html_data(set, child);
    }
}
