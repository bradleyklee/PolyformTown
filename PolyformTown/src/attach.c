#include "attach.h"
#include <string.h>

#define MAX_EDGES (MAX_VERTS)
#define MAX_LOCAL (4 * MAX_VERTS)

typedef struct {
    Edge e;
    int canceled;
} LEdge;

enum {
    WALK_FAIL = 0,
    WALK_OK = 1,
    WALK_DUP = 2
};

static int edge_same(Edge a, Edge b) { return coord_eq(a.a,b.a) && coord_eq(a.b,b.b); }
static int edge_opp(Edge a, Edge b) { return coord_eq(a.a,b.b) && coord_eq(a.b,b.a); }

/* ---------- point-in-cycle (ray casting) ---------- */

static int point_in_cycle(double px, double py, const Cycle *c) {
    int inside = 0;
    for (int i = 0, j = c->n - 1; i < c->n; j = i++) {
        double xi = c->v[i].x, yi = c->v[i].y;
        double xj = c->v[j].x, yj = c->v[j].y;

        if ((yi > py) != (yj > py)) {
            double xint = xi + (py - yi) * (xj - xi) / (yj - yi);
            if (xint > px)
                inside = !inside;
        }
    }
    return inside;
}

static void cycle_sample_point(const Cycle *c, double *px, double *py) {
    *px = (c->v[0].x + c->v[1].x + c->v[2].x) / 3.0;
    *py = (c->v[0].y + c->v[1].y + c->v[2].y) / 3.0;
}

/* ---------- NEW: overlap validation ---------- */

static int has_overlap_via_tile_test(const Poly *p, const Cycle *tile) {
    for (int i = 1; i < p->cycle_count; i++) {
        const Cycle *c = &p->cycles[i];
        if (c->n < 3) continue;

        double px, py;
        cycle_sample_point(c, &px, &py);

        if (point_in_cycle(px, py, tile))
            return 1;
    }
    return 0;
}

/* ---------- geometry core ---------- */

static void align_tile(const Cycle *tile, int tile_edge_index, Edge target, Cycle *out) {
    Edge te = cycle_edge(tile, tile_edge_index);
    int tdx = te.b.x - te.a.x, tdy = te.b.y - te.a.y;
    int bdx = target.a.x - target.b.x, bdy = target.a.y - target.b.y;
    int map = 0;

    if (tdx == bdx && tdy == bdy) map = 0;
    else if (tdy == bdx && -tdx == bdy) map = 1;
    else if (-tdx == bdx && -tdy == bdy) map = 2;
    else if (-tdy == bdx && tdx == bdy) map = 3;

    cycle_transform(tile, out, map);

    {
        Edge ne = cycle_edge(out, tile_edge_index);
        int dx = target.b.x - ne.a.x;
        int dy = target.b.y - ne.a.y;
        cycle_translate(out, dx, dy);
    }
}

int build_frontier_edges(const Poly *p, Edge *edges) {
    int n = 0;
    for (int i = 0; i < p->cycle_count; i++)
        for (int j = 0; j < p->cycles[i].n; j++)
            edges[n++] = cycle_edge(&p->cycles[i], j);
    return n;
}

static int build_union_edges(const Poly *a, const Cycle *b, LEdge *out, int *out_n) {
    int n = 0;

    for (int i = 0; i < a->cycle_count; i++)
        for (int j = 0; j < a->cycles[i].n; j++)
            out[n++] = (LEdge){ cycle_edge(&a->cycles[i], j), 0 };

    for (int i = 0; i < b->n; i++)
        out[n++] = (LEdge){ cycle_edge(b, i), 0 };

    for (int i = 0; i < n; i++) {
        if (out[i].canceled) continue;
        for (int j = i + 1; j < n; j++) {
            if (out[j].canceled) continue;
            if (edge_same(out[i].e, out[j].e)) return 0;
            if (edge_opp(out[i].e, out[j].e)) {
                out[i].canceled = 1;
                out[j].canceled = 1;
                break;
            }
        }
    }

    *out_n = n;
    return 1;
}

static int dir_index(Edge e) {
    int dx = e.b.x - e.a.x;
    int dy = e.b.y - e.a.y;
    if (dx == 1 && dy == 0) return 0;
    if (dx == 0 && dy == 1) return 1;
    if (dx == -1 && dy == 0) return 2;
    return 3;
}

