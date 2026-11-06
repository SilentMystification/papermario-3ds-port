/* BG seam host test (row BG1 + BG2, oracle plan 2). Host C, no 3DS code: it copies the arithmetic of
 * src/background.c (bg_emit, bg_texrect, bg_sky_rows), 3ds/src/pm_gbi.c (gbi_texrect COPY) and
 * 3ds/src/pm_gpu.c (pm_gpu_tex_rect: the F07-7 edge rule and the linear s over the pixel span).
 *
 * For each sky configuration it computes the texel every screen pixel shows and compares it with the
 * ideal texel from the inverse zoom. The sky is a product (x and y are separate), so the x check uses one
 * row and the y check one column. A pixel is a mismatch when no rectangle covers it, or the sampled texel
 * differs from the ideal texel by 0.5 or more (distance on the texel circle; an exact texel boundary is
 * not a mismatch). The GPU pad is edge-clamped (pm_tex.c), so samples past the edge read the edge texel.
 *
 * Switches: argv[1] = gbi fix (1: px0 = (xl+3)>>2, px1 = xh>>2, s and t moved to the pixel edge),
 *           argv[2] = fdiv5 (1: floor division by 5 in the zoom of bg_emit; 2: also the inclusive edge minus 1),
 *           argv[3] = F07-7 (1: pm_gpu_tex_rect extends game edge rects, as HEAD does; 0 off).
 * Prints the mismatch count and the first 12 mismatch lines. Exit 0 when there are none.
 *
 * Build and run (docker, no local compiler):
 *   docker run --rm -v <repo>/3ds/tests:/t pm3ds-host:1 sh -c "gcc -O1 -o /tmp/bst /t/bg_seam_test.c -lm && /tmp/bst 0 0 1"
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef int s32;

#define MINX 12
#define MAXX 296
#define MINY 20
#define MAXY 200
#define NSCR 400
#define NROW 240

typedef struct { s32 xl, yl, xh, yh, s, t, dsdx, dtdy; } Rect;

static int gfix = 0, f5 = 0, f7 = 1;
static Rect list[64];
static int nlist = 0;
static int bg_zoom = 0;

/* floor division by 5 of the zoom (oracle fix 2); C division truncates toward zero */
static s32 fdiv5(s32 v) { return v >= 0 ? v / 5 : -((-v + 4) / 5); }

/* bg_emit (src/background.c): zoom the edges and the steps, record the GPU call */
static void bg_emit(s32 xl, s32 yl, s32 xh, s32 yh, s32 s, s32 t, s32 dsdx, s32 dtdy) {
    if (bg_zoom) {
        if (f5) {
            int inc = f5 == 2 ? 1 : 4; /* f5 2: the inclusive last quarter pixel is the exclusive edge minus 1 (contiguous with the next rect) */
            xl = fdiv5((xl - 640) * 6) + 640;
            xh = fdiv5((xh + 4 - 640) * 6) + 640 - inc;
            yl = fdiv5((yl - 80) * 6);
            yh = fdiv5((yh + 4 - 80) * 6) - inc;
        } else {
            xl = (xl - 640) * 6 / 5 + 640;
            xh = (xh + 4 - 640) * 6 / 5 + 640 - 4;
            yl = (yl - 80) * 6 / 5;
            yh = (yh + 4 - 80) * 6 / 5 - 4;
        }
        dsdx = dsdx * 5 / 6;
        dtdy = dtdy * 5 / 6;
    }
    if (nlist < 64) {
        list[nlist].xl = xl; list[nlist].yl = yl; list[nlist].xh = xh; list[nlist].yh = yh;
        list[nlist].s = s; list[nlist].t = t; list[nlist].dsdx = dsdx; list[nlist].dtdy = dtdy;
        nlist++;
    }
}

