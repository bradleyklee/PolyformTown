#ifndef ATTACH_H
#define ATTACH_H

#include "cycle.h"
#include "tile.h"

int build_frontier_edges(const Poly *p, Edge *edges);
int try_attach_tile_poly(const Poly *base, const Cycle *tile_variant,
                         int base_edge_index, int tile_edge_index,
                         Poly *out);

#endif
