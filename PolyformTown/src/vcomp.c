#include "vcomp.h"
#include "attach.h"
#include <string.h>

#define MAX_BOUNDARY_VERTS (MAX_VERTS * MAX_CYCLES)

typedef struct {
    const Tile *tile;
    Coord target;
    Coord initial[MAX_BOUNDARY_VERTS];
    int initial_count;
    VCompEmitFn emit;
    void *userdata;
} VCompCtx;

static int coord_in_list(const Coord *verts, int count, Coord v) {
    for (int i = 0; i < count; i++) {
        if (coord_eq(verts[i], v)) return 1;
    }
    return 0;
}

static int coord_adjacent_lattice(Coord a, Coord b, int lattice) {
    int dx = b.x - a.x;
    int dy = b.y - a.y;
    if (dx == 1 && dy == 0) return 1;
    if (dx == -1 && dy == 0) return 1;
    if (dx == 0 && dy == 1) return 1;
    if (dx == 0 && dy == -1) return 1;
    if (lattice == TILE_LATTICE_TRIANGULAR) {
        if (dx == 1 && dy == -1) return 1;
        if (dx == -1 && dy == 1) return 1;
    }
    return 0;
}

static int point_on_segment(Coord p, Coord a, Coord b) {
    if (a.x == b.x) {
        int ymin = (a.y < b.y) ? a.y : b.y;
        int ymax = (a.y > b.y) ? a.y : b.y;
        return p.x == a.x && p.y >= ymin && p.y <= ymax;
    }
    if (a.y == b.y) {
        int xmin = (a.x < b.x) ? a.x : b.x;
        int xmax = (a.x > b.x) ? a.x : b.x;
        return p.y == a.y && p.x >= xmin && p.x <= xmax;
    }
    return 0;
}

static int point_on_cycle(const Cycle *c, Coord p) {
    for (int i = 0; i < c->n; i++) {
        Coord a = c->v[i];
        Coord b = c->v[(i + 1) % c->n];
        if (point_on_segment(p, a, b)) return 1;
    }
    return 0;
}

static int point_inside_cycle_strict(const Cycle *c, Coord p) {
    int yline2 = 2 * p.y + 1;
    int x2 = 2 * p.x;
    int crossings = 0;

    for (int i = 0; i < c->n; i++) {
        Coord a = c->v[i];
        Coord b = c->v[(i + 1) % c->n];
        int ymin, ymax;
        int ex2;

        if (a.y == b.y) continue;
        ymin = (a.y < b.y) ? a.y : b.y;
        ymax = (a.y > b.y) ? a.y : b.y;
        if (!(2 * ymin <= yline2 && yline2 < 2 * ymax)) continue;
        if (a.x != b.x) continue;
        ex2 = 2 * a.x;
        if (ex2 > x2) crossings++;
    }

    return (crossings & 1) != 0;
}

static int point_in_poly_strict(const Poly *p, Coord q) {
    int inside = 0;
    for (int i = 0; i < p->cycle_count; i++) {
        if (point_on_cycle(&p->cycles[i], q)) return 0;
        if (point_inside_cycle_strict(&p->cycles[i], q)) inside ^= 1;
    }
    return inside;
}

int vcomp_build_interior_vertices_square(const Poly *p, Coord *out) {
    int minx, maxx, miny, maxy;
    int count = 0;
    int init = 0;

    for (int i = 0; i < p->cycle_count; i++) {
        const Cycle *c = &p->cycles[i];
        for (int j = 0; j < c->n; j++) {
            Coord v = c->v[j];
            if (!init) {
                minx = maxx = v.x;
                miny = maxy = v.y;
                init = 1;
            } else {
                if (v.x < minx) minx = v.x;
                if (v.x > maxx) maxx = v.x;
                if (v.y < miny) miny = v.y;
                if (v.y > maxy) maxy = v.y;
            }
        }
    }

    if (!init) return 0;
    for (int y = miny + 1; y <= maxy - 1; y++) {
        for (int x = minx + 1; x <= maxx - 1; x++) {
            Coord q;
            q.v = 4;
            q.x = x;
            q.y = y;
            if (point_in_poly_strict(p, q)) out[count++] = q;
        }
    }
    return count;
}

int vcomp_square_interior_connected(const Poly *p) {
    Coord interior[MAX_BOUNDARY_VERTS];
    int queue[MAX_BOUNDARY_VERTS];
    int seen[MAX_BOUNDARY_VERTS] = {0};
    int ic = vcomp_build_interior_vertices_square(p, interior);
    int qh = 0, qt = 0;
    int reached = 0;

    if (ic <= 1) return 1;
    seen[0] = 1;
    queue[qt++] = 0;
    while (qh < qt) {
        int cur = queue[qh++];
        reached++;
        for (int i = 0; i < ic; i++) {
            if (seen[i]) continue;
            if (!coord_adjacent_lattice(interior[cur], interior[i],
                                        TILE_LATTICE_SQUARE)) {
                continue;
            }
            seen[i] = 1;
            queue[qt++] = i;
        }
    }
    return reached == ic;
}

int build_boundary_vertices(const Poly *p, Coord *verts) {
    int count = 0;
    for (int i = 0; i < p->cycle_count; i++) {
        const Cycle *c = &p->cycles[i];
        for (int j = 0; j < c->n; j++) {
            Coord v = c->v[j];
            if (!coord_in_list(verts, count, v)) {
                verts[count++] = v;
            }
        }
    }
    return count;
}

