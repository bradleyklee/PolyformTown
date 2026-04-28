#include <math.h>
#include "core/numerics.h"
#include "core/lattice.h"

/* ---------- point-in-cycle (ray casting) ---------- */

int point_in_cycle(double px, double py,
                       const Cycle *c, int lattice)
{
    int inside = 0;

    for (int i = 0, j = c->n - 1; i < c->n; j = i++) {
        double xi, yi, xj, yj;

        lattice_embed_point(lattice, c->v[i], &xi, &yi);
        lattice_embed_point(lattice, c->v[j], &xj, &yj);

        if ((yi > py) != (yj > py)) {
            double xint = xi + (py - yi) * (xj - xi) / (yj - yi);
            if (xint > px) inside = !inside;
        }
    }

    return inside;
}

int cycle_interior_point(const Cycle *c, int lattice,
                             double *px, double *py)
{
    long long area2 = cycle_signed_area2(c, lattice);
    if (area2 == 0) return 0;

    for (int i = 0; i < c->n; i++) {
        Coord a = c->v[i];
        Coord b = c->v[(i + 1) % c->n];

        double ax, ay, bx, by;
        double dx, dy, nx, ny, scale;
        double mx, my, sx, sy;

        lattice_embed_point(lattice, a, &ax, &ay);
        lattice_embed_point(lattice, b, &bx, &by);

        dx = bx - ax;
        dy = by - ay;
        if (dx == 0.0 && dy == 0.0) continue;

        if (area2 > 0) {
            nx = -dy;
            ny = dx;
        } else {
            nx = dy;
            ny = -dx;
        }

        scale = fabs(nx) > fabs(ny) ? fabs(nx) : fabs(ny);
        if (scale == 0.0) continue;

        mx = (ax + bx) / 2.0;
        my = (ay + by) / 2.0;

        sx = mx + 0.25 * nx / scale;
        sy = my + 0.25 * ny / scale;

        if (point_in_cycle(sx, sy, c, lattice)) {
            *px = sx;
            *py = sy;
            return 1;
        }
    }

    /* fallback triangle */

    for (int i = 0; i < c->n; i++) {
        Coord a = c->v[(i + c->n - 1) % c->n];
        Coord b = c->v[i];
        Coord d = c->v[(i + 1) % c->n];

        double ax, ay, bx, by, dx, dy;
        double cross;

        lattice_embed_point(lattice, a, &ax, &ay);
        lattice_embed_point(lattice, b, &bx, &by);
        lattice_embed_point(lattice, d, &dx, &dy);

        cross = (bx - ax) * (dy - by) - (by - ay) * (dx - bx);
        if (cross == 0.0) continue;

        if ((area2 > 0 && cross > 0.0) ||
            (area2 < 0 && cross < 0.0)) {
            *px = (ax + bx + dx) / 3.0;
            *py = (ay + by + dy) / 3.0;
            return 1;
        }
    }

    return 0;
}
