#ifndef VCOMP_H
#define VCOMP_H

#include "cycle.h"
#include "tile.h"

typedef void (*VCompEmitFn)(const Poly *p, int steps, void *userdata);

int build_boundary_vertices(const Poly *p, Coord *verts);
void enumerate_vertex_completions(const Poly *base,
                                  const Tile *tile,
                                  Coord target,
                                  VCompEmitFn emit,
                                  void *userdata);
int vcomp_square_interior_connected(const Poly *p);
int vcomp_build_interior_vertices_square(const Poly *p, Coord *out);

#endif