/* bg_texrect: the rectangle and its copies at +-296 px (wide), clipped at the screen edges */
static void bg_texrect(s32 xl, s32 yl, s32 xh, s32 yh, s32 s, s32 t, s32 dsdx, s32 dtdy) {
    s32 shift;
    s32 lo = bg_zoom ? -8 * 4 : -40 * 4;
    s32 hi = bg_zoom ? 328 * 4 : 360 * 4;
    bg_emit(xl, yl, xh, yh, s, t, dsdx, dtdy);
    for (shift = -296 * 4; shift <= 296 * 4; shift += 592 * 4) {
        if (xh + shift >= lo && xl + shift <= hi)
            bg_emit(xl + shift, yl, xh + shift > hi - 1 ? hi - 1 : xh + shift, yh, s, t, dsdx, dtdy);
    }
}

/* bg_sky_rows: rows row0..row0+rows-1 from source row t/32; left piece and right piece of the scroll */
static void bg_sky_rows(s32 xOff, s32 row0, s32 rows, s32 t) {
    s32 yl, yh;
    if (rows <= 0) return;
    yl = (MINY + row0) * 4;
    yh = (MINY + row0 + rows - 1) * 4;
    bg_texrect(MINX * 4, yl, (xOff + MINX - 1) * 4, yh, (MAXX - xOff) * 32, t, 4096, 1024);
    bg_texrect((xOff + MINX) * 4, yl, (MAXX + MINX - 1) * 4, yh, 0, t, 4096, 1024);
}

/* gbi_texrect COPY (pm_gbi.c), one axis: the pixel span [a, b] (inclusive) and the texel at the left
 * edge (s0) and the right edge (s1) of the span. copy4: dsdx counts 4 texels per pixel (x only). */
typedef struct { int a, b; double s0, s1; int ok; } Span;

static Span gbi_axis(s32 lo_q, s32 hi_q, s32 st, s32 step, int copy4) {
    Span r;
    double s0 = (double)st / 32.0;
    double dstep = (double)step / 1024.0 / (copy4 ? 4.0 : 1.0);
    double width = (double)(hi_q - lo_q) / 4.0 + 1.0;
    if (!gfix) {
        int a = lo_q / 4, b = hi_q / 4; /* C division toward zero */
        r.a = a; r.b = b; r.ok = (b >= a);
        r.s0 = s0; r.s1 = s0 + dstep * width;
    } else {
        int a = (lo_q + 3) >> 2, b = hi_q >> 2; /* arithmetic shifts: floor */
        r.a = a; r.b = b; r.ok = (b >= a);
        r.s0 = s0 + dstep * ((double)(a * 4 - lo_q) * 0.25);
        r.s1 = r.s0 + dstep * (double)(b - a + 1);
    }
    return r;
}

/* the texel at sample position X + 0.5 of the quad of span sp, with the F07-7 rule of pm_gpu_tex_rect
 * (wide): off is 40 for columns (game x 0 is screen 40), 0 for rows. -1: not covered */
static double sample_span(Span sp, int X, int wide_edge_on, int off) {
    double xa = sp.a + (double)off, xb = sp.b + 1.0 + (double)off;
    double s0 = sp.s0, s1 = sp.s1;
    double ds;
    if (xb <= xa) return -1.0;
    ds = (s1 - s0) / (xb - xa);
    if (wide_edge_on && off == 40 && (sp.a == 0 || sp.b == 319)) {
        if (sp.a == 0) { s0 -= ds * xa; xa = 0.0; }
        if (sp.b == 319) { s1 += ds * (400.0 - xb); xb = 400.0; }
        ds = (s1 - s0) / (xb - xa);
    }
    if (X + 0.5 < xa || X + 0.5 >= xb) return -1.0;
    return s0 + ds * (X + 0.5 - xa);
}

/* the GPU reads the image: texels past the edge read the edge texel (pad is edge-clamped) */
static double clampt(double v, int n) {
    if (v < 0.0) return 0.0;
    if (v >= n) return (double)n - 0.001;
    return v;
}

/* signed distance on the circle of length m */
static double wrapd(double d, double m) { return d - m * floor(d / m + 0.5); }
static double fmodp(double a, double m) { double r = fmod(a, m); return r < 0 ? r + m : r; }

