#include <stdio.h>
#include <stdlib.h>

#include "vcomp_pipeline.h"

static int on_level(int level,
                    const VCompStateVec *states,
                    size_t unique_poly_count,
                    const Tile *tile,
                    void *userdata) {
    (void)states;
    (void)tile;
    (void)userdata;
    if (level > 0) printf("n=%d count=%zu\n", level, unique_poly_count);
    return 1;
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

    run_vcomp_levels(&tile, max_n, on_level, NULL);
    return 0;
}
