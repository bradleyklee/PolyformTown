#ifndef ATTACH_H
#define ATTACH_H

#include "cycle.h"
#include "tile.h"

int build_frontier_edges(const Poly *p, Edge *edges);
int build_frontier_vertices(const Poly *p, Coord *verts);
int poly_has_boundary_vertex(const Poly *p, Coord v);
int align_tile_to_frontier_edge(const Poly *base, const Cycle *tile_variant,
                                int base_edge_index, int tile_edge_index,
                                Cycle *aligned);
int try_attach_tile_poly(const Poly *base, const Cycle *tile_variant,
                         int lattice,
                         int base_edge_index, int tile_edge_index,
                         Poly *out);

#endif
