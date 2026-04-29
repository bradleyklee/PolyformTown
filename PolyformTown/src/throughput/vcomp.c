#include "vcomp.h"

#include <stdlib.h>
#include <string.h>

#include "core/attach.h"
#include "core/boundary.h"
#include "core/hash.h"
#include "core/lattice.h"
#include "core/tetrille.h"

#define MAX_BOUNDARY_VERTS (MAX_VERTS * MAX_CYCLES)

typedef struct {
    const Tile *tile;
    Coord target;
    int base_hidden_count;
    int max_hidden;
    int track_tiles;
    HashTable *seen;
    VCompLevels *out;
} VCompCtx;

static void raw_vec_push(VCompRawVec *v, const VCompRawState *s) {
    if (v->count == v->cap) {
        size_t nc = v->cap ? 2 * v->cap : 64;
        v->data = realloc(v->data, nc * sizeof(*v->data));
        if (!v->data) {
            exit(1);
        }
        v->cap = nc;
    }
    v->data[v->count++] = *s;
}

void vcomp_levels_init(VCompLevels *out, int max_level) {
    memset(out, 0, sizeof(*out));
    if (max_level < 0) max_level = 0;
    if (max_level >= VCOMP_MAX_LEVELS) max_level = VCOMP_MAX_LEVELS - 1;
    out->max_level = max_level;
}

void vcomp_levels_destroy(VCompLevels *out) {
    for (int i = 0; i <= out->max_level; i++) {
        free(out->levels[i].data);
        out->levels[i].data = NULL;
        out->levels[i].count = 0;
        out->levels[i].cap = 0;
    }
}

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

static int edge_in_tiles(const Cycle *tiles,
                         int tile_count,
                         Coord a,
                         Coord b) {
    for (int i = 0; i < tile_count; i++) {
        const Cycle *c = &tiles[i];
        for (int j = 0; j < c->n; j++) {
            Coord u = c->v[j];
            Coord v = c->v[(j + 1) % c->n];
            if ((coord_eq(u, a) && coord_eq(v, b)) ||
                (coord_eq(u, b) && coord_eq(v, a))) {
                return 1;
            }
        }
    }
    return 0;
}

static int hidden_port_connected(const Coord *hidden,
                                 int hidden_count,
                                 const Coord *ports,
                                 int port_count,
                                 const Cycle *tiles,
                                 int tile_count) {
    int seen[MAX_BOUNDARY_VERTS] = {0};
    int queue[MAX_BOUNDARY_VERTS];
    int qh = 0;
    int qt = 0;
    int touches_port = 0;

    if (hidden_count <= 1) return 1;
    if (hidden_count > MAX_BOUNDARY_VERTS) return 0;
    if (port_count <= 0 || tile_count <= 0) return 0;

    seen[0] = 1;
    queue[qt++] = 0;
    while (qh < qt) {
        int cur = queue[qh++];
        for (int p = 0; p < port_count; p++) {
            if (edge_in_tiles(tiles,
                              tile_count,
                              hidden[cur],
                              ports[p])) {
                touches_port = 1;
                break;
            }
        }
        for (int i = 0; i < hidden_count; i++) {
            if (seen[i]) continue;
            if (!edge_in_tiles(tiles,
                               tile_count,
                               hidden[cur],
                               hidden[i])) {
                continue;
            }
            seen[i] = 1;
            queue[qt++] = i;
        }
    }

    if (!touches_port) return 0;
    for (int i = 0; i < hidden_count; i++) {
        if (!seen[i]) return 0;
    }
    return 1;
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
        if (!point_on_poly_boundary(grown, v, lattice) &&
            !coord_in_list(out_hidden, out_count, v)) {
            if (out_count >= MAX_BOUNDARY_VERTS) return -1;
            out_hidden[out_count++] = v;
        }
    }
    return out_count;
}

static int floor_div6_local(long long a) {
    if (a >= 0) return (int)(a / 6LL);
    return (int)(-(((-a) + 5LL) / 6LL));
}