static int hidden_is_monotone(const Coord *old_hidden,
                              int old_count,
                              const Coord *new_hidden,
                              int new_count) {
    for (int i = 0; i < old_count; i++) {
        if (!coord_in_list(new_hidden, new_count, old_hidden[i])) return 0;
    }
    return 1;
}

static int hidden_connected_lattice(const Coord *hidden,
                                    int hidden_count,
                                    int lattice) {
    int queue[MAX_BOUNDARY_VERTS];
    int seen[MAX_BOUNDARY_VERTS] = {0};
    int qh = 0, qt = 0;
    int reached = 0;

    if (hidden_count <= 1) return 1;
    seen[0] = 1;
    queue[qt++] = 0;
    while (qh < qt) {
        int cur = queue[qh++];
        reached++;
        for (int i = 0; i < hidden_count; i++) {
            if (seen[i]) continue;
            if (!coord_adjacent_lattice(hidden[cur], hidden[i], lattice)) {
                continue;
            }
            seen[i] = 1;
            queue[qt++] = i;
        }
    }
    return reached == hidden_count;
}

static int build_next_hidden(const VCompCtx *ctx,
                             const Coord *grown_boundary,
                             int grown_count,
                             Coord *out_hidden) {
    int out_count = 0;

    out_hidden[out_count++] = ctx->target;
    for (int i = 0; i < ctx->initial_count; i++) {
        Coord v = ctx->initial[i];
        if (coord_eq(v, ctx->target)) continue;
        if (coord_in_list(grown_boundary, grown_count, v)) continue;
        if (!coord_in_list(out_hidden, out_count, v)) {
            out_hidden[out_count++] = v;
        }
    }
    return out_count;
}

static void dfs_vertex_completions(const Poly *p,
                                   const VCompCtx *ctx,
                                   const Coord *hidden,
                                   int hidden_count,
                                   int steps_so_far) {
    Edge frontier[MAX_VERTS * MAX_CYCLES];
    int square = (ctx->tile->lattice == TILE_LATTICE_SQUARE);
    Poly pcanon;
    int parent_interior_count = 0;

    if (square) {
        Coord parent_interior[MAX_BOUNDARY_VERTS];
        poly_canonicalize_lattice(p, &pcanon, ctx->tile->lattice);
        parent_interior_count =
            vcomp_build_interior_vertices_square(&pcanon, parent_interior);
    }
    int fc = build_frontier_edges(p, frontier);

    for (int be = 0; be < fc; be++) {
        if (!coord_eq(frontier[be].b, ctx->target)) continue;

        for (int v = 0; v < ctx->tile->variant_count; v++) {
            const Cycle *tv = &ctx->tile->variants[v];
            for (int te = 0; te < tv->n; te++) {
                Poly grown;
                Coord next_hidden[MAX_BOUNDARY_VERTS];
                int next_hidden_count;
                Coord grown_boundary[MAX_BOUNDARY_VERTS];
                int grown_boundary_count;
                int target_present;
                int delta_steps = 0;

                if (!try_attach_tile_poly(p, tv, ctx->tile->lattice, be, te, &grown)) continue;
                if (square && !vcomp_square_interior_connected(&grown)) {
                    continue;
                }
                if (square) {
                    Coord grown_interior[MAX_BOUNDARY_VERTS];
                    Poly gcanon;
                    int gc;
                    poly_canonicalize_lattice(&grown, &gcanon, ctx->tile->lattice);
                    gc = vcomp_build_interior_vertices_square(&gcanon,
                                                              grown_interior);
                    delta_steps = gc - parent_interior_count;
                    if (delta_steps < 0) delta_steps = 0;
                }

                grown_boundary_count = build_boundary_vertices(&grown,
                                                               grown_boundary);
                next_hidden_count = build_next_hidden(ctx,
                                                      grown_boundary,
                                                      grown_boundary_count,
                                                      next_hidden);
                if (!hidden_is_monotone(hidden, hidden_count,
                                        next_hidden, next_hidden_count)) {
                    continue;
                }
                if (ctx->tile->lattice == TILE_LATTICE_SQUARE &&
                    !hidden_connected_lattice(next_hidden, next_hidden_count,
                                              ctx->tile->lattice)) {
                    continue;
                }
                target_present = coord_in_list(grown_boundary,
                                               grown_boundary_count,
                                               ctx->target);
                if (!target_present) {
                    int steps = 1;
                    if (square) steps = steps_so_far + delta_steps;
                    if (steps < 1) steps = 1;
                    ctx->emit(&grown, steps, ctx->userdata);
                    continue;
                }
                dfs_vertex_completions(&grown, ctx,
                                       next_hidden, next_hidden_count,
                                       square ? (steps_so_far + delta_steps)
                                              : steps_so_far);
            }
        }
    }
}

void enumerate_vertex_completions(const Poly *base,
                                  const Tile *tile,
                                  Coord target,
                                  VCompEmitFn emit,
                                  void *userdata) {
    VCompCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.tile = tile;
    ctx.target = target;
    ctx.initial_count = build_boundary_vertices(base, ctx.initial);
    ctx.emit = emit;
    ctx.userdata = userdata;
    dfs_vertex_completions(base, &ctx, NULL, 0, 0);
}
