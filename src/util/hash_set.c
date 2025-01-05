#include "hash_set.h"

Set *hash_set_init()
{
    Set *set = (Set*) malloc(sizeof(Set));
    memset(set->table, 0, TABLE_SIZE * sizeof(Node *));
    return set;
}

// djb2 hash formula implementation
static unsigned int hash_set_hash(const char *str)
{
    unsigned int hash = 5381;
    int c;

    while ((c = *str++))
    {
        hash = ((hash << 5) + hash) + c;
    }

    return hash % TABLE_SIZE;
}

int hash_set_contains(Set *set, const char *key)
{
    const int index = hash_set_hash(key);
    const Node *curr = set->table[index];

    while (curr != NULL)
    {
        if (strcmp(curr->key, key) == 0)
        {
            return 1;
        }
        curr = curr->nextNode;
    }
    return 0;
}

void hash_set_add(Set *set, const char *key)
{
    const int index = hash_set_hash(key);

    Node *curr = set->table[index];
    while (curr != NULL)
    {
        if (strcmp(curr->key, key) == 0)
        {
            return;
        }
        curr = curr->nextNode;
    }

    Node *node = (Node *)malloc(sizeof(Node));
    if (node == NULL)
    {
        fprintf(stderr, "Hash Set Error: Failed to allocate memory for node\n");
        return;
    }

    node->key = strdup(key);
    node->nextNode = set->table[index];
    set->table[index] = node;
}

void hash_set_display(Set *set)
{
    printf("\nDisplay Hash Set Data:\n");
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        Node *curr = set->table[i];
        while (curr != NULL)
        {
            printf("%s\n", curr->key);
            curr = curr->nextNode;
        }
    }
}

void hash_set_free(Set *set)
{
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        Node *curr = set->table[i];
        while (curr != NULL)
        {
            Node *nextNode = curr->nextNode;
            free(curr->key);
            free(curr);
            curr = nextNode;
        }
    }
    free(set);
}
