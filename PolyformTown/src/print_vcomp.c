#include <stdio.h>
#include <stdlib.h>

#include "hash.h"
#include "vcomp_pipeline.h"

typedef struct {
    int max_n;
} PrintCtx;

static int on_level(int level,
                    const VCompStateVec *states,
                    size_t unique_poly_count,
                    const Tile *tile,
                    void *userdata) {
    PrintCtx *ctx = userdata;
    (void)unique_poly_count;
    if (level < ctx->max_n) return 1;

    HashTable printed;
    hash_init(&printed, 1024);
    for (size_t i = 0; i < states->count; i++) {
        Poly key;
        vcomp_poly_hash_key(&states->data[i].poly, tile->lattice, &key);
        if (hash_insert(&printed, &key)) {
            tile_print_imgtable_shape(tile, &states->data[i].poly);
        }
    }
    hash_destroy(&printed);
    return 0;
}

int main(int argc, char **argv) {
    int max_n = 5;
    const char *tile_path = "tiles/monomino.tile";
    if (argc > 1) max_n = atoi(argv[1]);
    if (argc > 2) tile_path = argv[2];
    if (max_n < 0) max_n = 0;
    if (max_n >= VCOMP_MAX_LEVELS) max_n = VCOMP_MAX_LEVELS - 1;

    Tile tile;
    if (!tile_load(tile_path, &tile)) {
        fprintf(stderr, "failed to load tile: %s\n", tile_path);
        return 1;
    }

    PrintCtx ctx = { max_n };
    run_vcomp_levels(&tile, max_n, on_level, &ctx);
    return 0;
}
