#include "tile.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int tile_load(const char *path, Tile *tile) {
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    memset(tile, 0, sizeof(*tile));
    strncpy(tile->name, path, sizeof(tile->name)-1);
    char line[256];
    int in_cycle = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#') continue;
        if (strncmp(line, "name:", 5) == 0) {
            char *p = line + 5;
            while (*p == ' ' || *p == '\t') p++;
            size_t n = strcspn(p, "\r\n");
            if (n >= sizeof(tile->name)) n = sizeof(tile->name)-1;
            memcpy(tile->name, p, n);
            tile->name[n] = 0;
            continue;
        }
        if (strncmp(line, "cycle:", 6) == 0) {
            in_cycle = 1;
            continue;
        }
        if (!in_cycle) continue;
        int x, y;
        if (sscanf(line, "%d %d", &x, &y) == 2) {
            if (tile->base.n >= MAX_VERTS) { fclose(fp); return 0; }
            tile->base.v[tile->base.n++] = (Coord){x, y};
        }
    }
    fclose(fp);
    if (tile->base.n < 4) return 0;
    if (cycle_signed_area2(&tile->base) < 0) cycle_reverse(&tile->base);
    tile_build_variants(tile);
    return 1;
}

void tile_build_variants(Tile *tile) {
    tile->variant_count = 0;
    for (int t = 0; t < 8; t++) {
        Cycle c, canon;
        cycle_transform(&tile->base, &c, t);
        if (cycle_signed_area2(&c) < 0) cycle_reverse(&c);
        cycle_canonicalize(&c, &canon);
        int dup = 0;
        for (int i = 0; i < tile->variant_count; i++) {
            if (!cycle_less(&canon, &tile->variants[i]) && !cycle_less(&tile->variants[i], &canon)) {
                dup = 1; break;
            }
        }
        if (!dup) tile->variants[tile->variant_count++] = canon;
    }
}
