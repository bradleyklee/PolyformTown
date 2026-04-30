#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "throughput/vcomp_pipeline.h"

typedef struct {
    size_t counts[VCOMP_MAX_LEVELS];
} CountRows;

static int collect_counts(int level,
                          const VCompStateVec *states,
                          size_t unique_poly_count,
                          const Tile *tile,
                          void *userdata) {
    CountRows *rows = userdata;
    (void)states;
    (void)tile;
    if (level >= 0 && level < VCOMP_MAX_LEVELS) {
        rows->counts[level] = unique_poly_count;
    }
    return 1;
}

static int run_case(const char *path,
                    int max_n,
                    const size_t *expected) {
    Tile tile;
    CountRows off;
    CountRows on;

    memset(&off, 0, sizeof(off));
    memset(&on, 0, sizeof(on));

    if (!tile_load(path, &tile)) {
        fprintf(stderr, "failed to load tile: %s\n", path);
        return 1;
    }

    run_vcomp_levels(&tile, max_n, 0, 0, collect_counts, &off);
    run_vcomp_levels(&tile, max_n, 1, 0, collect_counts, &on);

    for (int n = 1; n <= max_n; n++) {
        if (off.counts[n] != on.counts[n]) {
            fprintf(stderr,
                    "track parity mismatch %s n=%d off=%zu on=%zu\n",
                    path,
                    n,
                    off.counts[n],
                    on.counts[n]);
            return 1;
        }
        if (expected && off.counts[n] != expected[n - 1]) {
            fprintf(stderr,
                    "unexpected count %s n=%d got=%zu expected=%zu\n",
                    path,
                    n,
                    off.counts[n],
                    expected[n - 1]);
            return 1;
        }
    }

    printf("PASS parity %s\n", path);
    return 0;
}

int main(void) {
    static const size_t domino[] = {4, 9, 29};
    static const size_t chair[] = {10, 25};
    static const size_t hat[] = {9, 6, 3, 13};
    int failed = 0;

    failed |= run_case("tiles/domino.tile", 3, domino);
    failed |= run_case("tiles/chair.tile", 2, chair);
    failed |= run_case("tiles/hat.tile", 4, hat);

    return failed ? 1 : 0;
}
