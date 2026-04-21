#include "hash.h"
#include <stdlib.h>
#include <stdio.h>

static unsigned poly_hash(const Poly *p) {
    unsigned h = 2166136261u;
    h ^= (unsigned)p->cycle_count; h *= 16777619u;
    for (int k = 0; k < p->cycle_count; k++) {
        const Cycle *c = &p->cycles[k];
        h ^= (unsigned)c->n; h *= 16777619u;
        for (int i = 0; i < c->n; i++) {
            h ^= (unsigned)(c->v[i].v + 10000); h *= 16777619u;
            h ^= (unsigned)(c->v[i].x + 10000); h *= 16777619u;
            h ^= (unsigned)(c->v[i].y + 10000); h *= 16777619u;
        }
    }
    return h;
}

static int poly_equal(const Poly *a, const Poly *b) {
    if (a->cycle_count != b->cycle_count) return 0;
    for (int k = 0; k < a->cycle_count; k++) {
        const Cycle *ca = &a->cycles[k];
        const Cycle *cb = &b->cycles[k];
        if (ca->n != cb->n) return 0;
        for (int i = 0; i < ca->n; i++) {
            if (ca->v[i].v != cb->v[i].v || ca->v[i].x != cb->v[i].x || ca->v[i].y != cb->v[i].y) return 0;
        }
    }
    return 1;
}

void hash_init(HashTable *h, size_t size) {
    h->size = size;
    h->count = 0;
    h->slots = calloc(size, sizeof(HashSlot));
    if (!h->slots) { fprintf(stderr, "oom\n"); exit(1); }
}

void hash_destroy(HashTable *h) {
    free(h->slots);
    h->slots = NULL;
    h->size = h->count = 0;
}

static void hash_resize(HashTable *h) {
    HashTable nh;
    hash_init(&nh, h->size * 2);
    for (size_t i = 0; i < h->size; i++) {
        if (!h->slots[i].used) continue;
        Poly *p = &h->slots[i].p;
        unsigned idx = poly_hash(p) % nh.size;
        while (nh.slots[idx].used) idx = (idx + 1) % nh.size;
        nh.slots[idx].used = 1;
        nh.slots[idx].p = *p;
        nh.count++;
    }
    free(h->slots);
    *h = nh;
}

int hash_insert(HashTable *h, const Poly *p) {
    if ((h->count + 1) * 10 > h->size * 7) hash_resize(h);
    unsigned idx = poly_hash(p) % h->size;
    while (h->slots[idx].used) {
        if (poly_equal(&h->slots[idx].p, p)) return 0;
        idx = (idx + 1) % h->size;
    }
    h->slots[idx].used = 1;
    h->slots[idx].p = *p;
    h->count++;
    return 1;
}
