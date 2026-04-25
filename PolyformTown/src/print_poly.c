#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "poly_pipeline.h"

typedef struct {
    int max_n;
} PrintCtx;

static int on_level(int level,
                    const PolyVec *cur,
                    const Tile *tile,
                    void *userdata) {
    PrintCtx *ctx = userdata;
    if (level < ctx->max_n) return 1;

    for (size_t i = 0; i < cur->count; i++) {
        tile_print_imgtable_shape(tile, &cur->data[i]);
    }
    return 0;
}

int main(int argc, char **argv) {
    int max_n = 5;
    const char *tile_path = "tiles/monomino.tile";
    int live_only = 0;
    if (argc > 1) max_n = atoi(argv[1]);
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--live-only") == 0) {
            live_only = 1;
            continue;
        }
        tile_path = argv[i];
    }
    if (max_n < 1) max_n = 1;
    if (max_n > 31) max_n = 31;

    Tile tile;
    if (!tile_load(tile_path, &tile)) {
        fprintf(stderr, "failed to load tile: %s\n", tile_path);
        return 1;
    }

    PrintCtx ctx = { max_n };
    run_poly_levels(&tile, max_n, live_only, on_level, &ctx);
    return 0;
}
