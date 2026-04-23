#include "tetrille.h"

static int is_unit6(int dx, int dy) {
    if (dx == 1 && dy == 0) return 1;
    if (dx == 0 && dy == 1) return 1;
    if (dx == -1 && dy == 1) return 1;
    if (dx == -1 && dy == 0) return 1;
    if (dx == 0 && dy == -1) return 1;
    if (dx == 1 && dy == -1) return 1;
    return 0;
}

int tetrille_point_to_sys3_scaled(Coord p, int *x, int *y) {
    if (p.v == 3) {
        *x = 2 * p.x;
        *y = 2 * p.y;
        return 1;
    }
    if (p.v == 4) {
        *x = 2 * p.x + p.y;
        *y = -p.x + p.y;
        return 1;
    }
    if (p.v == 6) {
        *x = 4 * p.x + 2 * p.y;
        *y = -2 * p.x + 2 * p.y;
        return 1;
    }
    return 0;
}

int tetrille_point_to_sys4(Coord p, int *x, int *y) {
    if (p.v == 4) {
        *x = p.x;
        *y = p.y;
        return 1;
    }
    if (p.v == 6) {
        *x = 2 * p.x;
        *y = 2 * p.y;
        return 1;
    }
    return 0;
}


int tetrille_edge_tag(Edge e, TetrilleTaggedVec *tv) {
    int system = e.a.v < e.b.v ? e.a.v : e.b.v;
    int ax, ay, bx, by;
    if (system == 3) {
        if (!tetrille_point_to_sys3_scaled(e.a, &ax, &ay)) return 0;
        if (!tetrille_point_to_sys3_scaled(e.b, &bx, &by)) return 0;
        tv->system = 3;
        tv->dx = bx - ax;
        tv->dy = by - ay;
        return 1;
    }
    if (system == 4) {
        if (!tetrille_point_to_sys4(e.a, &ax, &ay)) return 0;
        if (!tetrille_point_to_sys4(e.b, &bx, &by)) return 0;
        tv->system = 4;
        tv->dx = bx - ax;
        tv->dy = by - ay;
        return 1;
    }
    if (system == 6) {
        tv->system = 6;
        tv->dx = e.b.x - e.a.x;
        tv->dy = e.b.y - e.a.y;
        return 1;
    }
    return 0;
}

int tetrille_delta_to_6(int valence, int dx, int dy, int *m, int *n) {
    if (valence == 6) {
        *m = dx;
        *n = dy;
        return 1;
    }
    if (valence == 4) {
        if ((dx & 1) || (dy & 1)) return 0;
        *m = dx / 2;
        *n = dy / 2;
        return 1;
    }
    if (valence == 3) {
        int a = dx - dy;
        int b = dx + 2 * dy;
        if (a % 3 != 0 || b % 3 != 0) return 0;
        *m = a / 3;
        *n = b / 3;
        return 1;
    }
    return 0;
}

void tetrille_translate_cycle(Cycle *c, int m6, int n6) {
    for (int i = 0; i < c->n; i++) {
        Coord *p = &c->v[i];
        if (p->v == 6) {
            p->x += m6;
            p->y += n6;
        } else if (p->v == 4) {
            p->x += 2 * m6;
            p->y += 2 * n6;
        } else if (p->v == 3) {
            p->x += 2 * m6 + n6;
            p->y += -m6 + n6;
        } else {
            p->x += m6;
            p->y += n6;
        }
    }
}

int tetrille_coords_adjacent(Coord a, Coord b) {
    int system = a.v < b.v ? a.v : b.v;
    int ax, ay, bx, by;
    int dx, dy;

    if (system == 3) {
        if (!tetrille_point_to_sys3_scaled(a, &ax, &ay)) return 0;
        if (!tetrille_point_to_sys3_scaled(b, &bx, &by)) return 0;
        dx = bx - ax;
        dy = by - ay;
        return is_unit6(dx, dy);
    }
    if (system == 4) {
        if (!tetrille_point_to_sys4(a, &ax, &ay)) return 0;
        if (!tetrille_point_to_sys4(b, &bx, &by)) return 0;
        dx = bx - ax;
        dy = by - ay;
        return is_unit6(dx, dy);
    }
    if (system == 6) {
        dx = b.x - a.x;
        dy = b.y - a.y;
        return is_unit6(dx, dy);
    }
    return 0;
}

void tetrille_embed_point_scaled(Coord p, long long *x, long long *y) {
    if (p.v == 6) {
        *x = 6LL * p.x;
        *y = 6LL * p.y;
    } else if (p.v == 4) {
        *x = 3LL * p.x;
        *y = 3LL * p.y;
    } else {
        *x = 2LL * (p.x - p.y);
        *y = 2LL * (p.x + 2LL * p.y);
    }
}

int tetrille_point_on_segment(Coord p, Coord a, Coord b) {
    long long px, py, ax, ay, bx, by;
    long long cross;
    tetrille_embed_point_scaled(p, &px, &py);
    tetrille_embed_point_scaled(a, &ax, &ay);
    tetrille_embed_point_scaled(b, &bx, &by);

    cross = (px - ax) * (by - ay) - (py - ay) * (bx - ax);
    if (cross != 0) return 0;

    if (px < (ax < bx ? ax : bx) || px > (ax > bx ? ax : bx)) return 0;
    if (py < (ay < by ? ay : by) || py > (ay > by ? ay : by)) return 0;
    return 1;
}

void tetrille_poly_canonical_key(const Poly *in, Poly *out) {
    Poly tmp;
    tmp.cycle_count = in->cycle_count;
    for (int i = 0; i < in->cycle_count; i++) {
        tmp.cycles[i].n = in->cycles[i].n;
        for (int j = 0; j < in->cycles[i].n; j++) {
            long long x, y;
            tetrille_embed_point_scaled(in->cycles[i].v[j], &x, &y);
            tmp.cycles[i].v[j].v = 6;
            tmp.cycles[i].v[j].x = (int)x;
            tmp.cycles[i].v[j].y = (int)y;
        }
    }
    poly_canonicalize_lattice(&tmp, out, TILE_LATTICE_TRIANGULAR);
}
