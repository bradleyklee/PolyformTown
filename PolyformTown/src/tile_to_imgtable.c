#include <stdio.h>
#include <stdlib.h>
#include "tile.h"

static void usage(const char *argv0) {
    fprintf(stderr, "usage: %s TILEFILE\n", argv0);
    fprintf(stderr, "prints one explicit imgtable input line for the tile base cycle\n");
}

int main(int argc, char **argv) {
    if (argc != 2) {
        usage(argv[0]);
        return 1;
    }

    Tile tile;
    if (!tile_load(argv[1], &tile)) {
        fprintf(stderr, "failed to load tile: %s\n", argv[1]);
        return 1;
    }

    tile_print_imgtable_cycle(&tile, &tile.base);
    return 0;
}