static int cycle_abs_area_cmp_desc(const Cycle *a, const Cycle *b, int lattice) {
    long long aa = cycle_signed_area2(a, lattice);
    long long ab = cycle_signed_area2(b, lattice);
    if (aa < 0) aa = -aa;
    if (ab < 0) ab = -ab;
    if (aa != ab) return aa > ab;
    return cycle_less(a, b);
}

static void prepare_poly_cycles_local(Poly *p, int lattice) {
    int outer = 0;
    for (int i = 1; i < p->cycle_count; i++) {
        if (cycle_abs_area_cmp_desc(&p->cycles[i], &p->cycles[outer], lattice)) {
            outer = i;
        }
    }
    if (outer != 0) {
        Cycle tmp = p->cycles[0];
        p->cycles[0] = p->cycles[outer];
        p->cycles[outer] = tmp;
    }

    for (int i = 0; i < p->cycle_count; i++) {
        long long area = cycle_signed_area2(&p->cycles[i], lattice);
        if (i == 0) {
            if (area < 0) cycle_reverse(&p->cycles[i]);
        } else {
            if (area > 0) cycle_reverse(&p->cycles[i]);
        }
        cycle_canonicalize_shift(&p->cycles[i]);
    }

    for (int i = 1; i < p->cycle_count; i++) {
        for (int j = i + 1; j < p->cycle_count; j++) {
            if (cycle_less(&p->cycles[j], &p->cycles[i])) {
                Cycle tmp = p->cycles[i];
                p->cycles[i] = p->cycles[j];
                p->cycles[j] = tmp;
            }
        }
    }
}

static Coord coord_transform_lattice(Coord q, int lattice, int t) {
    Cycle in = {0};
    Cycle out = {0};
    in.n = 1;
    in.v[0] = q;
    cycle_transform_lattice(&in, &out, lattice, t);
    return out.v[0];
}

static Coord coord_translate_lattice(Coord q, int lattice, int dx, int dy) {
    Cycle c = {0};
    c.n = 1;
    c.v[0] = q;
    if (lattice == TILE_LATTICE_TETRILLE) {
        tetrille_translate_cycle(&c, dx, dy);
    } else {
        cycle_translate(&c, dx, dy);
    }
    return c.v[0];
}

static void normalize_payload(Poly *p,
                              Coord *target,
                              Coord *hidden,
                              int hidden_count,
                              Cycle *tiles,
                              int tile_count,
                              int lattice) {
    if (lattice == TILE_LATTICE_TETRILLE) {
        long long minx = 0;
        long long miny = 0;
        int first = 1;
        for (int i = 0; i < p->cycle_count; i++) {
            for (int j = 0; j < p->cycles[i].n; j++) {
                long long x, y;
                tetrille_embed_point_scaled(p->cycles[i].v[j], &x, &y);
                if (first || x < minx) minx = x;
                if (first || y < miny) miny = y;
                first = 0;
            }
        }
        int tx = -floor_div6_local(minx);
        int ty = -floor_div6_local(miny);
        for (int i = 0; i < p->cycle_count; i++) {
            tetrille_translate_cycle(&p->cycles[i], tx, ty);
        }
        *target = coord_translate_lattice(*target, lattice, tx, ty);
        for (int i = 0; i < hidden_count; i++) {
            hidden[i] = coord_translate_lattice(hidden[i], lattice, tx, ty);
        }
        for (int i = 0; i < tile_count; i++) {
            tetrille_translate_cycle(&tiles[i], tx, ty);
        }
        return;
    }

    int minx = p->cycles[0].v[0].x;
    int miny = p->cycles[0].v[0].y;
    for (int i = 0; i < p->cycle_count; i++) {
        for (int j = 0; j < p->cycles[i].n; j++) {
            if (p->cycles[i].v[j].x < minx) minx = p->cycles[i].v[j].x;
            if (p->cycles[i].v[j].y < miny) miny = p->cycles[i].v[j].y;
        }
    }
    for (int i = 0; i < p->cycle_count; i++) {
        cycle_translate(&p->cycles[i], -minx, -miny);
    }
    *target = coord_translate_lattice(*target, lattice, -minx, -miny);
    for (int i = 0; i < hidden_count; i++) {
        hidden[i] = coord_translate_lattice(hidden[i], lattice, -minx, -miny);
    }
    for (int i = 0; i < tile_count; i++) {
        cycle_translate(&tiles[i], -minx, -miny);
    }
}

