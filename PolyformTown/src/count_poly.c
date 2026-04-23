#include <stdio.h>
#include <stdlib.h>

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
    if (argc > 1) max_n = atoi(argv[1]);
    if (argc > 2) tile_path = argv[2];
    if (max_n < 1) max_n = 1;
    if (max_n > 31) max_n = 31;

    Tile tile;
    if (!tile_load(tile_path, &tile)) {
        fprintf(stderr, "failed to load tile: %s\n", tile_path);
        return 1;
    }

    run_poly_levels(&tile, max_n, on_level, NULL);
    return 0;
}
