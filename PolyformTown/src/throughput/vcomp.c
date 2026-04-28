#include "vcomp.h"
#include "core/attach.h"
#include "core/boundary.h"
#include "core/cycle.h"
#include "core/lattice.h"
#include "core/tetrille.h"
#include <string.h>

#define MAX_BOUNDARY_VERTS (MAX_VERTS * MAX_CYCLES)

typedef struct {
    const Tile *tile;
    Cycle variants[MAX_VARIANTS];
    int variant_count;
    Coord target;
    int base_hidden_count;
    int required_steps;
    int max_steps;
    int stop_after_first;
    int *stop_flag;
    VCompEmitFn emit;
    VCompTraceEmitFn trace_emit;
    void *userdata;
} VCompCtx;

//duplicate with boundary
static int coord_in_list(const Coord *verts, int count, Coord v) {
    for (int i = 0; i < count; i++) if (coord_eq(verts[i], v)) return 1;
    return 0;
}

static int point_on_segment(Coord p, Coord a, Coord b, int lattice) {
    if (lattice == TILE_LATTICE_TETRILLE) {
        return tetrille_point_on_segment(p, a, b);
    }
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

static int hidden_connected_lattice(const Coord *hidden, int hidden_count, int lattice) {
    int queue[MAX_BOUNDARY_VERTS];
    int seen[MAX_BOUNDARY_VERTS] = {0};
    int qh = 0, qt = 0, reached = 0;
    if (hidden_count > MAX_BOUNDARY_VERTS) return 0;
    if (hidden_count <= 1) return 1;
    seen[0] = 1;
    queue[qt++] = 0;
    while (qh < qt) {
        int cur = queue[qh++];
        reached++;
        for (int i = 0; i < hidden_count; i++) {
            if (seen[i]) continue;
            if (!lattice_coords_adjacent(lattice, hidden[cur], hidden[i])) continue;
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
        if (!coord_in_list(out_hidden, out_count, old_hidden[i])) {
            if (out_count >= MAX_BOUNDARY_VERTS) return -1;
            out_hidden[out_count++] = old_hidden[i];
        }
    }
    for (int i = 0; i < prev_count; i++) {
        Coord v = prev_boundary[i];
        if (!point_on_poly_boundary(grown, v, lattice) && !coord_in_list(out_hidden, out_count, v)) {
            if (out_count >= MAX_BOUNDARY_VERTS) return -1;
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
                                   int prev_boundary_count,
                                   int completion_steps,
                                   Cycle *trace_tiles,
                                   int *trace_indices,
                                   int trace_tile_count) {
    if (ctx->stop_after_first && *ctx->stop_flag) return;
    if (ctx->max_steps >= 0 && completion_steps >= ctx->max_steps) return;
    Edge frontier[MAX_VERTS * MAX_CYCLES];
    int fc = build_boundary_edges(p, frontier);
    for (int be = 0; be < fc; be++) {
        if (ctx->stop_after_first && *ctx->stop_flag) return;
        if (!coord_eq(frontier[be].b, ctx->target)) continue;
        for (int v = 0; v < ctx->variant_count; v++) {
            const Cycle *tv = &ctx->variants[v];
            for (int te = 0; te < tv->n; te++) {
                Poly grown;
                Cycle aligned;
                Coord next_hidden[MAX_BOUNDARY_VERTS];
                Coord grown_boundary[MAX_BOUNDARY_VERTS];
                int next_hidden_count, grown_boundary_count, target_present;
                if (!try_attach_tile_poly_ex(p, tv, ctx->tile->lattice,
                                             be, te, &grown, &aligned)) {
                    continue;
                }
                grown_boundary_count = build_boundary_vertices(&grown, grown_boundary);
                next_hidden_count = build_next_hidden(prev_boundary, prev_boundary_count,
                                                      &grown, hidden, hidden_count,
                                                      ctx->tile->lattice, next_hidden);
                if (grown_boundary_count < 0 || next_hidden_count < 0) continue;
                if (!hidden_connected_lattice(next_hidden, next_hidden_count, ctx->tile->lattice)) continue;
                target_present = point_on_poly_boundary(&grown, ctx->target, ctx->tile->lattice);
                if (!target_present) {
                    if (next_hidden_count > ctx->base_hidden_count) {
                        if (ctx->required_steps < 0 ||
                            completion_steps + 1 == ctx->required_steps) {
                            if (ctx->trace_emit) {
                                Cycle out_tiles[MAX_VERTS + 1];
                                int out_indices[MAX_VERTS + 1];
                                for (int ti = 0; ti < trace_tile_count; ti++) {
                                    out_tiles[ti] = trace_tiles[ti];
                                    out_indices[ti] = trace_indices[ti];
                                }
                                out_tiles[trace_tile_count] = aligned;
                                out_indices[trace_tile_count] = -1;
                                for (int vi = 0; vi < aligned.n; vi++) {
                                    if (coord_eq(aligned.v[vi], ctx->target)) {
                                        out_indices[trace_tile_count] = vi;
                                        break;
                                    }
                                }
                                ctx->trace_emit(&grown, next_hidden, next_hidden_count,
                                                out_tiles, trace_tile_count + 1,
                                                out_indices,
                                                ctx->userdata);
                            } else {
                                ctx->emit(&grown, next_hidden, next_hidden_count,
                                          ctx->userdata);
                            }
                            if (ctx->stop_after_first) {
                                *ctx->stop_flag = 1;
                                return;
                            }
                        }
                    }
                    continue;
                }
                if (ctx->trace_emit) {
                    if (trace_tile_count < MAX_VERTS) {
                        trace_tiles[trace_tile_count] = aligned;
                        trace_indices[trace_tile_count] = -1;
                        for (int vi = 0; vi < aligned.n; vi++) {
                            if (coord_eq(aligned.v[vi], ctx->target)) {
                                trace_indices[trace_tile_count] = vi;
                                break;
                            }
                        }
                        dfs_vertex_completions(&grown, ctx,
                                               next_hidden, next_hidden_count,
                                               grown_boundary, grown_boundary_count,
                                               completion_steps + 1,
                                               trace_tiles, trace_indices,
                                               trace_tile_count + 1);
                    }
                } else {
                    dfs_vertex_completions(&grown, ctx,
                                           next_hidden, next_hidden_count,
                                           grown_boundary, grown_boundary_count,
                                           completion_steps + 1,
                                           trace_tiles, trace_indices,
                                           trace_tile_count);
                }
            }
        }
    }
}

void enumerate_vertex_completions_steps(const Poly *base,
                                        const Tile *tile,
                                        Coord target,
                                        const Coord *initial_hidden,
                                        int initial_hidden_count,
                                        int required_steps,
                                        VCompEmitFn emit,
                                        void *userdata) {
    VCompCtx ctx;
    Coord initial_boundary[MAX_BOUNDARY_VERTS];
    Cycle trace_tiles[MAX_VERTS];
    int trace_indices[MAX_VERTS];
    int stop = 0;
    memset(&ctx, 0, sizeof(ctx));
    ctx.tile = tile;
    ctx.target = target;
    ctx.base_hidden_count = initial_hidden_count;
    ctx.required_steps = required_steps;
    ctx.max_steps = required_steps;
    ctx.stop_after_first = 0;
    ctx.stop_flag = &stop;
    ctx.emit = emit;
    ctx.userdata = userdata;
    ctx.variant_count = tile->variant_count;
    for (int i = 0; i < tile->variant_count; i++) {
        ctx.variants[i] = tile->variants[i];
    }
    int initial_boundary_count = build_boundary_vertices(base, initial_boundary);
    dfs_vertex_completions(base, &ctx, initial_hidden, initial_hidden_count,
                           initial_boundary, initial_boundary_count, 0,
                           trace_tiles, trace_indices, 0);
}

void enumerate_vertex_completions_steps_trace(const Poly *base,
                                              const Tile *tile,
                                              Coord target,
                                              const Coord *initial_hidden,
                                              int initial_hidden_count,
                                              int required_steps,
                                              VCompTraceEmitFn emit,
                                              void *userdata) {
    VCompCtx ctx;
    Coord initial_boundary[MAX_BOUNDARY_VERTS];
    Cycle trace_tiles[MAX_VERTS];
    int trace_indices[MAX_VERTS];
    int stop = 0;
    memset(&ctx, 0, sizeof(ctx));
    ctx.tile = tile;
    ctx.target = target;
    ctx.base_hidden_count = initial_hidden_count;
    ctx.required_steps = required_steps;
    ctx.max_steps = required_steps;
    ctx.stop_after_first = 0;
    ctx.stop_flag = &stop;
    ctx.trace_emit = emit;
    ctx.userdata = userdata;
    ctx.variant_count = tile->variant_count;
    for (int i = 0; i < tile->variant_count; i++) {
        ctx.variants[i] = tile->variants[i];
    }
    int initial_boundary_count = build_boundary_vertices(base, initial_boundary);
    dfs_vertex_completions(base, &ctx, initial_hidden, initial_hidden_count,
                           initial_boundary, initial_boundary_count, 0,
                           trace_tiles, trace_indices, 0);
}

void enumerate_vertex_completions_mode(const Poly *base,
                                       const Tile *tile,
                                       Coord target,
                                       const Coord *initial_hidden,
                                       int initial_hidden_count,
                                       VCompEmitFn emit,
                                       void *userdata) {
    enumerate_vertex_completions_steps(base, tile, target,
                                       initial_hidden, initial_hidden_count,
                                       -1, emit, userdata);
}

typedef struct {
    int found;
} ProbeCtx;

static void probe_emit(const Poly *p,
                       const Coord *hidden,
                       int hidden_count,
                       void *userdata) {
    ProbeCtx *probe = userdata;
    (void)p;
    (void)hidden;
    (void)hidden_count;
    probe->found = 1;
}

int has_vertex_completion_steps(const Poly *base,
                                const Tile *tile,
                                Coord target,
                                const Coord *initial_hidden,
                                int initial_hidden_count,
                                int required_steps) {
    VCompCtx ctx;
    Coord initial_boundary[MAX_BOUNDARY_VERTS];
    Cycle trace_tiles[MAX_VERTS];
    int trace_indices[MAX_VERTS];
    ProbeCtx probe;
    memset(&ctx, 0, sizeof(ctx));
    memset(&probe, 0, sizeof(probe));
    ctx.tile = tile;
    ctx.target = target;
    ctx.base_hidden_count = initial_hidden_count;
    ctx.required_steps = required_steps;
    ctx.max_steps = required_steps;
    ctx.stop_after_first = 1;
    ctx.stop_flag = &probe.found;
    ctx.emit = probe_emit;
    ctx.userdata = &probe;
    ctx.variant_count = tile->variant_count;
    for (int i = 0; i < tile->variant_count; i++) {
        ctx.variants[i] = tile->variants[i];
    }
    int initial_boundary_count = build_boundary_vertices(base, initial_boundary);
    dfs_vertex_completions(base, &ctx, initial_hidden, initial_hidden_count,
                           initial_boundary, initial_boundary_count, 0,
                           trace_tiles, trace_indices, 0);
    return probe.found;
}

int has_vertex_completion(const Poly *base,
                          const Tile *tile,
                          Coord target,
                          const Coord *initial_hidden,
                          int initial_hidden_count) {
    return has_vertex_completion_steps(base, tile, target,
                                       initial_hidden, initial_hidden_count,
                                       -1);
}

static int target_vertex_valence(const Tile *tile, Coord target) {
    if (tile->lattice == TILE_LATTICE_TETRILLE) {
        if (target.v == 3 || target.v == 4 || target.v == 6) return target.v;
    }
    return lattice_direction_count(tile->lattice);
}

static int has_vertex_completion_bounded(const Poly *base,
                                         const Tile *tile,
                                         Coord target,
                                         const Coord *initial_hidden,
                                         int initial_hidden_count,
                                         int max_steps) {
    VCompCtx ctx;
    Coord initial_boundary[MAX_BOUNDARY_VERTS];
    Cycle trace_tiles[MAX_VERTS];
    int trace_indices[MAX_VERTS];
    ProbeCtx probe;
    memset(&ctx, 0, sizeof(ctx));
    memset(&probe, 0, sizeof(probe));
    ctx.tile = tile;
    ctx.target = target;
    ctx.base_hidden_count = initial_hidden_count;
    ctx.required_steps = -1;
    ctx.max_steps = max_steps;
    ctx.stop_after_first = 1;
    ctx.stop_flag = &probe.found;
    ctx.emit = probe_emit;
    ctx.userdata = &probe;
    ctx.variant_count = tile->variant_count;
    for (int i = 0; i < tile->variant_count; i++) {
        ctx.variants[i] = tile->variants[i];
    }
    int initial_boundary_count = build_boundary_vertices(base, initial_boundary);
    dfs_vertex_completions(base, &ctx, initial_hidden, initial_hidden_count,
                           initial_boundary, initial_boundary_count, 0,
                           trace_tiles, trace_indices, 0);
    return probe.found;
}

int has_vertex_completion_local(const Poly *base,
                                const Tile *tile,
                                Coord target,
                                const Coord *initial_hidden,
                                int initial_hidden_count) {
    int valence = target_vertex_valence(tile, target);
    int max_steps = valence > 0 ? 2 * valence : 8;
    return has_vertex_completion_bounded(base, tile, target,
                                         initial_hidden, initial_hidden_count,
                                         max_steps);
}

void enumerate_vertex_completions(const Poly *base,
                                  const Tile *tile,
                                  Coord target,
                                  const Coord *initial_hidden,
                                  int initial_hidden_count,
                                  VCompEmitFn emit,
                                  void *userdata) {
    enumerate_vertex_completions_mode(base, tile, target,
                                      initial_hidden, initial_hidden_count,
                                      emit, userdata);
}
