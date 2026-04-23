#include <stdio.h>
#include <stdlib.h>
#include "cycle.h"
#include "tile.h"
#include "hash.h"
#include "vec.h"
#include "vcomp.h"

typedef struct {
    HashTable *seen_levels;
    PolyVec *levels;
    int lattice;
    int current_level;
    int max_level;
} EmitCtx;

static void collect_emit(const Poly *p, int steps, void *userdata) {
    EmitCtx *ctx = (EmitCtx *)userdata;
    Poly canon;
    int dst = ctx->current_level + steps;
    if (dst > ctx->max_level) return;
    poly_canonicalize_lattice(p, &canon, ctx->lattice);
    if (hash_insert(&ctx->seen_levels[dst], &canon)) {
        vec_push(&ctx->levels[dst], &canon);
    }
}

int main(int argc, char **argv) {
    int max_n = 5;
    const char *tile_path = "tiles/monomino.tile";
    if (argc > 1) max_n = atoi(argv[1]);
    if (argc > 2) tile_path = argv[2];
    if (max_n < 0) max_n = 0;
    if (max_n > 31) max_n = 31;

    Tile tile;
    if (!tile_load(tile_path, &tile)) {
        fprintf(stderr, "failed to load tile: %s\n", tile_path);
        return 1;
    }

    Poly seed_raw, seed;
    seed_raw.cycle_count = 1;
    seed_raw.cycles[0] = tile.base;
    poly_canonicalize_lattice(&seed_raw, &seed, tile.lattice);

    PolyVec levels[32];
    HashTable seen_levels[32];
    EmitCtx ectx;

    for (int i = 0; i <= max_n; i++) {
        vec_init(&levels[i], 64);
        hash_init(&seen_levels[i], 1024);
    }
    vec_push(&levels[0], &seed);
    hash_insert(&seen_levels[0], &seed);

    ectx.seen_levels = seen_levels;
    ectx.levels = levels;
    ectx.lattice = tile.lattice;
    ectx.max_level = max_n;

    for (int level = 0; level <= max_n; level++) {
        if (level > 0) printf("n=%d count=%zu\n", level, levels[level].count);
        if (level == max_n) break;
        ectx.current_level = level;
        for (size_t i = 0; i < levels[level].count; i++) {
            Coord verts[MAX_VERTS * MAX_CYCLES];
            int vc = build_boundary_vertices(&levels[level].data[i], verts);
            for (int j = 0; j < vc; j++) {
                enumerate_vertex_completions(&levels[level].data[i], &tile,
                                             verts[j], collect_emit, &ectx);
            }
        }
    }

    for (int i = 0; i <= max_n; i++) {
        hash_destroy(&seen_levels[i]);
        vec_destroy(&levels[i]);
    }
    return 0;
}
