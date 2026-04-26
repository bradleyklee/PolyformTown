#include "core/boundary.h"

#include "core/attach.h"
#include "vcomp.h"

#define MAX_BOUNDARY_VERTS (MAX_VERTS * MAX_CYCLES)

static int coord_in_list(const Coord *verts, int count, Coord v) {
    for (int i = 0; i < count; i++) if (coord_eq(verts[i], v)) return 1;
    return 0;
}

int build_boundary_vertices(const Poly *p, Coord *verts) {
    int count = 0;
    for (int i = 0; i < p->cycle_count; i++) {
        const Cycle *c = &p->cycles[i];
        for (int j = 0; j < c->n; j++) {
            Coord v = c->v[j];
            if (!coord_in_list(verts, count, v)) {
                if (count >= MAX_BOUNDARY_VERTS) return -1;
                verts[count++] = v;
            }
        }
    }
    return count;
}

int poly_has_live_boundary(const Poly *p, const Tile *tile) {
    Coord verts[2 * MAX_BOUNDARY_VERTS];
    int vc = build_frontier_vertices(p, verts);
    if (vc < 0 || vc > 2 * MAX_BOUNDARY_VERTS) return 0;
    for (int i = 0; i < vc; i++) {
        if (!has_vertex_completion_steps(p, tile, verts[i], NULL, 0, -1)) {
            return 0;
        }
    }
    return 1;
}
