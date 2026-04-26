#include "core/lattice.h"

#include "core/tetrille.h"

int lattice_transform_count(int lattice) {
    if (lattice == TILE_LATTICE_TRIANGULAR) return 12;
    if (lattice == TILE_LATTICE_TETRILLE) return 12;
    if (lattice == TILE_LATTICE_SQUARE) return 8;
    return 1;
}

int lattice_direction_count(int lattice) {
    if (lattice == TILE_LATTICE_TRIANGULAR) return 6;
    if (lattice == TILE_LATTICE_TETRILLE) return 6;
    return 4;
}

int lattice_unit_direction(int lattice, int dx, int dy) {
    if (dx == 1 && dy == 0) return 1;
    if (dx == -1 && dy == 0) return 1;
    if (dx == 0 && dy == 1) return 1;
    if (dx == 0 && dy == -1) return 1;
    if (lattice != TILE_LATTICE_SQUARE) {
        if (dx == 1 && dy == -1) return 1;
        if (dx == -1 && dy == 1) return 1;
    }
    return 0;
}

int lattice_direction_index(int lattice, Edge e) {
    int dx = e.b.x - e.a.x;
    int dy = e.b.y - e.a.y;

    if (lattice == TILE_LATTICE_TETRILLE) {
        TetrilleTaggedVec tv;
        if (!tetrille_edge_tag(e, &tv)) return -1;
        dx = tv.dx;
        dy = tv.dy;
    }

    if (lattice != TILE_LATTICE_SQUARE) {
        if (dx == 1 && dy == 0) return 0;
        if (dx == 0 && dy == 1) return 1;
        if (dx == -1 && dy == 1) return 2;
        if (dx == -1 && dy == 0) return 3;
        if (dx == 0 && dy == -1) return 4;
        if (dx == 1 && dy == -1) return 5;
        return -1;
    }

    if (dx == 1 && dy == 0) return 0;
    if (dx == 0 && dy == 1) return 1;
    if (dx == -1 && dy == 0) return 2;
    if (dx == 0 && dy == -1) return 3;
    return -1;
}

int lattice_coords_adjacent(int lattice, Coord a, Coord b) {
    if (lattice == TILE_LATTICE_TETRILLE) return tetrille_coords_adjacent(a, b);
    return lattice_unit_direction(lattice, b.x - a.x, b.y - a.y);
}

void lattice_embed_point(int lattice, Coord p, double *x, double *y) {
    if (lattice == TILE_LATTICE_TETRILLE) {
        long long sx, sy;
        tetrille_embed_point_scaled(p, &sx, &sy);
        *x = (double)sx;
        *y = (double)sy;
        return;
    }
    *x = (double)p.x;
    *y = (double)p.y;
}
