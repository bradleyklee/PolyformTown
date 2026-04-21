#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#define MAX_LINE 65536
#define MAX_EDGES 4096
#define MAX_CYCLES 128
#define MAX_VERTS_PER_CYCLE 4096
#define CELL_MARGIN 14.0
#define STROKE_W 1.5
#define UNIT 20.0

typedef struct { int x, y; } IPoint;
typedef struct { IPoint a, b; } Edge;
typedef struct {
    int n;
    IPoint v[MAX_VERTS_PER_CYCLE];
} Path;
typedef struct {
    int cycle_count;
    Path cycles[MAX_CYCLES];
} Shape;

typedef struct { double x, y; } DPoint;

static int parse_next_edge(const char **pp, Edge *e) {
    int ax, ay, bx, by, n = 0;
    if (sscanf(*pp, " (%d,%d)->(%d,%d)%n", &ax, &ay, &bx, &by, &n) == 4) {
        e->a.x = ax; e->a.y = ay; e->b.x = bx; e->b.y = by;
        *pp += n;
        return 1;
    }
    return 0;
}

static int point_eq(IPoint a, IPoint b) {
    return a.x == b.x && a.y == b.y;
}

static DPoint lattice_to_xy(IPoint p, int g) {
    if (g == 6) {
        const double rt3 = 1.7320508075688772;
        return (DPoint){ p.x + 0.5 * p.y, 0.5 * rt3 * p.y };
    }
    return (DPoint){ (double)p.x, (double)p.y };
}

static int read_shapes(Shape **out_shapes) {
    char line[MAX_LINE];
    int cap = 64;
    int count = 0;
    Shape *shapes = (Shape *)malloc((size_t)cap * sizeof(Shape));
    if (!shapes) return -1;

    while (fgets(line, sizeof(line), stdin)) {
        const char *p = line;
        while (isspace((unsigned char)*p)) p++;
        if (*p == '\0') continue;

        int hole_flag = 0, n = 0;
        if (sscanf(p, "%d%n", &hole_flag, &n) != 1) continue;
        p += n;
        (void)hole_flag;

        Edge edges[MAX_EDGES];
        int ec = 0;
        while (ec < MAX_EDGES) {
            Edge e;
            if (!parse_next_edge(&p, &e)) break;
            edges[ec++] = e;
            while (isspace((unsigned char)*p)) p++;
        }
        if (ec == 0) continue;

        if (count >= cap) {
            cap *= 2;
            Shape *tmp = (Shape *)realloc(shapes, (size_t)cap * sizeof(Shape));
            if (!tmp) {
                free(shapes);
                return -1;
            }
            shapes = tmp;
        }

        Shape *s = &shapes[count++];
        memset(s, 0, sizeof(*s));

        int start = 0;
        while (start < ec && s->cycle_count < MAX_CYCLES) {
            int end = start + 1;
            while (end < ec && point_eq(edges[end - 1].b, edges[end].a)) end++;

            Path *path = &s->cycles[s->cycle_count++];
            path->n = 0;
            path->v[path->n++] = edges[start].a;
            for (int i = start; i < end; i++) {
                if (path->n < MAX_VERTS_PER_CYCLE) path->v[path->n++] = edges[i].b;
            }
            if (path->n > 1 && point_eq(path->v[0], path->v[path->n - 1])) path->n--;

            start = end;
        }
    }

    *out_shapes = shapes;
    return count;
}

static void shape_bbox(const Shape *s, int g, double *minx, double *miny, double *maxx, double *maxy) {
    int first = 1;
    *minx = *miny = *maxx = *maxy = 0.0;
    for (int c = 0; c < s->cycle_count; c++) {
        const Path *p = &s->cycles[c];
        for (int i = 0; i < p->n; i++) {
            DPoint q = lattice_to_xy(p->v[i], g);
            if (first) {
                *minx = *maxx = q.x;
                *miny = *maxy = q.y;
                first = 0;
            } else {
                if (q.x < *minx) *minx = q.x;
                if (q.x > *maxx) *maxx = q.x;
                if (q.y < *miny) *miny = q.y;
                if (q.y > *maxy) *maxy = q.y;
            }
        }
    }
}

