#include <stdio.h>
#include <stdlib.h>
#include "cycle.h"
#include "tile.h"
#include "attach.h"
#include "hash.h"
#include "vec.h"

int main(int argc, char **argv) {
    int max_n = 10;
    const char *tile_path = "tiles/monomino.tile";
    if (argc > 1) max_n = atoi(argv[1]);
    if (argc > 2) tile_path = argv[2];
    if (max_n < 1) max_n = 1;
    if (max_n > 31) max_n = 31;

    Tile tile;
    if (!tile_load(tile_path, &tile)) {
        fprintf(stderr, "failed to load tile: %s\n", tile_path);
        return 1;
    }

    Poly seed_raw, seed;
    seed_raw.cycle_count = 1;
    seed_raw.cycles[0] = tile.base;
    poly_canonicalize(&seed_raw, &seed);

    PolyVec cur, next;
    vec_init(&cur, 64);
    vec_init(&next, 64);
    vec_push(&cur, &seed);

    for (int level = 1; level <= max_n; level++) {
        printf("n=%d count=%zu\n", level, cur.count);
        fflush(stdout);
        if (level == max_n) break;

        HashTable seen;
        hash_init(&seen, 1024);
        vec_clear(&next);

        for (size_t i = 0; i < cur.count; i++) {
            Edge frontier[MAX_VERTS * MAX_CYCLES];
            int fc = build_frontier_edges(&cur.data[i], frontier);
            for (int v = 0; v < tile.variant_count; v++) {
                const Cycle *tv = &tile.variants[v];
                for (int be = 0; be < fc; be++) {
                    for (int te = 0; te < tv->n; te++) {
                        Poly grown, canon;
                        if (!try_attach_tile_poly(&cur.data[i], tv, be, te, &grown)) continue;
                        poly_canonicalize(&grown, &canon);
                        if (hash_insert(&seen, &canon)) vec_push(&next, &canon);
                    }
                }
            }
        }

        hash_destroy(&seen);
        PolyVec tmp = cur; cur = next; next = tmp;
    }

    vec_destroy(&cur);
    vec_destroy(&next);
    return 0;
}
