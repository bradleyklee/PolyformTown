#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#define MAX_LINE 65536
#define MAX_SHAPES 512
#define MAX_CONSTS 32
#define MAX_NAME 32
#define MAX_EXPR 128
#define MAX_BLOCK 4096
#define MAX_BASES 16
#define MAX_CYCLES 32
#define MAX_VERTS_PER_CYCLE 4096
#define CELL_MARGIN 14.0
#define STROKE_W 1.5
#define UNIT 20.0

typedef struct { char name[MAX_NAME]; char expr[MAX_EXPR]; double value; int have_value; } Constant;
typedef struct { int valence; char e11[MAX_EXPR], e12[MAX_EXPR], e21[MAX_EXPR], e22[MAX_EXPR]; double a11, a12, a21, a22; } Basis;
typedef struct { int v, x, y; } VPoint;
typedef struct { int n; VPoint v[MAX_VERTS_PER_CYCLE]; } Path;
typedef struct {
    int hole_flag;
    int constant_count;
    Constant constants[MAX_CONSTS];
    int basis_count;
    Basis bases[MAX_BASES];
    int cycle_count;
    Path cycles[MAX_CYCLES];
} Shape;
typedef struct { double x, y; } DPoint;

typedef struct {
    const char *s;
    const Shape *shape;
} ExprParser;

static void skip_ws(const char **pp) {
    while (isspace((unsigned char)**pp)) (*pp)++;
}

static int parse_int(const char **pp, int *out) {
    char *end;
    long v;
    skip_ws(pp);
    v = strtol(*pp, &end, 10);
    if (end == *pp) return 0;
    *out = (int)v;
    *pp = end;
    return 1;
}

static int expect_char(const char **pp, char ch) {
    skip_ws(pp);
    if (**pp != ch) return 0;
    (*pp)++;
    return 1;
}

static void copy_trim(char *dst, size_t dstsz, const char *src, size_t len) {
    while (len > 0 && isspace((unsigned char)*src)) {
        src++;
        len--;
    }
    while (len > 0 && isspace((unsigned char)src[len - 1])) len--;
    if (len >= dstsz) len = dstsz - 1;
    memcpy(dst, src, len);
    dst[len] = '\0';
}

static int split_top_level(const char *src, char delim, const char **out_left, size_t *out_left_len,
                           const char **out_right, size_t *out_right_len) {
    int depth = 0;
    for (const char *p = src; *p; p++) {
        if (*p == '(') depth++;
        else if (*p == ')' && depth > 0) depth--;
        else if (*p == delim && depth == 0) {
            *out_left = src;
            *out_left_len = (size_t)(p - src);
            *out_right = p + 1;
            *out_right_len = strlen(p + 1);
            return 1;
        }
    }
    return 0;
}

static int extract_paren_block(const char **pp, char *dst, size_t dstsz) {
    const char *start;
    const char *p;
    int depth = 0;
    skip_ws(pp);
    if (**pp != '(') return 0;
    (*pp)++;
    start = *pp;
    p = *pp;
    while (*p) {
        if (*p == '(') depth++;
        else if (*p == ')') {
            if (depth == 0) {
                copy_trim(dst, dstsz, start, (size_t)(p - start));
                *pp = p + 1;
                return 1;
            }
            depth--;
        }
        p++;
    }
    return 0;
}

static int is_factor_start(char c) {
    return c == '(' || c == '.' || c == '+' || c == '-' || isdigit((unsigned char)c) || isalpha((unsigned char)c) || c == '_';
}

static Constant *find_constant(const Shape *shape, const char *name) {
    for (int i = 0; i < shape->constant_count; i++) {
        if (strcmp(shape->constants[i].name, name) == 0) return (Constant *)&shape->constants[i];
    }
    return NULL;
}

static double parse_expr_sum(ExprParser *p);

static double eval_constant(Constant *c, const Shape *shape) {
    if (c->have_value) return c->value;
    ExprParser p = { c->expr, shape };
    c->value = parse_expr_sum(&p);
    c->have_value = 1;
    return c->value;
}

