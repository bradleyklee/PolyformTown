#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hatseq_pipeline.h"

static int selector_bit_from_token(const char *s) {
    if (strcmp(s, "3") == 0) return 1;
    if (strcmp(s, "4") == 0) return 2;
    if (strcmp(s, "6") == 0) return 4;
    return 0;
}

static int looks_like_selector_token(const char *s) {
    return selector_bit_from_token(s) != 0;
}

static int add_selector_token(const char *s, int *mask) {
    int bit = selector_bit_from_token(s);
    if (!bit) return 0;
    *mask |= bit;
    return 1;
}

static int on_level(int level,
                    const VCompStateVec *states,
                    size_t unique_poly_count,
                    const Tile *tile,
                    void *userdata) {
    (void)states;
    (void)tile;
    (void)userdata;
    if (level > 0) {
        printf("n=%d count=%zu\n", level, unique_poly_count);
    }
    return 1;
}

int main(int argc, char **argv) {
    int max_n = 5;
    const char *tile_path = "tiles/hat.tile";
    int selector_mask = 1 | 2 | 4;
    int live_boundary = 0;
    int strict_three = 0;
    int saw_tile = 0;
    int selector_given = 0;

    if (argc > 1) max_n = atoi(argv[1]);
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--live-boundary") == 0) {
            live_boundary = 1;
            continue;
        }
        if (strcmp(argv[i], "--check-boundary") == 0) {
            live_boundary = 1;
            continue;
        }
        if (strcmp(argv[i], "--double-check") == 0) {
            live_boundary = 1;
            continue;
        }
        if (strcmp(argv[i], "--strict-three") == 0) {
            strict_three = 1;
            continue;
        }
        if (!saw_tile && !looks_like_selector_token(argv[i])) {
            tile_path = argv[i];
            saw_tile = 1;
            continue;
        }
        if (!selector_given) selector_mask = 0;
        if (!add_selector_token(argv[i], &selector_mask)) {
            fprintf(stderr, "invalid selector token: %s\n", argv[i]);
            return 1;
        }
        selector_given = 1;
    }
    if (max_n < 0) max_n = 0;
    if (max_n >= VCOMP_MAX_LEVELS) max_n = VCOMP_MAX_LEVELS - 1;

    Tile tile;
    if (!tile_load(tile_path, &tile)) {
        fprintf(stderr, "failed to load tile: %s\n", tile_path);
        return 1;
    }

    run_hatseq_levels(&tile, max_n, selector_mask, live_boundary, strict_three,
                      on_level, NULL);
    return 0;
}
