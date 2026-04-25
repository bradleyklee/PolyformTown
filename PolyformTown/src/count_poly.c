#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "poly_pipeline.h"

static int on_level(int level,
                    const PolyVec *cur,
                    const Tile *tile,
                    void *userdata) {
    (void)tile;
    (void)userdata;
    printf("n=%d count=%zu\n", level, cur->count);
    fflush(stdout);
    return 1;
}

int main(int argc, char **argv) {
    int max_n = 10;
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

    run_poly_levels(&tile, max_n, live_only, on_level, NULL);
    return 0;
}
