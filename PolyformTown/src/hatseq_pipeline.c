#include "hatseq_pipeline.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "hash.h"
#include "vcomp.h"

static void sv_init(VCompStateVec *v) {
    v->data = NULL;
    v->count = 0;
    v->cap = 0;
}

static void sv_destroy(VCompStateVec *v) { free(v->data); }

static void sv_push(VCompStateVec *v, const VCompState *s) {
    if (v->count == v->cap) {
        size_t nc = v->cap ? 2 * v->cap : 64;
        v->data = realloc(v->data, nc * sizeof(*v->data));
        if (!v->data) {
            fprintf(stderr, "oom\n");
            exit(1);
        }
        v->cap = nc;
    }
    v->data[v->count++] = *s;
}

static int coord_cmp(const void *A, const void *B) {
    const Coord *a = A;
    const Coord *b = B;
    if (a->v != b->v) return a->v - b->v;
    if (a->x != b->x) return a->x - b->x;
    return a->y - b->y;
}

static int poly_equal_local(const Poly *a, const Poly *b) {
    if (a->cycle_count != b->cycle_count) return 0;
    for (int k = 0; k < a->cycle_count; k++) {
        const Cycle *ca = &a->cycles[k];
        const Cycle *cb = &b->cycles[k];
        if (ca->n != cb->n) return 0;
        for (int i = 0; i < ca->n; i++) {
            if (ca->v[i].v != cb->v[i].v ||
                ca->v[i].x != cb->v[i].x ||
                ca->v[i].y != cb->v[i].y) {
                return 0;
            }
        }
    }
    return 1;
}

static int state_equal(const VCompState *a, const VCompState *b) {
    if (!poly_equal_local(&a->poly, &b->poly)) return 0;
    if (a->hidden_count != b->hidden_count) return 0;
    for (int i = 0; i < a->hidden_count; i++) {
        if (!coord_eq(a->hidden[i], b->hidden[i])) return 0;
    }
    return 1;
}

typedef struct {
    uint64_t hash;
    size_t index;
    int used;
} StateSetEntry;

typedef struct {
    StateSetEntry *data;
    size_t cap;
    size_t count;
} StateSet;

static size_t pow2_ge(size_t n) {
    size_t p = 1;
    while (p < n) p <<= 1;
    return p;
}

static void stateset_init(StateSet *s, size_t cap_hint) {
    s->cap = pow2_ge(cap_hint < 128 ? 128 : cap_hint);
    s->count = 0;
    s->data = calloc(s->cap, sizeof(*s->data));
    if (!s->data) {
        fprintf(stderr, "oom\n");
        exit(1);
    }
}

static void stateset_destroy(StateSet *s) { free(s->data); }

static uint64_t mix_u64(uint64_t h, uint64_t x) {
    h ^= x + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
    return h;
}

static uint64_t poly_hash64(const Poly *p) {
    uint64_t h = 1469598103934665603ULL;
    h = mix_u64(h, (uint64_t)p->cycle_count);
    for (int c = 0; c < p->cycle_count; c++) {
        h = mix_u64(h, (uint64_t)p->cycles[c].n);
        for (int i = 0; i < p->cycles[c].n; i++) {
            const Coord *q = &p->cycles[c].v[i];
            h = mix_u64(h, (uint64_t)(uint32_t)q->v);
            h = mix_u64(h, (uint64_t)(uint32_t)q->x);
            h = mix_u64(h, (uint64_t)(uint32_t)q->y);
        }
    }
    return h;
}

static uint64_t state_hash64(const VCompState *s, const Poly *poly_key) {
    uint64_t h = poly_hash64(poly_key);
    h = mix_u64(h, (uint64_t)s->hidden_count);
    for (int i = 0; i < s->hidden_count; i++) {
        h = mix_u64(h, (uint64_t)(uint32_t)s->hidden[i].v);
        h = mix_u64(h, (uint64_t)(uint32_t)s->hidden[i].x);
        h = mix_u64(h, (uint64_t)(uint32_t)s->hidden[i].y);
    }
    return h;
}

static void stateset_rehash(StateSet *s, const VCompStateVec *v, size_t nc) {
    StateSetEntry *old = s->data;
    size_t old_cap = s->cap;
    s->data = calloc(nc, sizeof(*s->data));
    if (!s->data) {
        fprintf(stderr, "oom\n");
        exit(1);
    }
    s->cap = nc;
    s->count = 0;
    for (size_t i = 0; i < old_cap; i++) {
        if (!old[i].used) continue;
        size_t idx = (size_t)(old[i].hash & (s->cap - 1));
        while (s->data[idx].used) idx = (idx + 1) & (s->cap - 1);
        s->data[idx] = old[i];
        s->count++;
        (void)v;
    }
    free(old);
}

static int stateset_insert(StateSet *s,
                           const VCompStateVec *v,
                           const VCompState *state,
                           uint64_t hash,
                           size_t index) {
    if ((s->count + 1) * 10 >= s->cap * 7) {
        stateset_rehash(s, v, s->cap * 2);
    }
    size_t slot = (size_t)(hash & (s->cap - 1));
    while (s->data[slot].used) {
        if (s->data[slot].hash == hash &&
            state_equal(&v->data[s->data[slot].index], state)) {
            return 0;
        }
        slot = (slot + 1) & (s->cap - 1);
    }
    s->data[slot].used = 1;
    s->data[slot].hash = hash;
    s->data[slot].index = index;
    s->count++;
    return 1;
}