static int coord_cmp(const void *A, const void *B) {
    const Coord *a = A;
    const Coord *b = B;
    if (a->v != b->v) return a->v - b->v;
    if (a->x != b->x) return a->x - b->x;
    return a->y - b->y;
}

static void canonicalize_result(VCompRawState *s, int lattice) {
    int tcount = lattice_transform_count(lattice);
    VCompRawState best = {0};
    int first = 1;

    for (int t = 0; t < tcount; t++) {
        VCompRawState cur = {0};
        int unique_hidden = 0;

        poly_transform_lattice(&s->poly, &cur.poly, lattice, t);
        cur.target = coord_transform_lattice(s->target, lattice, t);
        cur.hidden_count = s->hidden_count;
        for (int i = 0; i < s->hidden_count; i++) {
            cur.hidden[i] = coord_transform_lattice(s->hidden[i], lattice, t);
        }
        cur.tile_count = s->tile_count;
        for (int i = 0; i < s->tile_count; i++) {
            cycle_transform_lattice(&s->tiles[i], &cur.tiles[i], lattice, t);
        }

        normalize_payload(&cur.poly,
                          &cur.target,
                          cur.hidden,
                          cur.hidden_count,
                          cur.tiles,
                          cur.tile_count,
                          lattice);
        prepare_poly_cycles_local(&cur.poly, lattice);

        qsort(cur.hidden, (size_t)cur.hidden_count, sizeof(Coord), coord_cmp);
        for (int i = 0; i < cur.hidden_count; i++) {
            if (i == 0 || !coord_eq(cur.hidden[i], cur.hidden[i - 1])) {
                cur.hidden[unique_hidden++] = cur.hidden[i];
            }
        }
        cur.hidden_count = unique_hidden;

        if (first || poly_less(&cur.poly, &best.poly)) {
            best = cur;
            first = 0;
        }
    }
    *s = best;
}

static void dfs_levels(const Poly *p,
                       const VCompCtx *ctx,
                       const Coord *hidden,
                       int hidden_count,
                       const Coord *prev_boundary,
                       int prev_boundary_count,
                       Cycle *trace_tiles,
                       int trace_tile_count) {
    Edge frontier[MAX_VERTS * MAX_CYCLES];
    int fc = build_boundary_edges(p, frontier);

    if (hidden_count >= ctx->max_hidden) return;

    for (int be = 0; be < fc; be++) {
        if (!coord_eq(frontier[be].b, ctx->target)) continue;

        for (int v = 0; v < ctx->tile->variant_count; v++) {
            const Cycle *tv = &ctx->tile->variants[v];
            for (int te = 0; te < tv->n; te++) {
                Poly grown;
                Cycle aligned;
                Coord next_hidden[MAX_BOUNDARY_VERTS];
                Coord grown_boundary[MAX_BOUNDARY_VERTS];
                int next_hidden_count;
                int grown_boundary_count;
                int target_present;

                if (!try_attach_tile_poly_ex(p,
                                             tv,
                                             ctx->tile->lattice,
                                             be,
                                             te,
                                             &grown,
                                             &aligned)) {
                    continue;
                }

                grown_boundary_count = build_boundary_vertices(&grown, grown_boundary);
                next_hidden_count = build_next_hidden(prev_boundary,
                                                      prev_boundary_count,
                                                      &grown,
                                                      hidden,
                                                      hidden_count,
                                                      ctx->tile->lattice,
                                                      next_hidden);
                if (grown_boundary_count < 0 || next_hidden_count < 0) continue;
                if (next_hidden_count > ctx->max_hidden) continue;

                target_present = point_on_poly_boundary(&grown,
                                                        ctx->target,
                                                        ctx->tile->lattice);
                if (!target_present) {
                    if (next_hidden_count > ctx->base_hidden_count) {
                        VCompRawState state;
                        Coord ports[MAX_BOUNDARY_VERTS];
                        int port_count = 0;
                        memset(&state, 0, sizeof(state));
                        state.poly = grown;
                        state.target = ctx->target;
                        state.hidden_count = next_hidden_count;
                        for (int i = 0; i < next_hidden_count; i++) {
                            state.hidden[i] = next_hidden[i];
                        }

                        if (ctx->track_tiles) {
                            for (int i = 0; i < trace_tile_count; i++) {
                                state.tiles[i] = trace_tiles[i];
                            }
                            state.tiles[trace_tile_count] = aligned;
                            state.tile_count = trace_tile_count + 1;

                            for (int i = 0; i < grown_boundary_count; i++) {
                                Coord q = grown_boundary[i];
                                if (coord_in_list(next_hidden,
                                                  next_hidden_count,
                                                  q)) {
                                    continue;
                                }
                                if (!coord_in_list(ports, port_count, q)) {
                                    ports[port_count++] = q;
                                }
                            }
                            if (!hidden_port_connected(state.hidden,
                                                       state.hidden_count,
                                                       ports,
                                                       port_count,
                                                       state.tiles,
                                                       state.tile_count)) {
                                continue;
                            }
                        }

                        canonicalize_result(&state, ctx->tile->lattice);
                        if (state.hidden_count > ctx->out->max_level) continue;
                        if (hash_insert(ctx->seen, &state.poly)) {
                            raw_vec_push(&ctx->out->levels[state.hidden_count], &state);
                        }
                    }
                    continue;
                }

                if (ctx->track_tiles && trace_tile_count < VCOMP_MAX_TRACE) {
                    trace_tiles[trace_tile_count] = aligned;
                    dfs_levels(&grown,
                               ctx,
                               next_hidden,
                               next_hidden_count,
                               grown_boundary,
                               grown_boundary_count,
                               trace_tiles,
                               trace_tile_count + 1);
                } else if (!ctx->track_tiles) {
                    dfs_levels(&grown,
                               ctx,
                               next_hidden,
                               next_hidden_count,
                               grown_boundary,
                               grown_boundary_count,
                               trace_tiles,
                               trace_tile_count);
                }
            }
        }
    }
}

