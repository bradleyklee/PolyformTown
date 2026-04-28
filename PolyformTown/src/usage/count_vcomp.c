#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "throughput/vcomp_pipeline.h"

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
    int live_only = 0;
    if (argc > 1) max_n = atoi(argv[1]);
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--live-only") == 0) {
            live_only = 1;
            continue;
        }
        tile_path = argv[i];
    }
    if (max_n < 0) max_n = 0;
    if (max_n >= VCOMP_MAX_LEVELS) max_n = VCOMP_MAX_LEVELS - 1;

    Tile tile;
    if (!tile_load(tile_path, &tile)) {
        fprintf(stderr, "failed to load tile: %s\n", tile_path);
        return 1;
    }

    run_vcomp_levels(&tile, max_n, 0, live_only, on_level, NULL);
    return 0;
}
