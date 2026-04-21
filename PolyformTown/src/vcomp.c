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

static int classify_state(const VCompCtx *ctx, const Poly *p) {
    Coord current[MAX_BOUNDARY_VERTS];
    int current_count = build_boundary_vertices(p, current);
    int target_present = coord_in_list(current, current_count, ctx->target);

    for (int i = 0; i < ctx->initial_count; i++) {
        Coord v = ctx->initial[i];
        if (coord_eq(v, ctx->target)) continue;
        if (!coord_in_list(current, current_count, v)) {
            return 0; /* invalid: non-target initial boundary vertex vanished */
        }
    }

    return target_present ? 1 : 2; /* 1 recurse, 2 emit */
}

static void dfs_vertex_completions(const Poly *p, const VCompCtx *ctx) {
    Edge frontier[MAX_VERTS * MAX_CYCLES];
    int fc = build_frontier_edges(p, frontier);
    int advanced = 0;

    for (int be = 0; be < fc; be++) {
        if (!coord_eq(frontier[be].b, ctx->target)) continue;

        for (int v = 0; v < ctx->tile->variant_count; v++) {
            const Cycle *tv = &ctx->tile->variants[v];
            for (int te = 0; te < tv->n; te++) {
                Poly grown;
                int state;

                if (!try_attach_tile_poly(p, tv, ctx->tile->lattice, be, te, &grown)) continue;
                advanced = 1;

                state = classify_state(ctx, &grown);
                if (state == 0) continue;
                if (state == 2) {
                    ctx->emit(&grown, ctx->userdata);
                    continue;
                }
                dfs_vertex_completions(&grown, ctx);
            }
        }
    }

    (void)advanced;
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
    ctx.emit = emit;
    ctx.userdata = userdata;
    ctx.initial_count = build_boundary_vertices(base, ctx.initial);
    dfs_vertex_completions(base, &ctx);
}
