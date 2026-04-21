#include "tile.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int lattice_transform_count(int lattice) {
    return (lattice == TILE_LATTICE_TRIANGULAR) ? 12 : 8;
}

int tile_load(const char *path, Tile *tile) {
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    memset(tile, 0, sizeof(*tile));
    tile->lattice = TILE_LATTICE_SQUARE;
    strncpy(tile->name, path, sizeof(tile->name) - 1);
    char line[256];
    int in_cycle = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#') continue;
        if (strncmp(line, "name:", 5) == 0) {
            char *p = line + 5;
            while (*p == ' ' || *p == '\t') p++;
            size_t n = strcspn(p, "\r\n");
            if (n >= sizeof(tile->name)) n = sizeof(tile->name) - 1;
            memcpy(tile->name, p, n);
            tile->name[n] = 0;
            continue;
        }
        if (strncmp(line, "lattice:", 8) == 0) {
            char *p = line + 8;
            while (*p == ' ' || *p == '\t') p++;
            if (strncmp(p, "triangular", 10) == 0) tile->lattice = TILE_LATTICE_TRIANGULAR;
            else tile->lattice = TILE_LATTICE_SQUARE;
            continue;
        }
        if (strncmp(line, "cycle:", 6) == 0) {
            in_cycle = 1;
            continue;
        }
        if (!in_cycle) continue;
        int x, y;
        if (sscanf(line, "%d %d", &x, &y) == 2) {
            if (tile->base.n >= MAX_VERTS) {
                fclose(fp);
                return 0;
            }
            tile->base.v[tile->base.n++] = (Coord){x, y};
        }
    }
    fclose(fp);
    if (tile->base.n < 3) return 0;
    if (cycle_signed_area2(&tile->base) < 0) cycle_reverse(&tile->base);
    tile_build_variants(tile);
    return 1;
}

void tile_build_variants(Tile *tile) {
    tile->variant_count = 0;

    int count = lattice_transform_count(tile->lattice);
    for (int t = 0; t < count; t++) {
        Cycle cur;
        cycle_transform_lattice(&tile->base, &cur, tile->lattice, t);
        if (cycle_signed_area2(&cur) < 0) cycle_reverse(&cur);
        cycle_normalize_position(&cur);
        cycle_canonicalize_shift(&cur);

        int dup = 0;
        for (int i = 0; i < tile->variant_count; i++) {
            if (!cycle_less(&cur, &tile->variants[i]) &&
                !cycle_less(&tile->variants[i], &cur)) {
                dup = 1;
                break;
            }
        }
        if (!dup && tile->variant_count < MAX_VARIANTS) {
            tile->variants[tile->variant_count++] = cur;
        }
    }
}