static double parse_expr_atom(ExprParser *p) {
    double v;
    char name[MAX_NAME];
    int n = 0;

    while (isspace((unsigned char)*p->s)) p->s++;

    if (*p->s == '(') {
        p->s++;
        v = parse_expr_sum(p);
        while (isspace((unsigned char)*p->s)) p->s++;
        if (*p->s == ')') p->s++;
        return v;
    }

    if (isdigit((unsigned char)*p->s) || *p->s == '.') {
        char *end;
        v = strtod(p->s, &end);
        p->s = end;
        return v;
    }

    if (isalpha((unsigned char)*p->s) || *p->s == '_') {
        while ((isalpha((unsigned char)p->s[n]) || isdigit((unsigned char)p->s[n]) || p->s[n] == '_') && n < MAX_NAME - 1) {
            name[n] = p->s[n];
            n++;
        }
        name[n] = '\0';
        p->s += n;
        if (strcmp(name, "sqrt") == 0) {
            while (isspace((unsigned char)*p->s)) p->s++;
            if (*p->s == '(') {
                p->s++;
                v = parse_expr_sum(p);
                while (isspace((unsigned char)*p->s)) p->s++;
                if (*p->s == ')') p->s++;
                return sqrt(v);
            }
            return 0.0;
        }
        Constant *c = find_constant(p->shape, name);
        if (c) return eval_constant(c, p->shape);
        return 0.0;
    }

    return 0.0;
}

static double parse_expr_unary(ExprParser *p) {
    while (isspace((unsigned char)*p->s)) p->s++;
    if (*p->s == '+') {
        p->s++;
        return parse_expr_unary(p);
    }
    if (*p->s == '-') {
        p->s++;
        return -parse_expr_unary(p);
    }
    return parse_expr_atom(p);
}

static double parse_expr_product(ExprParser *p) {
    double v = parse_expr_unary(p);
    for (;;) {
        while (isspace((unsigned char)*p->s)) p->s++;
        if (*p->s == '*') {
            p->s++;
            v *= parse_expr_unary(p);
            continue;
        }
        if (*p->s == '/') {
            p->s++;
            v /= parse_expr_unary(p);
            continue;
        }
        if (is_factor_start(*p->s) && *p->s != ')' && *p->s != ',' && *p->s != ';' && *p->s != '|') {
            v *= parse_expr_unary(p);
            continue;
        }
        break;
    }
    return v;
}

static double parse_expr_sum(ExprParser *p) {
    double v = parse_expr_product(p);
    for (;;) {
        while (isspace((unsigned char)*p->s)) p->s++;
        if (*p->s == '+') {
            p->s++;
            v += parse_expr_product(p);
            continue;
        }
        if (*p->s == '-') {
            p->s++;
            v -= parse_expr_product(p);
            continue;
        }
        break;
    }
    return v;
}

static double eval_expr_text(const Shape *shape, const char *expr) {
    ExprParser p = { expr, shape };
    return parse_expr_sum(&p);
}

static Basis *find_basis(const Shape *shape, int valence) {
    for (int i = 0; i < shape->basis_count; i++) {
        if (shape->bases[i].valence == valence) return (Basis *)&shape->bases[i];
    }
    return NULL;
}

static DPoint vertex_to_xy(const Shape *shape, VPoint p) {
    Basis *b = find_basis(shape, p.v);
    if (!b) return (DPoint){ (double)p.x, (double)p.y };
    return (DPoint){ b->a11 * p.x + b->a21 * p.y, b->a12 * p.x + b->a22 * p.y };
}

static int parse_constants_block(const char **pp, Shape *s) {
    char block[MAX_BLOCK];
    if (!extract_paren_block(pp, block, sizeof(block))) return 0;
    if (!block[0]) return 1;

    const char *cur = block;
    while (*cur && s->constant_count < MAX_CONSTS) {
        const char *lhs, *rhs;
        size_t lhs_len, rhs_len;
        char item[MAX_BLOCK];
        const char *comma;
        int depth = 0;
        for (comma = cur; *comma; comma++) {
            if (*comma == '(') depth++;
            else if (*comma == ')' && depth > 0) depth--;
            else if (*comma == ',' && depth == 0) break;
        }
        copy_trim(item, sizeof(item), cur, (size_t)(comma - cur));
        if (!split_top_level(item, '=', &lhs, &lhs_len, &rhs, &rhs_len)) return 0;
        copy_trim(s->constants[s->constant_count].name, sizeof(s->constants[s->constant_count].name), lhs, lhs_len);
        copy_trim(s->constants[s->constant_count].expr, sizeof(s->constants[s->constant_count].expr), rhs, rhs_len);
        s->constants[s->constant_count].have_value = 0;
        s->constant_count++;
        cur = *comma ? comma + 1 : comma;
    }
    return 1;
}