void vcomp_enumerate_levels(const Poly *base,
                            const Tile *tile,
                            Coord target,
                            const Coord *initial_hidden,
                            int initial_hidden_count,
                            const Cycle *initial_tiles,
                            int initial_tile_count,
                            int max_hidden,
                            int track_tiles,
                            VCompLevels *out) {
    VCompCtx ctx;
    Coord initial_boundary[MAX_BOUNDARY_VERTS];
    Cycle trace_tiles[VCOMP_MAX_TRACE];
    int start_tiles = 0;
    int initial_boundary_count;
    HashTable seen;

    if (!base || !tile || !out) return;
    if (max_hidden < 0) return;
    if (max_hidden > out->max_level) max_hidden = out->max_level;
    if (initial_hidden_count < 0 || initial_hidden_count > VCOMP_MAX_HIDDEN) {
        return;
    }
    if (initial_tile_count < 0 || initial_tile_count > VCOMP_MAX_TRACE) {
        return;
    }

    memset(&ctx, 0, sizeof(ctx));
    ctx.tile = tile;
    ctx.target = target;
    ctx.base_hidden_count = initial_hidden_count;
    ctx.max_hidden = max_hidden;
    ctx.track_tiles = track_tiles;
    ctx.seen = &seen;
    ctx.out = out;
    hash_init(&seen, 1024);

    initial_boundary_count = build_boundary_vertices(base, initial_boundary);
    if (initial_boundary_count < 0) {
        hash_destroy(&seen);
        return;
    }

    if (track_tiles) {
        if (initial_tiles && initial_tile_count > 0) {
            for (int i = 0; i < initial_tile_count; i++) {
                trace_tiles[i] = initial_tiles[i];
            }
            start_tiles = initial_tile_count;
        } else {
            trace_tiles[0] = ctx.tile->base;
            start_tiles = 1;
        }
    }

    dfs_levels(base,
               &ctx,
               initial_hidden,
               initial_hidden_count,
               initial_boundary,
               initial_boundary_count,
               trace_tiles,
               start_tiles);
    hash_destroy(&seen);
}