static int coord_seen_before(const Coord *seen, int seen_n, Coord c) {
    for (int i = 0; i < seen_n; i++)
        if (coord_eq(seen[i], c)) return 1;
    return 0;
}

static int pick_next_edge(const Edge *edges, int m, const int *used, Coord v, int prev_dir, int prefer_left) {
    int rev = (prev_dir + 2) % 4;
    int best = -1;
    int best_score = 99;

    for (int i = 0; i < m; i++) {
        if (used[i]) continue;
        if (!coord_eq(edges[i].a, v)) continue;

        int d = dir_index(edges[i]);
        int score = (d - rev + 4) % 4;
        if (score == 0) continue;

        if (prefer_left) {
            if (score == 1) score = 3;
            else if (score == 3) score = 1;
        }

        if (score < best_score) {
            best_score = score;
            best = i;
        }
    }

    return best;
}

static int walk_one_cycle(const Edge *edges, int m, int *used, int start, int prefer_left, Cycle *out) {
    Coord seen[MAX_VERTS];
    int seen_n = 0;
    int cur = start;

    out->n = 0;

    while (1) {
        if (used[cur]) return WALK_FAIL;
        if (out->n >= MAX_VERTS) return WALK_FAIL;
        if (coord_seen_before(seen, seen_n, edges[cur].a)) return WALK_DUP;

        seen[seen_n++] = edges[cur].a;
        used[cur] = 1;
        out->v[out->n++] = edges[cur].a;

        Coord v = edges[cur].b;
        if (coord_eq(v, edges[start].a)) break;

        int next = pick_next_edge(edges, m, used, v, dir_index(edges[cur]), prefer_left);
        if (next < 0) return WALK_FAIL;

        cur = next;
    }

    return WALK_OK;
}

static int find_start_edge(const Edge *edges, int m, const int *used) {
    int start = -1;

    for (int i = 0; i < m; i++) {
        if (used[i]) continue;
        if (start < 0 ||
            edges[i].a.x < edges[start].a.x ||
            (edges[i].a.x == edges[start].a.x && edges[i].a.y < edges[start].a.y) ||
            (edges[i].a.x == edges[start].a.x && edges[i].a.y == edges[start].a.y &&
             dir_index(edges[i]) < dir_index(edges[start]))) {
            start = i;
        }
    }

    return start;
}

static int extract_cycles(const LEdge *in, int in_n, int prefer_left, Poly *out) {
    Edge edges[MAX_LOCAL];
    int m = 0;
    int used[MAX_LOCAL] = {0};

    out->cycle_count = 0;

    for (int i = 0; i < in_n; i++)
        if (!in[i].canceled)
            edges[m++] = in[i].e;

    if (m == 0) return 0;

    while (1) {
        int start = find_start_edge(edges, m, used);
        if (start < 0) break;

        if (out->cycle_count >= MAX_CYCLES) return 0;

        int used_snapshot[MAX_LOCAL];
        memcpy(used_snapshot, used, sizeof(used));

        Cycle c;
        int r = walk_one_cycle(edges, m, used, start, prefer_left, &c);

        if (r == WALK_DUP) {
            memcpy(used, used_snapshot, sizeof(used));
            r = walk_one_cycle(edges, m, used, start, !prefer_left, &c);
        }

        if (r != WALK_OK) return 0;

        out->cycles[out->cycle_count++] = c;
    }

    return out->cycle_count;
}

int try_attach_tile_poly(const Poly *base, const Cycle *tile_variant,
                         int base_edge_index, int tile_edge_index,
                         Poly *out) {
    Edge frontier[MAX_VERTS * MAX_CYCLES];
    Cycle aligned;
    LEdge merged[MAX_LOCAL];
    int frontier_n;
    int merged_n;

    frontier_n = build_frontier_edges(base, frontier);
    if (base_edge_index < 0 || base_edge_index >= frontier_n) return 0;

    align_tile(tile_variant, tile_edge_index, frontier[base_edge_index], &aligned);

    if (!build_union_edges(base, &aligned, merged, &merged_n)) return 0;
    if (!extract_cycles(merged, merged_n, 1, out)) return 0;

    if (has_overlap_via_tile_test(out, &aligned)) return 0;

    return 1;
}
