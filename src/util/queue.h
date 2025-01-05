#ifndef QUEUE_H
#define QUEUE_H

#include <stdlib.h>
#include <stdio.h>

#define INITIAL_CAPACITY 1000

typedef struct {
    char **urls;    // Array of URL strings
    int front;      // Front index
    int back;       // Rear index
    int capacity;   // Current capacity
    int size;       // Current size
} Queue;

Queue *queue_init();
void queue_enqueue(Queue *queue, const char *url);
char *queue_dequeue(Queue *queue);
void queue_free(Queue *queue);
int queue_empty(const Queue *queue);

#endif // QUEUE_H
