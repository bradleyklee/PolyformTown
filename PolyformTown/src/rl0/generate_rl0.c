#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "core/cycle.h"
#include "core/attach.h"
#include "core/boundary.h"
#include "core/lattice.h"
#include "core/tetrille.h"
#include "core/tile.h"
#include "throughput/vcomp.h"

#define RL0_MAX_TRACE MAX_VERTS

typedef struct {
    FILE *fp;
    int lattice;
    const Tile *tile;
    Coord target;
    int record_no;
    Cycle seed_tile;
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

static void cycle_translate_lattice(Cycle *c, int lattice, int dx, int dy) {
    if (lattice == TILE_LATTICE_TETRILLE) {
        tetrille_translate_cycle(c, dx, dy);
        return;
    }
    cycle_translate(c, dx, dy);
}

static void poly_translate_lattice(Poly *p, int lattice, int dx, int dy) {
    for (int i = 0; i < p->cycle_count; i++) {
        cycle_translate_lattice(&p->cycles[i], lattice, dx, dy);
    }
}

static int floor_div6_local(long long a) {
    if (a >= 0) return (int)(a / 6LL);
    return (int)(-(((-a) + 5LL) / 6LL));
}

static void normalize_poly_and_tiles(Poly *p,
                                     Cycle *tiles,
                                     int tile_count,
                                     int lattice) {
    if (lattice == TILE_LATTICE_TETRILLE) {
        long long minx = 0;
        long long miny = 0;
        int first = 1;
        for (int i = 0; i < p->cycle_count; i++) {
            for (int j = 0; j < p->cycles[i].n; j++) {
                long long x, y;
                tetrille_embed_point_scaled(p->cycles[i].v[j], &x, &y);
                if (first || x < minx) minx = x;
                if (first || y < miny) miny = y;
                first = 0;
            }
        }
        int tx = -floor_div6_local(minx);
        int ty = -floor_div6_local(miny);
        poly_translate_lattice(p, lattice, tx, ty);
        for (int i = 0; i < tile_count; i++) {
            cycle_translate_lattice(&tiles[i], lattice, tx, ty);
        }
        return;
    }

    int minx = p->cycles[0].v[0].x;
    int miny = p->cycles[0].v[0].y;
    for (int i = 0; i < p->cycle_count; i++) {
        for (int j = 0; j < p->cycles[i].n; j++) {
            if (p->cycles[i].v[j].x < minx) minx = p->cycles[i].v[j].x;
            if (p->cycles[i].v[j].y < miny) miny = p->cycles[i].v[j].y;
        }
    }
    poly_translate_lattice(p, lattice, -minx, -miny);
    for (int i = 0; i < tile_count; i++) {
        cycle_translate_lattice(&tiles[i], lattice, -minx, -miny);
    }
}

static int cycle_abs_area_cmp_desc(const Cycle *a, const Cycle *b, int lattice) {
    long long aa = cycle_signed_area2(a, lattice);
    long long ab = cycle_signed_area2(b, lattice);
    if (aa < 0) aa = -aa;
    if (ab < 0) ab = -ab;
    if (aa != ab) return aa > ab;
    return cycle_less(a, b);
}

static void prepare_poly_cycles_local(Poly *p, int lattice) {
    int outer = 0;
    for (int i = 1; i < p->cycle_count; i++) {
        if (cycle_abs_area_cmp_desc(&p->cycles[i], &p->cycles[outer], lattice)) {
            outer = i;
        }
    }

    if (outer != 0) {
        Cycle tmp = p->cycles[0];
        p->cycles[0] = p->cycles[outer];
        p->cycles[outer] = tmp;
    }

    for (int i = 0; i < p->cycle_count; i++) {
        long long area = cycle_signed_area2(&p->cycles[i], lattice);
        if (i == 0) {
            if (area < 0) cycle_reverse(&p->cycles[i]);
        } else {
            if (area > 0) cycle_reverse(&p->cycles[i]);
        }
        cycle_canonicalize_shift(&p->cycles[i]);
    }

    for (int i = 1; i < p->cycle_count; i++) {
        for (int j = i + 1; j < p->cycle_count; j++) {
            if (cycle_less(&p->cycles[j], &p->cycles[i])) {
                Cycle tmp = p->cycles[i];
                p->cycles[i] = p->cycles[j];
                p->cycles[j] = tmp;
            }
        }
    }
}

static void copy_tiles(Cycle *dst, const Cycle *src, int tile_count) {
    for (int i = 0; i < tile_count; i++) dst[i] = src[i];
}

static void canonicalize_completion_with_tiles(const Poly *src,
                                               const Cycle *src_tiles,
                                               int tile_count,
                                               const int *src_indices,
                                               int lattice,
                                               Poly *out_poly,
                                               Cycle *out_tiles,
                                               int *out_indices) {
    int tcount = lattice_transform_count(lattice);
    Poly best_poly = {0};
    Cycle best_tiles[RL0_MAX_TRACE];
    int first = 1;

    for (int t = 0; t < tcount; t++) {
        Poly cur_poly;
        Cycle cur_tiles[RL0_MAX_TRACE];

        poly_transform_lattice(src, &cur_poly, lattice, t);
        for (int i = 0; i < tile_count; i++) {
            cycle_transform_lattice(&src_tiles[i], &cur_tiles[i], lattice, t);
        }

        normalize_poly_and_tiles(&cur_poly, cur_tiles, tile_count, lattice);
        prepare_poly_cycles_local(&cur_poly, lattice);

        if (first || poly_less(&cur_poly, &best_poly)) {
            best_poly = cur_poly;
            copy_tiles(best_tiles, cur_tiles, tile_count);
            first = 0;
        }
    }

    *out_poly = best_poly;
    copy_tiles(out_tiles, best_tiles, tile_count);
    for (int i = 0; i < tile_count; i++) out_indices[i] = src_indices[i];
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

static Coord canonical_center_from_tiles(const Cycle *tiles,
                                         const int *indices,
                                         int tile_count,
                                         Coord fallback) {
    Coord center = fallback;
    int have_center = 0;

    for (int i = 0; i < tile_count; i++) {
        int idx = indices[i];
        if (idx < 0 || idx >= tiles[i].n) continue;
        Coord q = tiles[i].v[idx];
        if (!have_center) {
            center = q;
            have_center = 1;
            continue;
        }
        if (!coord_eq(center, q)) return fallback;
    }

    return center;
}

static void emit_completion(const Poly *p,
                            const Coord *hidden,
                            int hidden_count,
                            const Cycle *added_tiles,
                            int added_tile_count,
                            const int *added_indices,
                            void *userdata) {
    RL0Ctx *ctx = userdata;
    Poly canonical;
    Cycle canonical_tiles[RL0_MAX_TRACE];
    int canonical_indices[RL0_MAX_TRACE];
    Cycle all_tiles[RL0_MAX_TRACE];
    int all_indices[RL0_MAX_TRACE];
    int total_tile_count = added_tile_count + 1;

    (void)hidden;
    (void)hidden_count;

    if (!poly_has_live_boundary(p, ctx->tile)) return;

    all_tiles[0] = ctx->seed_tile;
    all_indices[0] = cycle_vertex_index(&ctx->seed_tile, ctx->target);
    for (int i = 0; i < added_tile_count; i++) {
        all_tiles[i + 1] = added_tiles[i];
        all_indices[i + 1] = added_indices[i];
    }

    canonicalize_completion_with_tiles(p,
                                       all_tiles,
                                       total_tile_count,
                                       all_indices,
                                       ctx->lattice,
                                       &canonical,
                                       canonical_tiles,
                                       canonical_indices);
    Coord center = canonical_center_from_tiles(canonical_tiles,
                                               canonical_indices,
                                               total_tile_count,
                                               ctx->target);

    int valence = lattice_direction_count(ctx->lattice);
    if (ctx->lattice == TILE_LATTICE_TETRILLE &&
        (ctx->target.v == 3 || ctx->target.v == 4 || ctx->target.v == 6)) {
        valence = ctx->target.v;
    }

    ctx->record_no++;
    fprintf(ctx->fp, "---[%d]---\n", ctx->record_no);
    fprintf(ctx->fp, "valence:%d\n", valence);
    fprintf(ctx->fp, "tile_count:%d\n", total_tile_count);
    fprintf(ctx->fp, "center:(%d,%d,%d)\n",
            center.v, center.x, center.y);
    fprintf(ctx->fp, "canonical_boundary:");
    print_poly(ctx->fp, &canonical);
    fprintf(ctx->fp, "\n");
    fprintf(ctx->fp, "tiles:");
    print_tile_list(ctx->fp, canonical_tiles, total_tile_count);
    fprintf(ctx->fp, "\n");
    fprintf(ctx->fp, "indices:");
    print_indices(ctx->fp, canonical_indices, total_tile_count);
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
    ctx.seed_tile = tile.base;

    for (int i = 0; i < vc; i++) {
        ctx.target = verts[i];
        enumerate_vertex_completions_steps_trace(&seed,
                                                 &tile,
                                                 verts[i],
                                                 NULL,
                                                 0,
                                                 -1,
                                                 emit_completion,
                                                 &ctx);
    }

    fclose(fp);
    return 0;
}
