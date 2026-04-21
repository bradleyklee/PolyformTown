#ifndef TILE_H
#define TILE_H

#include "cycle.h"

#define TILE_LATTICE_SQUARE 0
#define TILE_LATTICE_TRIANGULAR 1
#define MAX_VARIANTS 12

typedef struct {
    char name[64];
    Cycle base;
    int lattice;
    int variant_count;
    Cycle variants[MAX_VARIANTS];
} Tile;

int tile_load(const char *path, Tile *tile);
void tile_build_variants(Tile *tile);

#endif
