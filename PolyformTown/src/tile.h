#ifndef TILE_H
#define TILE_H

#include "cycle.h"

typedef struct {
    char name[64];
    Cycle base;
    int variant_count;
    Cycle variants[8];
} Tile;

int tile_load(const char *path, Tile *tile);
void tile_build_variants(Tile *tile);

#endif
