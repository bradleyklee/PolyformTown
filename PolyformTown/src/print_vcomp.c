#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cycle.h"
#include "tile.h"
#include "hash.h"
#include "vcomp.h"
#include "tetrille.h"

#define MAX_LEVELS 32
#define MAX_HIDDEN (MAX_VERTS * MAX_CYCLES)

typedef struct {
    Poly poly;
    Coord hidden[MAX_HIDDEN];
    int hidden_count;
} State;

typedef struct {
    State *data;
    size_t count, cap;
} StateVec;

typedef struct {
    StateVec *levels;
    int lattice;
    int max_level;
} EmitCtx;

static void sv_init(StateVec *v) { v->data = NULL; v->count = v->cap = 0; }
static void sv_destroy(StateVec *v) { free(v->data); }
static void sv_push(StateVec *v, const State *s) {
    if (v->count == v->cap) {
        size_t nc = v->cap ? 2 * v->cap : 64;
        v->data = realloc(v->data, nc * sizeof(*v->data));
        if (!v->data) { fprintf(stderr, "oom\n"); exit(1); }
        v->cap = nc;
    }
    v->data[v->count++] = *s;
}

static int coord_cmp(const void *A, const void *B) {
    const Coord *a = A, *b = B;
    if (a->v != b->v) return a->v - b->v;
    if (a->x != b->x) return a->x - b->x;
    return a->y - b->y;
}

static int poly_equal_local(const Poly *a, const Poly *b) {
    if (a->cycle_count != b->cycle_count) return 0;
    for (int k = 0; k < a->cycle_count; k++) {
        const Cycle *ca = &a->cycles[k], *cb = &b->cycles[k];
        if (ca->n != cb->n) return 0;
        for (int i = 0; i < ca->n; i++) {
            if (ca->v[i].v != cb->v[i].v ||
                ca->v[i].x != cb->v[i].x ||
                ca->v[i].y != cb->v[i].y) return 0;
        }
    }
    return 1;
}

static int state_equal(const State *a, const State *b) {
    if (!poly_equal_local(&a->poly, &b->poly) || a->hidden_count != b->hidden_count) return 0;
    for (int i = 0; i < a->hidden_count; i++) if (!coord_eq(a->hidden[i], b->hidden[i])) return 0;
    return 1;
}

static int sv_contains(const StateVec *v, const State *s) {
    for (size_t i = 0; i < v->count; i++) if (state_equal(&v->data[i], s)) return 1;
    return 0;
}

static void poly_hash_key(const Poly *p, int lattice, Poly *key) {
    if (lattice == TILE_LATTICE_TETRILLE) tetrille_poly_canonical_key(p, key);
    else poly_canonicalize_lattice(p, key, lattice);
}

static void collect_emit(const Poly *p, const Coord *hidden, int hidden_count, void *userdata) {
    EmitCtx *ctx = userdata;
    if (hidden_count > ctx->max_level) return;

    State s;
    s.poly = *p;
    s.hidden_count = hidden_count;
    for (int i = 0; i < hidden_count; i++) s.hidden[i] = hidden[i];
    qsort(s.hidden, s.hidden_count, sizeof(Coord), coord_cmp);

    if (!sv_contains(&ctx->levels[hidden_count], &s)) sv_push(&ctx->levels[hidden_count], &s);
}

int main(int argc, char **argv) {
    int max_n = 5;
    const char *tile_path = "tiles/monomino.tile";
    if (argc > 1) max_n = atoi(argv[1]);
    if (argc > 2) tile_path = argv[2];
    if (max_n < 0) max_n = 0;
    if (max_n >= MAX_LEVELS) max_n = MAX_LEVELS - 1;

    Tile tile;
    if (!tile_load(tile_path, &tile)) {
        fprintf(stderr, "failed to load tile: %s\n", tile_path);
        return 1;
    }

    Poly seed_raw;
    seed_raw.cycle_count = 1;
    seed_raw.cycles[0] = tile.base;

    StateVec levels[MAX_LEVELS];
    for (int i = 0; i <= max_n; i++) sv_init(&levels[i]);

    State seed_state;
    memset(&seed_state, 0, sizeof(seed_state));
    seed_state.poly = seed_raw;
    sv_push(&levels[0], &seed_state);

    EmitCtx ectx = { levels, tile.lattice, max_n };

    for (int level = 0; level <= max_n; level++) {
        if (level == max_n) {
            HashTable printed;
            hash_init(&printed, 1024);
            for (size_t i = 0; i < levels[level].count; i++) {
                Poly key;
                poly_hash_key(&levels[level].data[i].poly, tile.lattice, &key);
                if (hash_insert(&printed, &key)) tile_print_imgtable_shape(&tile, &levels[level].data[i].poly);
            }
            hash_destroy(&printed);
            break;
        }

        for (size_t i = 0; i < levels[level].count; i++) {
            Coord verts[MAX_VERTS * MAX_CYCLES];
            int vc = build_boundary_vertices(&levels[level].data[i].poly, verts);
            for (int j = 0; j < vc; j++) {
                enumerate_vertex_completions(&levels[level].data[i].poly,
                                            &tile,
                                            verts[j],
                                            levels[level].data[i].hidden,
                                            levels[level].data[i].hidden_count,
                                            collect_emit,
                                            &ectx);
            }
        }
    }

    for (int i = 0; i <= max_n; i++) sv_destroy(&levels[i]);
    return 0;
}
