#include "queue.h"

Queue *queue_init()
{
    Queue *queue = (Queue *) malloc(sizeof(Queue));
    queue->urls = malloc(sizeof(char *) * INITIAL_CAPACITY);
    queue->front = 0;
    queue->back = -1;
    queue->capacity = INITIAL_CAPACITY;
    queue->size = 0;
    return queue;
}

void queue_enqueue(Queue *queue, const char* url)
{
    if (queue->size == queue->capacity)
    {
        queue->capacity *= 2;
        queue->urls = realloc(queue->urls, sizeof(char *) * queue->capacity);
    }
    queue->back = (queue->back+1) % queue->capacity;
    queue->urls[queue->back] = url;
    queue->size++;
}

char *queue_dequeue(Queue *queue)
{
    if (queue->size == 0)
    {
        return NULL;
    }
    char *url = queue->urls[queue->front];
    queue->front = (queue->front+1) % queue->capacity;
    queue->size--;

    return url;
}

void queue_free(Queue *queue)
{
    for (int i = 0; i < queue->size; i++)
    {
        free(queue->urls[(queue->front+i) % queue->capacity]);
    }
    free(queue->urls);
    free(queue);
}

int queue_empty(const Queue *queue)
{
    return queue->size == 0;
}