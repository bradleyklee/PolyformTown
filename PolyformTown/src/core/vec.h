#ifndef VEC_H
#define VEC_H

#include "core/cycle.h"
#include <stddef.h>

typedef struct {
    Poly *data;
    size_t count;
    size_t cap;
} PolyVec;

void vec_init(PolyVec *v, size_t cap);
void vec_clear(PolyVec *v);
void vec_push(PolyVec *v, const Poly *p);
void vec_destroy(PolyVec *v);

#endif