static void global_bbox(const Shape *shapes, int count, int g,
                        double *minx, double *miny, double *maxx, double *maxy) {
    int first = 1;
    *minx = *miny = *maxx = *maxy = 0.0;
    for (int s = 0; s < count; s++) {
        double sx0, sy0, sx1, sy1;
        shape_bbox(&shapes[s], g, &sx0, &sy0, &sx1, &sy1);
        if (first) {
            *minx = sx0; *miny = sy0; *maxx = sx1; *maxy = sy1;
            first = 0;
        } else {
            if (sx0 < *minx) *minx = sx0;
            if (sy0 < *miny) *miny = sy0;
            if (sx1 > *maxx) *maxx = sx1;
            if (sy1 > *maxy) *maxy = sy1;
        }
    }
}

static void choose_grid(int count, double cell_w, double cell_h, int *out_rows, int *out_cols) {
    double target = 16.0 / 9.0;
    double best_score = 1e100;
    int best_r = 1, best_c = count > 0 ? count : 1;
    for (int rows = 1; rows <= count; rows++) {
        int cols = (count + rows - 1) / rows;
        double aspect = (cols * cell_w) / (rows * cell_h);
        double score = fabs(aspect - target);
        if (score < best_score) {
            best_score = score;
            best_r = rows;
            best_c = cols;
        }
    }
    *out_rows = best_r;
    *out_cols = best_c;
}

static void emit_shape_svg(FILE *fp, const Shape *s, int g,
                           double cell_x, double cell_y,
                           double cell_w, double cell_h,
                           double scale) {
    double minx, miny, maxx, maxy;
    shape_bbox(s, g, &minx, &miny, &maxx, &maxy);
    double bw = maxx - minx;
    double bh = maxy - miny;

    double tx = cell_x + (cell_w - scale * bw) * 0.5 - scale * minx;
    double ty = cell_y + (cell_h - scale * bh) * 0.5 + scale * maxy;

    fprintf(fp, "<path d=\"");
    for (int c = 0; c < s->cycle_count; c++) {
        const Path *p = &s->cycles[c];
        if (p->n <= 0) continue;
        DPoint q0 = lattice_to_xy(p->v[0], g);
        double x0 = tx + scale * q0.x;
        double y0 = ty - scale * q0.y;
        fprintf(fp, "M %.3f %.3f ", x0, y0);
        for (int i = 1; i < p->n; i++) {
            DPoint q = lattice_to_xy(p->v[i], g);
            double x = tx + scale * q.x;
            double y = ty - scale * q.y;
            fprintf(fp, "L %.3f %.3f ", x, y);
        }
        fprintf(fp, "Z ");
    }
    fprintf(fp, "\" fill=\"#dddddd\" stroke=\"black\" stroke-width=\"%.2f\" fill-rule=\"evenodd\"/>\n", STROKE_W);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s G\n", argv[0]);
        fprintf(stderr, "  G = 4 for square lattice, 6 for triangular/hex lattice\n");
        return 1;
    }

    int g = atoi(argv[1]);
    if (g != 4 && g != 6) {
        fprintf(stderr, "imgtable: G must be 4 or 6\n");
        return 1;
    }

    Shape *shapes = NULL;
    int count = read_shapes(&shapes);
    if (count < 0) {
        fprintf(stderr, "imgtable: failed to read input\n");
        return 1;
    }
    if (count == 0) {
        free(shapes);
        fprintf(stderr, "imgtable: no shapes on stdin\n");
        return 1;
    }

    double global_minx, global_miny, global_maxx, global_maxy;
    global_bbox(shapes, count, g, &global_minx, &global_miny, &global_maxx, &global_maxy);

    double global_bw = global_maxx - global_minx;
    double global_bh = global_maxy - global_miny;
    if (global_bw < 1e-9) global_bw = 1.0;
    if (global_bh < 1e-9) global_bh = 1.0;

    double cell_w = 2.0 * CELL_MARGIN + UNIT * global_bw;
    double cell_h = 2.0 * CELL_MARGIN + UNIT * global_bh;

    int rows, cols;
    choose_grid(count, cell_w, cell_h, &rows, &cols);
    double width = cols * cell_w;
    double height = rows * cell_h;

    printf("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%.0f\" height=\"%.0f\" viewBox=\"0 0 %.0f %.0f\">\n", width, height, width, height);
    printf("<rect width=\"100%%\" height=\"100%%\" fill=\"white\"/>\n");

    for (int i = 0; i < count; i++) {
        int row = i / cols;
        int col = i % cols;
        emit_shape_svg(stdout, &shapes[i], g,
                       col * cell_w, row * cell_h,
                       cell_w, cell_h, UNIT);
    }

    printf("</svg>\n");
    free(shapes);
    return 0;
}
