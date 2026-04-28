#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "core/boundary.h"
#include "core/cycle.h"
#include "core/lattice.h"
#include "core/tile.h"
#include "throughput/vcomp.h"

#define RL0_MAX_TRACE MAX_VERTS

typedef struct {
    FILE *fp;
    int lattice;
    const Tile *tile;
    Coord target;
    int record_no;
} RL0Ctx;

static int coord_less_local(Coord a, Coord b) {
    if (a.v != b.v) return a.v < b.v;
    if (a.x != b.x) return a.x < b.x;
    return a.y < b.y;
}

static int coord_cmp_local(const void *A, const void *B) {
    const Coord *a = A;
    const Coord *b = B;
    if (coord_less_local(*a, *b)) return -1;
    if (coord_less_local(*b, *a)) return 1;
    return 0;
}

static void print_cycle(FILE *fp, const Cycle *c) {
    fprintf(fp, "[");
    for (int i = 0; i < c->n; i++) {
        if (i) fprintf(fp, ",");
        fprintf(fp, "(%d,%d,%d)", c->v[i].v, c->v[i].x, c->v[i].y);
    }
    fprintf(fp, "]");
}

static void print_poly(FILE *fp, const Poly *p) {
    fprintf(fp, "[");
    for (int i = 0; i < p->cycle_count; i++) {
        if (i) fprintf(fp, "|");
        print_cycle(fp, &p->cycles[i]);
    }
    fprintf(fp, "]");
}

static void print_tile_list(FILE *fp, const Cycle *tiles, int tile_count) {
    fprintf(fp, "[");
    for (int i = 0; i < tile_count; i++) {
        if (i) fprintf(fp, ",");
        print_cycle(fp, &tiles[i]);
    }
    fprintf(fp, "]");
}

static void print_indices(FILE *fp, const int *indices, int count) {
    fprintf(fp, "[");
    for (int i = 0; i < count; i++) {
        if (i) fprintf(fp, ",");
        fprintf(fp, "%d", indices[i]);
    }
    fprintf(fp, "]");
}

static int cycle_vertex_index(const Cycle *c, Coord q) {
    for (int i = 0; i < c->n; i++) {
        if (coord_eq(c->v[i], q)) return i;
    }
    return -1;
}

static void emit_raw_completion(const VCompRawState *raw, RL0Ctx *ctx) {
    int indices[RL0_MAX_TRACE];
    int total_tile_count = raw->tile_count;
    Coord center = raw->target;

    if (!poly_has_live_boundary(&raw->poly, ctx->tile)) return;

    for (int i = 0; i < total_tile_count; i++) {
        indices[i] = cycle_vertex_index(&raw->tiles[i], center);
    }

    int valence = lattice_direction_count(ctx->lattice);
    if (ctx->lattice == TILE_LATTICE_TETRILLE &&
        (center.v == 3 || center.v == 4 || center.v == 6)) {
        valence = center.v;
    }

    ctx->record_no++;
    fprintf(ctx->fp, "---[%d]---\n", ctx->record_no);
    fprintf(ctx->fp, "valence:%d\n", valence);
    fprintf(ctx->fp, "tile_count:%d\n", total_tile_count);
    fprintf(ctx->fp, "center:(%d,%d,%d)\n", center.v, center.x, center.y);
    fprintf(ctx->fp, "canonical_boundary:");
    print_poly(ctx->fp, &raw->poly);
    fprintf(ctx->fp, "\n");
    fprintf(ctx->fp, "tiles:");
    print_tile_list(ctx->fp, raw->tiles, total_tile_count);
    fprintf(ctx->fp, "\n");
    fprintf(ctx->fp, "indices:");
    print_indices(ctx->fp, indices, total_tile_count);
    fprintf(ctx->fp, "\n");
}

static int ensure_dir(const char *path) {
    if (mkdir(path, 0777) == 0) return 1;
    if (errno == EEXIST) return 1;
    return 0;
}

int main(int argc, char **argv) {
    const char *tile_path = "tiles/hat.tile";
    const char *output_path = "levelData/rl0/completions.dat";

    if (argc > 1) tile_path = argv[1];
    if (argc > 2) output_path = argv[2];

    Tile tile;
    if (!tile_load(tile_path, &tile)) {
        fprintf(stderr, "failed to load tile: %s\n", tile_path);
        return 1;
    }

    if (!ensure_dir("levelData") || !ensure_dir("levelData/rl0")) {
        fprintf(stderr, "failed to create levelData/rl0\n");
        return 1;
    }

    FILE *fp = fopen(output_path, "w");
    if (!fp) {
        fprintf(stderr, "failed to open output: %s\n", output_path);
        return 1;
    }

    Poly seed;
    Coord verts[MAX_VERTS * MAX_CYCLES];
    int vc;

    memset(&seed, 0, sizeof(seed));
    seed.cycle_count = 1;
    seed.cycles[0] = tile.base;
    vc = build_boundary_vertices(&seed, verts);
    if (vc < 0) {
        fprintf(stderr, "failed to build boundary vertices\n");
        fclose(fp);
        return 1;
    }

    qsort(verts, (size_t)vc, sizeof(Coord), coord_cmp_local);

    RL0Ctx ctx;
    ctx.fp = fp;
    ctx.lattice = tile.lattice;
    ctx.tile = &tile;
    ctx.record_no = 0;

    for (int i = 0; i < vc; i++) {
        VCompLevels raw;
        ctx.target = verts[i];
        vcomp_levels_init(&raw, VCOMP_MAX_LEVELS - 1);
        vcomp_enumerate_levels(&seed,
                               &tile,
                               verts[i],
                               NULL,
                               0,
                               NULL,
                               0,
                               VCOMP_MAX_LEVELS - 1,
                               1,
                               &raw);
        for (int level = 1; level <= raw.max_level; level++) {
            for (size_t r = 0; r < raw.levels[level].count; r++) {
                emit_raw_completion(&raw.levels[level].data[r], &ctx);
            }
        }
        vcomp_levels_destroy(&raw);
    }

    fclose(fp);
    return 0;
}
