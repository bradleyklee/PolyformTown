#include "vcomp.h"
#include "attach.h"
#include <string.h>

#define MAX_BOUNDARY_VERTS (MAX_VERTS * MAX_CYCLES)

typedef struct {
    const Tile *tile;
    Coord target;
    int base_hidden_count;
    VCompEmitFn emit;
    void *userdata;
} VCompCtx;

static int coord_in_list(const Coord *verts, int count, Coord v) {
    for (int i = 0; i < count; i++) if (coord_eq(verts[i], v)) return 1;
    return 0;
}

static int coord_adjacent_lattice(Coord a, Coord b, int lattice) {
    int dx = b.x - a.x;
    int dy = b.y - a.y;
    if (dx == 1 && dy == 0) return 1;
    if (dx == -1 && dy == 0) return 1;
    if (dx == 0 && dy == 1) return 1;
    if (dx == 0 && dy == -1) return 1;
    if (lattice != TILE_LATTICE_SQUARE) {
        if (dx == 1 && dy == -1) return 1;
        if (dx == -1 && dy == 1) return 1;
    }
    return 0;
}

static int point_on_segment(Coord p, Coord a, Coord b, int lattice) {
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
    if (lattice != TILE_LATTICE_SQUARE && a.x + a.y == b.x + b.y) {
        int xmin = (a.x < b.x) ? a.x : b.x;
        int xmax = (a.x > b.x) ? a.x : b.x;
        return p.x + p.y == a.x + a.y && p.x >= xmin && p.x <= xmax;
    }
    return 0;
}

static int point_on_cycle(const Cycle *c, Coord p, int lattice) {
    for (int i = 0; i < c->n; i++) {
        Coord a = c->v[i];
        Coord b = c->v[(i + 1) % c->n];
        if (point_on_segment(p, a, b, lattice)) return 1;
    }
    return 0;
}

static int point_on_poly_boundary(const Poly *p, Coord q, int lattice) {
    for (int i = 0; i < p->cycle_count; i++) {
        if (point_on_cycle(&p->cycles[i], q, lattice)) return 1;
    }
    return 0;
}

int build_boundary_vertices(const Poly *p, Coord *verts) {
    int count = 0;
    for (int i = 0; i < p->cycle_count; i++) {
        const Cycle *c = &p->cycles[i];
        for (int j = 0; j < c->n; j++) {
            Coord v = c->v[j];
            if (!coord_in_list(verts, count, v)) verts[count++] = v;
        }
    }
    return count;
}

static int hidden_connected_lattice(const Coord *hidden, int hidden_count, int lattice) {
    int queue[MAX_BOUNDARY_VERTS];
    int seen[MAX_BOUNDARY_VERTS] = {0};
    int qh = 0, qt = 0, reached = 0;
    if (hidden_count <= 1) return 1;
    seen[0] = 1;
    queue[qt++] = 0;
    while (qh < qt) {
        int cur = queue[qh++];
        reached++;
        for (int i = 0; i < hidden_count; i++) {
            if (seen[i]) continue;
            if (!coord_adjacent_lattice(hidden[cur], hidden[i], lattice)) continue;
            seen[i] = 1;
            queue[qt++] = i;
        }
    }
    return reached == hidden_count;
}

static int build_next_hidden(const Coord *prev_boundary,
                             int prev_count,
                             const Poly *grown,
                             const Coord *old_hidden,
                             int old_hidden_count,
                             int lattice,
                             Coord *out_hidden) {
    int out_count = 0;
    for (int i = 0; i < old_hidden_count; i++) {
        if (!coord_in_list(out_hidden, out_count, old_hidden[i])) out_hidden[out_count++] = old_hidden[i];
    }
    for (int i = 0; i < prev_count; i++) {
        Coord v = prev_boundary[i];
        if (!point_on_poly_boundary(grown, v, lattice) && !coord_in_list(out_hidden, out_count, v)) {
            out_hidden[out_count++] = v;
        }
    }
    return out_count;
}

static void dfs_vertex_completions(const Poly *p,
                                   const VCompCtx *ctx,
                                   const Coord *hidden,
                                   int hidden_count,
                                   const Coord *prev_boundary,
                                   int prev_boundary_count) {
    Edge frontier[MAX_VERTS * MAX_CYCLES];
    int fc = build_frontier_edges(p, frontier);
    for (int be = 0; be < fc; be++) {
        if (!coord_eq(frontier[be].b, ctx->target)) continue;
        for (int v = 0; v < ctx->tile->variant_count; v++) {
            const Cycle *tv = &ctx->tile->variants[v];
            for (int te = 0; te < tv->n; te++) {
                Poly grown;
                Coord next_hidden[MAX_BOUNDARY_VERTS];
                Coord grown_boundary[MAX_BOUNDARY_VERTS];
                int next_hidden_count, grown_boundary_count, target_present;
                if (!try_attach_tile_poly(p, tv, ctx->tile->lattice, be, te, &grown)) continue;
                grown_boundary_count = build_boundary_vertices(&grown, grown_boundary);
                next_hidden_count = build_next_hidden(prev_boundary, prev_boundary_count,
                                                      &grown, hidden, hidden_count,
                                                      ctx->tile->lattice, next_hidden);
                if (!hidden_connected_lattice(next_hidden, next_hidden_count, ctx->tile->lattice)) continue;
                target_present = point_on_poly_boundary(&grown, ctx->target, ctx->tile->lattice);
                if (!target_present) {
                    if (next_hidden_count > ctx->base_hidden_count) {
                        ctx->emit(&grown, next_hidden, next_hidden_count, ctx->userdata);
                    }
                    continue;
                }
                dfs_vertex_completions(&grown, ctx, next_hidden, next_hidden_count,
                                       grown_boundary, grown_boundary_count);
            }
        }
    }
}

void enumerate_vertex_completions(const Poly *base,
                                  const Tile *tile,
                                  Coord target,
                                  const Coord *initial_hidden,
                                  int initial_hidden_count,
                                  VCompEmitFn emit,
                                  void *userdata) {
    VCompCtx ctx;
    Coord initial_boundary[MAX_BOUNDARY_VERTS];
    memset(&ctx, 0, sizeof(ctx));
    ctx.tile = tile;
    ctx.target = target;
    ctx.base_hidden_count = initial_hidden_count;
    ctx.emit = emit;
    ctx.userdata = userdata;
    int initial_boundary_count = build_boundary_vertices(base, initial_boundary);
    dfs_vertex_completions(base, &ctx, initial_hidden, initial_hidden_count,
                           initial_boundary, initial_boundary_count);
}