static int read_basis_expr(const char **pp, char *dst, size_t dstsz, char stop) {
    const char *start;
    int depth = 0;
    skip_ws(pp);
    start = *pp;
    while (**pp) {
        char ch = **pp;
        if (ch == '(') depth++;
        else if (ch == ')' && depth > 0) depth--;
        else if (ch == stop && depth == 0) break;
        (*pp)++;
    }
    copy_trim(dst, dstsz, start, (size_t)(*pp - start));
    return dst[0] != '\0';
}

static int read_basis_expr_last(const char **pp, char *dst, size_t dstsz) {
    const char *start;
    int depth = 0;
    skip_ws(pp);
    start = *pp;
    while (**pp) {
        char ch = **pp;
        if (ch == '(') depth++;
        else if (ch == ')' && depth > 0) depth--;
        else if (ch == ',' && depth == 0) {
            const char *q = *pp + 1;
            skip_ws(&q);
            if (isdigit((unsigned char)*q) || *q == '-') break;
        }
        (*pp)++;
    }
    copy_trim(dst, dstsz, start, (size_t)(*pp - start));
    return dst[0] != '\0';
}

static int parse_basis_block(const char **pp, Shape *s) {
    char block[MAX_BLOCK];
    const char *p;
    if (!extract_paren_block(pp, block, sizeof(block))) return 0;
    if (!block[0]) return 1;

    p = block;
    while (*p && s->basis_count < MAX_BASES) {
        Basis *b = &s->bases[s->basis_count];
        if (!parse_int(&p, &b->valence)) return 0;
        if (!expect_char(&p, ':')) return 0;
        if (!read_basis_expr(&p, b->e11, sizeof(b->e11), ',')) return 0;
        if (!expect_char(&p, ',')) return 0;
        if (!read_basis_expr(&p, b->e12, sizeof(b->e12), ';')) return 0;
        if (!expect_char(&p, ';')) return 0;
        if (!read_basis_expr(&p, b->e21, sizeof(b->e21), ',')) return 0;
        if (!expect_char(&p, ',')) return 0;
        if (!read_basis_expr_last(&p, b->e22, sizeof(b->e22))) return 0;
        s->basis_count++;
        skip_ws(&p);
        if (*p == ',') {
            p++;
            continue;
        }
        if (*p == '\0') return 1;
        return 0;
    }
    return 1;
}

static int parse_cycle_block(const char **pp, Path *path) {
    char block[MAX_BLOCK];
    if (!extract_paren_block(pp, block, sizeof(block))) return 0;
    if (!block[0]) return 1;
    const char *p = block;
    while (path->n < MAX_VERTS_PER_CYCLE) {
        VPoint *v = &path->v[path->n];
        if (!parse_int(&p, &v->v) || !parse_int(&p, &v->x) || !parse_int(&p, &v->y)) return 0;
        path->n++;
        skip_ws(&p);
        if (*p == ',') {
            p++;
            continue;
        }
        if (*p == '\0') return 1;
        return 0;
    }
    return 0;
}

static int parse_shape_line(const char *line, Shape *s) {
    const char *p = line;
    memset(s, 0, sizeof(*s));
    if (!expect_char(&p, '[')) return 0;
    if (!parse_int(&p, &s->hole_flag)) return 0;
    if (!expect_char(&p, '|')) return 0;
    if (!parse_constants_block(&p, s)) return 0;
    if (!expect_char(&p, '|')) return 0;
    if (!parse_basis_block(&p, s)) return 0;

    while (s->cycle_count < MAX_CYCLES) {
        if (!expect_char(&p, '|')) return 0;
        if (!parse_cycle_block(&p, &s->cycles[s->cycle_count])) return 0;
        s->cycle_count++;
        skip_ws(&p);
        if (*p == ']') {
            p++;
            break;
        }
    }
    if (s->hole_flag == 0 && s->cycle_count != 1) return 0;
    if (s->hole_flag == 1 && s->cycle_count < 2) return 0;

    for (int i = 0; i < s->basis_count; i++) {
        s->bases[i].a11 = eval_expr_text(s, s->bases[i].e11);
        s->bases[i].a12 = eval_expr_text(s, s->bases[i].e12);
        s->bases[i].a21 = eval_expr_text(s, s->bases[i].e21);
        s->bases[i].a22 = eval_expr_text(s, s->bases[i].e22);
    }
    return 1;
}

