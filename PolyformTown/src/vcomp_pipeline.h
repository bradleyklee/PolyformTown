#ifndef VCOMP_PIPELINE_H
#define VCOMP_PIPELINE_H

#include "tile.h"

#define VCOMP_MAX_LEVELS 32
#define VCOMP_MAX_HIDDEN (MAX_VERTS * MAX_CYCLES)

typedef struct {
    Poly poly;
    Coord hidden[VCOMP_MAX_HIDDEN];
    int hidden_count;
} VCompState;

typedef struct {
    VCompState *data;
    size_t count, cap;
} VCompStateVec;

typedef int (*VCompLevelFn)(int level,
                            const VCompStateVec *states,
                            size_t unique_poly_count,
                            const Tile *tile,
                            void *userdata);

int vcomp_poly_hash_key(const Poly *p, int lattice, Poly *key);

void run_vcomp_levels(const Tile *tile,
                      int max_n,
                      VCompLevelFn on_level,
                      void *userdata);

#endif
