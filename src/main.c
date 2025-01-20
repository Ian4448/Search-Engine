#include "main.h"

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

    const double elapsed_time = run_crawler(set, queue, tidy_doc, curl);

    if (elapsed_time)
    {
        hash_set_display(set);
        printf("Total Run-Time Taken For Searching: %.2f seconds\n", elapsed_time);
    } else
    {
        fprintf(stderr, "Failed to perform the web crawler search.");
    }

    curl_easy_cleanup(curl);
    tidyBufFree(&errbuf);
    tidyRelease(tidy_doc);
    hash_set_free(set);
    queue_free(queue);

    return 0;
}
