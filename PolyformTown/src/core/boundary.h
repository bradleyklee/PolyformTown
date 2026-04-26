#ifndef CORE_BOUNDARY_H
#define CORE_BOUNDARY_H

#include "core/tile.h"

int build_boundary_vertices(const Poly *p, Coord *verts);
int poly_has_live_boundary(const Poly *p, const Tile *tile);

#endif