/* x check: one xOff, one zoom flag, all screen columns (row independent) */
static int check_x(int xOff, int zoomflag, int print, int *first) {
    int bad = 0, X;
    bg_zoom = zoomflag;
    nlist = 0;
    bg_sky_rows(xOff, 0, MAXY, 0);
    for (X = 0; X < NSCR; X++) {
        double g = X - 40 + 0.5; /* game frame, zoom centre 160 */
        double p = zoomflag ? (g - 160.0) / 1.2 + 160.0 : g;
        double ideal = fmodp(p - MINX - xOff, (double)MAXX);
        double got = -1.0, d;
        int covered = 0, i;
        for (i = 0; i < nlist; i++) {
            Rect* r = &list[i];
            Span sp = gbi_axis(r->xl, r->xh, r->s, r->dsdx, 1);
            double v;
            if (!sp.ok) continue;
            v = sample_span(sp, X, f7, 40);
            if (v >= 0.0) { got = v; covered = 1; }
        }
        if (!covered) {
            if (print && *first < 12) { printf("  x xOff %d zoom %d col %d uncovered, ideal %.3f\n", xOff, zoomflag, X, ideal); (*first)++; }
            bad++;
            continue;
        }
        d = wrapd(clampt(got, MAXX) - ideal, MAXX);
        if (fabs(d) >= 0.5) {
            if (print && *first < 12) { printf("  x xOff %d zoom %d col %d got %.3f ideal %.3f d %.3f\n", xOff, zoomflag, X, got, ideal, d); (*first)++; }
            bad++;
        }
    }
    return bad;
}

/* y check: one y0, one zoom flag, all screen rows (column independent) */
static int check_y(int y0, int zoomflag, int print, int *first) {
    int bad = 0, R;
    int h = MAXY;
    bg_zoom = zoomflag;
    nlist = 0;
    bg_sky_rows(0, 0, h - y0, y0 * 32);
    bg_sky_rows(0, h - y0, y0, 0);
    for (R = 0; R < NROW; R++) {
        double g = R + 0.5;
        double q = zoomflag ? g / 1.2 + MINY : g; /* zoom centre row 20 maps to row 0 (bg_emit) */
        double ideal, got = -1.0, d;
        int covered = 0, i;
        if (!zoomflag && (q < MINY || q >= MINY + h)) continue;
        ideal = fmodp(q - MINY + y0, (double)h);
        for (i = 0; i < nlist; i++) {
            Rect* r = &list[i];
            Span sp = gbi_axis(r->yl, r->yh, r->t, r->dtdy, 0);
            double v;
            if (!sp.ok) continue;
            v = sample_span(sp, R, 0, 0);
            if (v >= 0.0) { got = v; covered = 1; }
        }
        if (!covered) {
            if (print && *first < 12) { printf("  y y0 %d zoom %d row %d uncovered, ideal %.3f\n", y0, zoomflag, R, ideal); (*first)++; }
            bad++;
            continue;
        }
        d = wrapd(clampt(got, h) - ideal, h);
        if (fabs(d) >= 0.5) {
            if (print && *first < 12) { printf("  y y0 %d zoom %d row %d got %.3f ideal %.3f d %.3f\n", y0, zoomflag, R, got, ideal, d); (*first)++; }
            bad++;
        }
    }
    return bad;
}

int main(int argc, char** argv) {
    int xo, z, y0, total_x = 0, total_y = 0, first = 0;
    if (argc > 1) gfix = atoi(argv[1]);
    if (argc > 2) f5 = atoi(argv[2]);
    if (argc > 3) f7 = atoi(argv[3]);
    for (z = 0; z < 2; z++)
        for (xo = 0; xo < MAXX; xo++) total_x += check_x(xo, z, first < 12, &first);
    first = 0;
    for (z = 0; z < 2; z++)
        for (y0 = 0; y0 < MAXY; y0++) total_y += check_y(y0, z, first < 12, &first);
    printf("bg_seam_test gbi_fix %d fdiv5 %d f07_7 %d: x mismatches %d, y mismatches %d\n", gfix, f5, f7, total_x, total_y);
    return (total_x + total_y) == 0 ? 0 : 1;
}