typedef struct {
    VCompStateVec *levels;
    StateSet *level_sets;
    HashTable *poly_seen;
    const Tile *tile;
    int selector_mask;
    int max_level;
    int emit_level;
    const VCompState *parent_state;
    int double_check_mode;
    int strict_three_mode;
} EmitCtx;

static int selector_contains(int selector_mask, int v) {
    if (v == 3) return (selector_mask & 1) != 0;
    if (v == 4) return (selector_mask & 2) != 0;
    if (v == 6) return (selector_mask & 4) != 0;
    return 0;
}

static int count_selected_hidden(const VCompState *s, int selector_mask) {
    int count = 0;
    for (int i = 0; i < s->hidden_count; i++) {
        if (selector_contains(selector_mask, s->hidden[i].v)) count++;
    }
    return count;
}

static void collect_emit_trace(const Poly *p,
                               const Coord *hidden,
                               int hidden_count,
                               const Cycle *added_tiles,
                               int added_tile_count,
                               void *userdata) {
    EmitCtx *ctx = userdata;

    VCompState s;
    Poly key;
    uint64_t h;
    int level;
    s.poly = *p;
    s.hidden_count = hidden_count;
    for (int i = 0; i < hidden_count; i++) s.hidden[i] = hidden[i];
    s.tile_count = 0;
    if (ctx->parent_state) {
        s.tile_count = ctx->parent_state->tile_count;
        for (int ti = 0; ti < ctx->parent_state->tile_count &&
                         ti < VCOMP_MAX_TILES; ti++) {
            s.tiles[ti] = ctx->parent_state->tiles[ti];
        }
    }
    for (int ti = 0; ti < added_tile_count && s.tile_count < VCOMP_MAX_TILES; ti++) {
        s.tiles[s.tile_count++] = added_tiles[ti];
    }
    qsort(s.hidden, s.hidden_count, sizeof(Coord), coord_cmp);
    if (ctx->strict_three_mode) {
        level = ctx->emit_level + 1;
    } else {
        level = count_selected_hidden(&s, ctx->selector_mask);
    }
    if (level > ctx->max_level) return;

    if (ctx->double_check_mode) {
        if (!poly_has_live_boundary(&s.poly, ctx->tile)) {
            return;
        }
    }

    poly_hash_key_lattice(p, ctx->tile->lattice, &key);
    h = state_hash64(&s, &key);

    if (stateset_insert(&ctx->level_sets[level],
                        &ctx->levels[level],
                        &s,
                        h,
                        ctx->levels[level].count)) {
        sv_push(&ctx->levels[level], &s);
        hash_insert(&ctx->poly_seen[level], &key);
    }
}

void run_hatseq_levels(const Tile *tile,
                       int max_n,
                       int selector_mask,
                       int double_check_mode,
                       int strict_three_mode,
                       VCompLevelFn on_level,
                       void *userdata) {
    VCompStateVec levels[VCOMP_MAX_LEVELS];
    StateSet level_sets[VCOMP_MAX_LEVELS];
    HashTable poly_seen[VCOMP_MAX_LEVELS];

    for (int i = 0; i <= max_n; i++) {
        sv_init(&levels[i]);
        stateset_init(&level_sets[i], 128);
        hash_init(&poly_seen[i], 1024);
    }

    VCompState seed_state;
    Poly seed_key;
    memset(&seed_state, 0, sizeof(seed_state));
    seed_state.poly.cycle_count = 1;
    seed_state.poly.cycles[0] = tile->base;
    seed_state.tile_count = 1;
    seed_state.tiles[0] = tile->base;
    sv_push(&levels[0], &seed_state);
    poly_hash_key_lattice(&seed_state.poly, tile->lattice, &seed_key);
    stateset_insert(&level_sets[0], &levels[0], &seed_state,
                    state_hash64(&seed_state, &seed_key), 0);
    hash_insert(&poly_seen[0], &seed_key);

    EmitCtx ectx;
    ectx.levels = levels;
    ectx.level_sets = level_sets;
    ectx.poly_seen = poly_seen;
    ectx.tile = tile;
    ectx.selector_mask = selector_mask;
    ectx.max_level = max_n;
    ectx.emit_level = 0;
    ectx.parent_state = NULL;
    ectx.double_check_mode = double_check_mode;
    ectx.strict_three_mode = strict_three_mode;

    for (int level = 0; level <= max_n; level++) {
        if (on_level &&
            !on_level(level, &levels[level], poly_seen[level].count,
                      tile, userdata)) {
            break;
        }
        if (level == max_n) break;

        for (size_t i = 0; i < levels[level].count; i++) {
            Coord verts[MAX_VERTS * MAX_CYCLES];
            int vc = build_boundary_vertices(&levels[level].data[i].poly,
                                             verts);
            for (int j = 0; j < vc; j++) {
                if (!selector_contains(selector_mask, verts[j].v)) continue;
                ectx.emit_level = level;
                ectx.parent_state = &levels[level].data[i];
                enumerate_vertex_completions_steps_trace(
                    &levels[level].data[i].poly,
                    tile,
                    verts[j],
                    levels[level].data[i].hidden,
                    levels[level].data[i].hidden_count,
                    /* strict-three only constrains the active growth target */
                    strict_three_mode ? 2 : -1,
                    collect_emit_trace,
                    &ectx);
            }
        }
    }

    for (int i = 0; i <= max_n; i++) {
        hash_destroy(&poly_seen[i]);
        stateset_destroy(&level_sets[i]);
        sv_destroy(&levels[i]);
    }
}
