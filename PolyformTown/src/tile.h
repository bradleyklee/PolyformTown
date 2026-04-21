#ifndef TILE_H
#define TILE_H

#include "cycle.h"

#define TILE_LATTICE_SQUARE 0
#define TILE_LATTICE_TRIANGULAR 1
#define TILE_LATTICE_TETRILLE 2
#define MAX_VARIANTS 12
#define MAX_TILE_CONSTANTS 16
#define MAX_TILE_EXPR 64
#define MAX_TILE_BASES 16

typedef struct {
    char name[32];
    char expr[MAX_TILE_EXPR];
} TileConstant;

typedef struct {
    int valence;
    char a11[MAX_TILE_EXPR];
    char a12[MAX_TILE_EXPR];
    char a21[MAX_TILE_EXPR];
    char a22[MAX_TILE_EXPR];
} TileBasis;

typedef struct {
    char name[64];
    Cycle base;
    int lattice;
    int variant_count;
    Cycle variants[MAX_VARIANTS];
    int constant_count;
    TileConstant constants[MAX_TILE_CONSTANTS];
    int basis_count;
    TileBasis bases[MAX_TILE_BASES];
} Tile;

int tile_load(const char *path, Tile *tile);
void tile_build_variants(Tile *tile);
void tile_print_imgtable_shape(const Tile *tile, const Poly *poly);
void tile_print_imgtable_cycle(const Tile *tile, const Cycle *cycle);

#endif
