#include "main.h"

int main(void)
{
    Set *set = hash_set_init();
    Queue *queue = queue_init();

    const double elapsed_time = run_crawler(set, queue);

    if (elapsed_time)
    {
        hash_set_display(set);
        printf("Total Run-Time Taken For Searching: %.2f seconds\n", elapsed_time);
    } else
    {
        fprintf(stderr, "Failed to perform the web crawler search.");
    }

    hash_set_free(set);
    queue_free(queue);

    return 0;
}
