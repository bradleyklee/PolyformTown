#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hash.h"
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

typedef struct {
    int max_n;
    int detailed_mode;
} PrintCtx;

static int poly_equal_local(const Poly *a, const Poly *b) {
    if (a->cycle_count != b->cycle_count) return 0;
    for (int c = 0; c < a->cycle_count; c++) {
        const Cycle *ca = &a->cycles[c];
        const Cycle *cb = &b->cycles[c];
        if (ca->n != cb->n) return 0;
        for (int i = 0; i < ca->n; i++) {
            if (!coord_eq(ca->v[i], cb->v[i])) return 0;
        }
    }
    return 1;
}

static int on_level(int level,
                    const VCompStateVec *states,
                    size_t unique_poly_count,
                    const Tile *tile,
                    void *userdata) {
    PrintCtx *ctx = userdata;
    (void)unique_poly_count;
    if (level < ctx->max_n) return 1;

    if (!ctx->detailed_mode) {
        HashTable printed;
        hash_init(&printed, 1024);
        for (size_t i = 0; i < states->count; i++) {
            Poly key;
            poly_hash_key_lattice(&states->data[i].poly, tile->lattice, &key);
            if (hash_insert(&printed, &key)) {
                tile_print_imgtable_shape(tile, &states->data[i].poly);
            }
        }
        hash_destroy(&printed);
        return 0;
    }

    Poly *keys = malloc(states->count * sizeof(*keys));
    if (!keys) {
        fprintf(stderr, "oom\n");
        return 0;
    }
    for (size_t i = 0; i < states->count; i++) {
        poly_hash_key_lattice(&states->data[i].poly, tile->lattice, &keys[i]);
    }

    int res = 0;
    for (size_t i = 0; i < states->count; i++) {
        int first = 1;
        for (size_t k = 0; k < i; k++) {
            if (poly_equal_local(&keys[k], &keys[i])) {
                first = 0;
                break;
            }
        }
        if (!first) continue;
        res++;
        printf("[%d]\n", res);
        printf("Aggregate\n");
        tile_print_imgtable_shape(tile, &states->data[i].poly);
        printf("Tiles\n");
        for (int ti = 0; ti < states->data[i].tile_count; ti++) {
            tile_print_imgtable_cycle(tile, &states->data[i].tiles[ti]);
        }
    }
    free(keys);
    return 0;
}

int main(int argc, char **argv) {
    int max_n = 5;
    const char *tile_path = "tiles/hat.tile";
    int selector_mask = 1 | 2 | 4;
    int live_boundary = 0;
    int strict_three = 0;
    int saw_tile = 0;
    int selector_given = 0;
    PrintCtx ctx = {0};
    ctx.max_n = max_n;

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
        if (strcmp(argv[i], "--detailed") == 0) {
            ctx.detailed_mode = 1;
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
    ctx.max_n = max_n;

    Tile tile;
    if (!tile_load(tile_path, &tile)) {
        fprintf(stderr, "failed to load tile: %s\n", tile_path);
        return 1;
    }

    run_hatseq_levels(&tile, max_n, selector_mask,
                      live_boundary, strict_three,
                      on_level, &ctx);
    return 0;
}