static int read_shapes(Shape *shapes, int max_shapes) {
    char line[MAX_LINE];
    int count = 0;
    while (count < max_shapes && fgets(line, sizeof(line), stdin)) {
        const char *p = line;
        while (isspace((unsigned char)*p)) p++;
        if (!*p) continue;
        if (!parse_shape_line(p, &shapes[count])) return -1;
        count++;
    }
    return count;
}

static void shape_bbox(const Shape *s, double *minx, double *miny, double *maxx, double *maxy) {
    int first = 1;
    *minx = *miny = *maxx = *maxy = 0.0;
    for (int c = 0; c < s->cycle_count; c++) {
        const Path *p = &s->cycles[c];
        for (int i = 0; i < p->n; i++) {
            DPoint q = vertex_to_xy(s, p->v[i]);
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

static void global_bbox(const Shape *shapes, int count, double *minx, double *miny, double *maxx, double *maxy) {
    int first = 1;
    *minx = *miny = *maxx = *maxy = 0.0;
    for (int s = 0; s < count; s++) {
        double sx0, sy0, sx1, sy1;
        shape_bbox(&shapes[s], &sx0, &sy0, &sx1, &sy1);
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

static void emit_shape_svg(FILE *fp, const Shape *s,
                           double cell_x, double cell_y,
                           double cell_w, double cell_h,
                           double scale) {
    double minx, miny, maxx, maxy;
    shape_bbox(s, &minx, &miny, &maxx, &maxy);
    double bw = maxx - minx;
    double bh = maxy - miny;

    double tx = cell_x + (cell_w - scale * bw) * 0.5 - scale * minx;
    double ty = cell_y + (cell_h - scale * bh) * 0.5 + scale * maxy;

    fprintf(fp, "<path d=\"");
    for (int c = 0; c < s->cycle_count; c++) {
        const Path *p = &s->cycles[c];
        if (p->n <= 0) continue;
        DPoint q0 = vertex_to_xy(s, p->v[0]);
        double x0 = tx + scale * q0.x;
        double y0 = ty - scale * q0.y;
        fprintf(fp, "M %.3f %.3f ", x0, y0);
        for (int i = 1; i < p->n; i++) {
            DPoint q = vertex_to_xy(s, p->v[i]);
            double x = tx + scale * q.x;
            double y = ty - scale * q.y;
            fprintf(fp, "L %.3f %.3f ", x, y);
        }
        fprintf(fp, "Z ");
    }
    fprintf(fp, "\" fill=\"#dddddd\" stroke=\"black\" stroke-width=\"%.2f\" fill-rule=\"evenodd\"/>\n", STROKE_W);
}

int main(void) {
    Shape *shapes = (Shape *)calloc(MAX_SHAPES, sizeof(Shape));
    if (!shapes) {
        fprintf(stderr, "imgtable: out of memory\n");
        return 1;
    }
    int count = read_shapes(shapes, MAX_SHAPES);
    if (count < 0) {
        fprintf(stderr, "imgtable: failed to parse input\n");
        free(shapes);
        return 1;
    }
    if (count == 0) {
        fprintf(stderr, "imgtable: no shapes on stdin\n");
        free(shapes);
        return 1;
    }

    double global_minx, global_miny, global_maxx, global_maxy;
    global_bbox(shapes, count, &global_minx, &global_miny, &global_maxx, &global_maxy);

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
        emit_shape_svg(stdout, &shapes[i],
                       col * cell_w, row * cell_h,
                       cell_w, cell_h, UNIT);
    }

    printf("</svg>\n");
    free(shapes);
    return 0;
}
