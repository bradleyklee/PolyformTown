#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "attach.h"
#include "hash.h"
#include "tile.h"
#include "vcomp.h"

#define MAX_SEED_EDGES MAX_VERTS
#define MAX_RESULT_TILES MAX_VERTS

typedef struct {
    Poly aggregate;
    Cycle tiles[MAX_RESULT_TILES];
    int tile_count;
} SurroundResult;

typedef struct {
    const Tile *tile;
    Edge seed_edges[MAX_SEED_EDGES];
    int seed_edge_count;
    int live_boundary;
    HashTable seen;
    HashTable explored;
    SurroundResult *results;
    int result_count;
    int result_cap;
} SurroundCtx;

static int edge_eq(Edge a, Edge b) {
    return coord_eq(a.a, b.a) && coord_eq(a.b, b.b);
}

static int edge_same_segment(Edge a, Edge b) {
    return edge_eq(a, b) || edge_eq(a, (Edge){b.b, b.a});
}

static int edge_in_frontier(const Edge *frontier, int fc, Edge e) {
    for (int i = 0; i < fc; i++) {
        if (edge_same_segment(frontier[i], e)) return 1;
    }
    return 0;
}

static int count_exposed_seed_edges(const SurroundCtx *ctx, const Poly *p) {
    Edge frontier[MAX_VERTS * MAX_CYCLES];
    int fc = build_frontier_edges(p, frontier);
    int exposed = 0;
    for (int i = 0; i < ctx->seed_edge_count; i++) {
        if (edge_in_frontier(frontier, fc, ctx->seed_edges[i])) exposed++;
    }
    return exposed;
}

static int edge_covers_exposed_seed(const SurroundCtx *ctx,
                                    const Poly *p,
                                    int frontier_edge_index) {
    Edge frontier[MAX_VERTS * MAX_CYCLES];
    int fc = build_frontier_edges(p, frontier);
    if (frontier_edge_index < 0 || frontier_edge_index >= fc) return 0;
    for (int i = 0; i < ctx->seed_edge_count; i++) {
        if (!edge_in_frontier(frontier, fc, ctx->seed_edges[i])) continue;
        if (edge_same_segment(frontier[frontier_edge_index],
                              ctx->seed_edges[i])) {
            return 1;
        }
    }
    return 0;
}

static void save_result(SurroundCtx *ctx,
                        const Poly *p,
                        const Cycle *tiles,
                        int tile_count) {
    Poly key;
    poly_hash_key_lattice(p, ctx->tile->lattice, &key);
    if (!hash_insert(&ctx->seen, &key)) return;

    if (ctx->result_count == ctx->result_cap) {
        int nc = ctx->result_cap ? 2 * ctx->result_cap : 32;
        SurroundResult *nr = realloc(ctx->results, (size_t)nc * sizeof(*nr));
        if (!nr) {
            fprintf(stderr, "oom\n");
            exit(1);
        }
        ctx->results = nr;
        ctx->result_cap = nc;
    }

    ctx->results[ctx->result_count].aggregate = *p;
    ctx->results[ctx->result_count].tile_count = tile_count;
    for (int i = 0; i < tile_count && i < MAX_RESULT_TILES; i++) {
        ctx->results[ctx->result_count].tiles[i] = tiles[i];
    }
    ctx->result_count++;
}

static void dfs_surround(SurroundCtx *ctx,
                         const Poly *p,
                         int exposed,
                         const Cycle *tiles,
                         int tile_count) {
    Poly key;

    poly_hash_key_lattice(p, ctx->tile->lattice, &key);
    if (!hash_insert(&ctx->explored, &key)) return;

    if (exposed == 0) {
        if (ctx->live_boundary && !poly_has_live_boundary(p, ctx->tile)) {
            return;
        }
        save_result(ctx, p, tiles, tile_count);
        return;
    }

    Edge frontier[MAX_VERTS * MAX_CYCLES];
    int fc = build_frontier_edges(p, frontier);

    for (int be = 0; be < fc; be++) {
        if (!edge_covers_exposed_seed(ctx, p, be)) continue;
        for (int v = 0; v < ctx->tile->variant_count; v++) {
            const Cycle *variant = &ctx->tile->variants[v];
            for (int te = 0; te < variant->n; te++) {
                Poly grown;
                Cycle aligned;
                Cycle next_tiles[MAX_RESULT_TILES];
                int next_exposed;
                int next_tile_count;
                if (tile_count >= MAX_RESULT_TILES) continue;
                if (!try_attach_tile_poly_ex(p,
                                             variant,
                                             ctx->tile->lattice,
                                             be,
                                             te,
                                             &grown,
                                             &aligned)) {
                    continue;
                }
                next_exposed = count_exposed_seed_edges(ctx, &grown);
                if (next_exposed >= exposed) continue;

                next_tile_count = tile_count;
                for (int ti = 0; ti < tile_count; ti++) {
                    next_tiles[ti] = tiles[ti];
                }
                next_tiles[next_tile_count++] = aligned;
                dfs_surround(ctx,
                             &grown,
                             next_exposed,
                             next_tiles,
                             next_tile_count);
            }
        }
    }
}

int main(int argc, char **argv) {
    const char *tile_path = "tiles/hat.tile";
    int live_boundary = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--live-boundary") == 0 ||
            strcmp(argv[i], "--check-boundary") == 0 ||
            strcmp(argv[i], "--double-check") == 0) {
            live_boundary = 1;
            continue;
        }
        tile_path = argv[i];
    }

    Tile tile;
    if (!tile_load(tile_path, &tile)) {
        fprintf(stderr, "failed to load tile: %s\n", tile_path);
        return 1;
    }

    SurroundCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.tile = &tile;
    ctx.live_boundary = live_boundary;
    ctx.seed_edge_count = tile.base.n;
    if (ctx.seed_edge_count > MAX_SEED_EDGES) {
        fprintf(stderr, "seed edge count exceeds MAX_SEED_EDGES\n");
        return 1;
    }
    for (int i = 0; i < tile.base.n; i++) {
        ctx.seed_edges[i] = cycle_edge(&tile.base, i);
    }
    hash_init(&ctx.seen, 4096);
    hash_init(&ctx.explored, 65536);

    Poly seed;
    Cycle seed_tiles[MAX_RESULT_TILES];
    seed.cycle_count = 1;
    seed.cycles[0] = tile.base;
    seed_tiles[0] = tile.base;
    dfs_surround(&ctx, &seed, ctx.seed_edge_count, seed_tiles, 1);

    for (int i = 0; i < ctx.result_count; i++) {
        printf("[%d]\n", i + 1);
        printf("Aggregate\n");
        tile_print_imgtable_shape(&tile, &ctx.results[i].aggregate);
        printf("Tiles\n");
        for (int ti = 0; ti < ctx.results[i].tile_count; ti++) {
            tile_print_imgtable_cycle(&tile, &ctx.results[i].tiles[ti]);
        }
    }
    printf("count=%d\n", ctx.result_count);

    hash_destroy(&ctx.seen);
    hash_destroy(&ctx.explored);
    free(ctx.results);
    return 0;
}
