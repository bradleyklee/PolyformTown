#ifndef HASH_H
#define HASH_H

#include "cycle.h"
#include <stddef.h>

typedef struct {
    Poly p;
    int used;
} HashSlot;

typedef struct {
    HashSlot *slots;
    size_t size;
    size_t count;
} HashTable;

void hash_init(HashTable *h, size_t size);
void hash_destroy(HashTable *h);
int hash_insert(HashTable *h, const Poly *p);

#endif
