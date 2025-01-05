#ifndef HASH_SET_H
#define HASH_SET_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define TABLE_SIZE 10000

typedef struct Node
{
    char *key;
    struct Node *nextNode;
} Node;

typedef struct
{
    Node *table[TABLE_SIZE];
} Set;

/* Hash set functions */
Set *hash_set_init();
void hash_set_add(Set *set, const char *key);
int hash_set_contains(Set *set, const char *key);
void hash_set_display(Set *set);
void hash_set_free(Set *set);

#endif // HASH_SET_H
