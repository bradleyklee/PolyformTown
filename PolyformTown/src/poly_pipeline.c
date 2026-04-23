#include "poly_pipeline.h"

#include "attach.h"
#include "cycle.h"
#include "hash.h"

void run_poly_levels(const Tile *tile,
                     int max_n,
                     PolyLevelFn on_level,
                     void *userdata) {
    Poly seed_raw, seed;
    seed_raw.cycle_count = 1;
    seed_raw.cycles[0] = tile->base;
    poly_canonicalize_lattice(&seed_raw, &seed, tile->lattice);

    PolyVec cur, next;
    vec_init(&cur, 64);
    vec_init(&next, 64);
    vec_push(&cur, &seed);

    for (int level = 1; level <= max_n; level++) {
        if (on_level && !on_level(level, &cur, tile, userdata)) break;
        if (level == max_n) break;

        HashTable seen;
        hash_init(&seen, 1024);
        vec_clear(&next);

        for (size_t i = 0; i < cur.count; i++) {
            Edge frontier[MAX_VERTS * MAX_CYCLES];
            int fc = build_frontier_edges(&cur.data[i], frontier);
            for (int v = 0; v < tile->variant_count; v++) {
                const Cycle *tv = &tile->variants[v];
                for (int be = 0; be < fc; be++) {
                    for (int te = 0; te < tv->n; te++) {
                        Poly grown, canon;
                        if (!try_attach_tile_poly(&cur.data[i],
                                                  tv,
                                                  tile->lattice,
                                                  be,
                                                  te,
                                                  &grown)) {
                            continue;
                        }
                        poly_canonicalize_lattice(&grown,
                                                  &canon,
                                                  tile->lattice);
                        if (hash_insert(&seen, &canon)) vec_push(&next, &canon);
                    }
                }
            }
        }

        hash_destroy(&seen);
        PolyVec tmp = cur;
        cur = next;
        next = tmp;
    }

    vec_destroy(&cur);
    vec_destroy(&next);
}
