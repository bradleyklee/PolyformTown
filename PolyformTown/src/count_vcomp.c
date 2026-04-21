#include <stdio.h>
#include <stdlib.h>
#include "cycle.h"
#include "tile.h"
#include "hash.h"
#include "vec.h"
#include "vcomp.h"

typedef struct {
    HashTable *seen;
    PolyVec *next;
    int lattice;
} EmitCtx;

static void collect_emit(const Poly *p, void *userdata) {
    EmitCtx *ctx = (EmitCtx *)userdata;
    Poly canon;
    poly_canonicalize_lattice(p, &canon, ctx->lattice);
    if (hash_insert(ctx->seen, &canon)) vec_push(ctx->next, &canon);
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

    PolyVec cur, next;
    vec_init(&cur, 64);
    vec_init(&next, 64);
    vec_push(&cur, &seed);

    for (int level = 0; level <= max_n; level++) {
        printf("n=%d count=%zu\n", level, cur.count);
        fflush(stdout);
        if (level == max_n) break;

        HashTable seen;
        EmitCtx ectx;
        hash_init(&seen, 1024);
        vec_clear(&next);
        ectx.seen = &seen;
        ectx.next = &next;
        ectx.lattice = tile.lattice;

        for (size_t i = 0; i < cur.count; i++) {
            Coord verts[MAX_VERTS * MAX_CYCLES];
            int vc = build_boundary_vertices(&cur.data[i], verts);
            for (int j = 0; j < vc; j++) {
                enumerate_vertex_completions(&cur.data[i], &tile, verts[j], collect_emit, &ectx);
            }
        }

        hash_destroy(&seen);
        PolyVec tmp = cur; cur = next; next = tmp;
    }

    vec_destroy(&cur);
    vec_destroy(&next);
    return 0;
}
