#ifndef TETRILLE_H
#define TETRILLE_H

#include "cycle.h"
#include "tile.h"

int tetrille_point_to_sys3_scaled(Coord p, int *x, int *y);
int tetrille_point_to_sys4(Coord p, int *x, int *y);
typedef struct { int system; int dx; int dy; } TetrilleTaggedVec;

int tetrille_delta_to_6(int valence, int dx, int dy, int *m, int *n);
int tetrille_edge_tag(Edge e, TetrilleTaggedVec *tv);
void tetrille_translate_cycle(Cycle *c, int m6, int n6);
int tetrille_coords_adjacent(Coord a, Coord b);
void tetrille_embed_point_scaled(Coord p, long long *x, long long *y);
int tetrille_point_on_segment(Coord p, Coord a, Coord b);
void tetrille_poly_canonical_key(const Poly *in, Poly *out);

#endif
