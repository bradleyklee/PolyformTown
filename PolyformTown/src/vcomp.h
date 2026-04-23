#ifndef VCOMP_H
#define VCOMP_H

#include "cycle.h"
#include "tile.h"

typedef void (*VCompEmitFn)(const Poly *p,
                            const Coord *hidden,
                            int hidden_count,
                            void *userdata);

int build_boundary_vertices(const Poly *p, Coord *verts);
void enumerate_vertex_completions(const Poly *base,
                                  const Tile *tile,
                                  Coord target,
                                  const Coord *initial_hidden,
                                  int initial_hidden_count,
                                  VCompEmitFn emit,
                                  void *userdata);
#endif
