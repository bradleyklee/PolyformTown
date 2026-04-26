#ifndef LATTICE_H
#define LATTICE_H

#include "core/tile.h"

int lattice_transform_count(int lattice);
int lattice_direction_count(int lattice);
int lattice_unit_direction(int lattice, int dx, int dy);
int lattice_direction_index(int lattice, Edge e);
int lattice_coords_adjacent(int lattice, Coord a, Coord b);
void lattice_embed_point(int lattice, Coord p, double *x, double *y);

#endif
