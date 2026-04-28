#ifndef CORE_BOUNDARY_H
#define CORE_BOUNDARY_H

#include "core/tile.h"

int build_boundary_vertices(const Poly *p, Coord *verts);

int build_boundary_edges(const Poly *p, Edge *edges);

int poly_has_live_boundary(const Poly *p, const Tile *tile);

int poly_has_vertex_completion(const Poly *p,
                               const Tile *tile,
                               Coord target);

#endif
