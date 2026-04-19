#include "vec.h"
#include <stdlib.h>
#include <stdio.h>

void vec_init(PolyVec *v, size_t cap) {
    v->data = malloc(cap * sizeof(Poly));
    if (!v->data) { fprintf(stderr, "oom\n"); exit(1); }
    v->count = 0;
    v->cap = cap;
}

void vec_clear(PolyVec *v) { v->count = 0; }

void vec_push(PolyVec *v, const Poly *p) {
    if (v->count == v->cap) {
        v->cap *= 2;
        v->data = realloc(v->data, v->cap * sizeof(Poly));
        if (!v->data) { fprintf(stderr, "oom\n"); exit(1); }
    }
    v->data[v->count++] = *p;
}

void vec_destroy(PolyVec *v) {
    free(v->data);
    v->data = NULL;
    v->count = v->cap = 0;
}
