/* nd_mtx_test.c: host test of nd_mtx.c (plan_native_slices S1-4). Known answers for the N64 unpack, the 10 deep stack (push, load, premultiply, pop, depth cap),
 * the eye shift of a perspective matrix, and the C3D_Mtx order product. The bit equal check of the palette product against the old path
 * (libcitro3d Mtx_Multiply) runs in nd_seam_test on the ndref kind 5 records. Build with -msse2 -mfpmath=sse so the host rounds each operation to 32 bits. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nd.h"

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)

static void put_elem(uint32_t* w, int r, int c, int32_t fixed) {
    int idx = r * 2 + (c >> 1);
    uint32_t sh = (c & 1) ? 0 : 16;
    w[idx] = (w[idx] & ~(0xFFFFu << sh)) | (((uint32_t)fixed >> 16) & 0xFFFFu) << sh;
    w[idx + 8] = (w[idx + 8] & ~(0xFFFFu << sh)) | ((uint32_t)fixed & 0xFFFFu) << sh;
}

static void test_unpack(void) {
    uint32_t w[16];
    float d[4][4];
    int32_t fx[4][4] = { { 65536, -98304, 1, -1 }, { 0, 32768, -32768, 2147483647 }, { -2147483647 - 1, 7, 8, 9 }, { 123456, -654321, 0, 65536 } };
    memset(w, 0, sizeof w);
    for (int r = 0; r < 4; r++) for (int c = 0; c < 4; c++) put_elem(w, r, c, fx[r][c]);
    nd_mtx_unpack(w, d);
    for (int r = 0; r < 4; r++) for (int c = 0; c < 4; c++) CHECK(d[r][c] == (float)fx[r][c] / 65536.f, "unpack [%d][%d] %f", r, c, d[r][c]);
}

static void trans(float m[4][4], float x, float y, float z) { nd_mtx_ident(m); m[3][0] = x; m[3][1] = y; m[3][2] = z; }

static void test_stack(void) {
    float a[4][4], b[4][4];
    nd_mtx_reset();
    CHECK(nd_mv_sp == 0 && nd_mv[0][0][0] == 1.f && nd_proj[3][3] == 1.f && nd_proj[0][1] == 0.f, "reset");
    trans(a, 1, 2, 3);
    nd_mtx_load(a, ND_MTX_LOAD);
    CHECK(nd_mv_sp == 0 && nd_mv[0][3][0] == 1.f && nd_mv[0][3][2] == 3.f, "load into the top");
    trans(b, 10, 20, 30);
    nd_mtx_load(b, ND_MTX_PUSH); /* push, then premultiply: child x parent, translations add */
    CHECK(nd_mv_sp == 1 && nd_mv[1][3][0] == 11.f && nd_mv[1][3][1] == 22.f && nd_mv[1][3][2] == 33.f, "push and premultiply");
    CHECK(nd_mv[0][3][0] == 1.f, "the pushed level keeps the parent");
    nd_mtx_pop(0);
    CHECK(nd_mv_sp == 0, "pop count 0 pops one");
    nd_mtx_pop(5);
    CHECK(nd_mv_sp == 0, "pop below the bottom stays at 0");
    for (int i = 0; i < 12; i++) nd_mtx_load(a, ND_MTX_PUSH | ND_MTX_LOAD);
    CHECK(nd_mv_sp == ND_MV_DEPTH - 1, "push beyond the depth is dropped, sp %d", nd_mv_sp);
    nd_mtx_pop(3);
    CHECK(nd_mv_sp == ND_MV_DEPTH - 4, "pop 3");
    nd_mtx_load(b, ND_MTX_PROJ | ND_MTX_LOAD);
    CHECK(nd_proj[3][0] == 10.f && nd_mv_sp == ND_MV_DEPTH - 4, "projection load leaves the stack");
    nd_mtx_load(a, ND_MTX_PROJ | ND_MTX_PUSH);
    CHECK(nd_mv_sp == ND_MV_DEPTH - 4 && nd_proj[3][0] == 11.f, "projection ignores push");
}

static void test_combine(void) {
    float c[4][4];
    nd_mtx_reset();
    nd_proj[0][0] = 2.f; nd_proj[2][3] = -1.f; nd_proj[3][3] = 0.f;
    nd_mtx_combine(c, 0.f, 2.f);
    CHECK(c[2][3] == -1.f && c[2][0] == 0.f && c[3][0] == 0.f, "no shift without an eye offset");
    nd_mtx_combine(c, 0.5f, 2.f);
    CHECK(c[2][0] == -0.5f && c[3][0] == -1.f && c[0][0] == 2.f, "perspective shift: c20 %f c30 %f", c[2][0], c[3][0]);
    nd_mtx_ident(nd_proj);
    nd_mtx_combine(c, 0.5f, 2.f);
    CHECK(c[3][0] == 0.f && c[0][0] == 1.f, "an orthographic matrix stays at the screen plane");
}

static void test_cmul(void) {
    float a[16], b[16], o[16], ref[16];
    for (int i = 0; i < 16; i++) { a[i] = (float)(i * 3 % 7) - 2.f; b[i] = (float)(i * 5 % 11) - 4.f; }
    nd_mtx_cmul(o, a, b);
    for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) { float s = 0.f; for (int k = 0; k < 4; k++) s += a[4 * i + 3 - k] * b[4 * k + j]; ref[4 * i + j] = s; }
    CHECK(memcmp(o, ref, sizeof o) == 0, "cmul pairs a[i][3 - k] with b[k][j] (C3D_Mtx order, as libcitro3d Mtx_Multiply; integer values, exact)");
    nd_mtx_cmul(a, a, b); /* aliasing */
    CHECK(memcmp(a, ref, sizeof a) == 0, "cmul with out = a");
}

/* S1-5: stubs of pm_interp.c: one replacement for the key matrix */
int pm_interp_repl;
static float rep_tab[4][4];
static const void* rep_key;
const float (*pm_interp_find(const void* m))[4] { return m == rep_key ? rep_tab : 0; }

/* the same G_MTX sequence with and without the replacement table: only the replaced matrix words differ */
static void test_replace(void) {
    uint32_t n64[16], other[16];
    float in[4][4], sa[ND_MV_DEPTH][4][4], ia[4][4];
    memset(n64, 0, sizeof n64);
    memset(other, 0, sizeof other);
    for (int r = 0; r < 4; r++) { put_elem(n64, r, r, 65536); put_elem(other, r, r, 65536); }
    put_elem(n64, 3, 0, 3 * 65536); put_elem(n64, 3, 1, 4 * 65536); put_elem(n64, 3, 2, 5 * 65536);
    trans(rep_tab, 30, 40, 50);
    rep_key = n64;
    /* run A: no replacement pass */
    pm_interp_repl = 0;
    CHECK(nd_mtx_replace(n64, in) == 0, "no replacement outside the t = 0.5 pass");
    nd_mtx_reset();
    nd_mtx_unpack(n64, in);
    nd_mtx_load(in, ND_MTX_PUSH | ND_MTX_LOAD);
    memcpy(sa, nd_mv, sizeof sa); memcpy(ia, nd_proj, sizeof ia);
    int sp_a = nd_mv_sp;
    /* run B: the t = 0.5 pass */
    pm_interp_repl = 1;
    CHECK(nd_mtx_replace(other, in) == 0, "a matrix without a table entry loads its own words");
    CHECK(nd_mtx_replace(n64, in) == 1 && in[3][0] == 30.f && in[3][1] == 40.f && in[3][2] == 50.f, "the table entry replaces the matrix");
    nd_mtx_reset();
    nd_mtx_load(in, ND_MTX_PUSH | ND_MTX_LOAD);
    CHECK(nd_mv_sp == sp_a && memcmp(ia, nd_proj, sizeof ia) == 0 && memcmp(sa[0], nd_mv[0], sizeof sa[0]) == 0, "stack depth, projection and the parent level are equal");
    int d = 0;
    for (int r = 0; r < 4; r++) for (int c = 0; c < 4; c++) d += memcmp(&sa[1][r][c], &nd_mv[1][r][c], 4) != 0;
    CHECK(d == 3, "%d matrix words differ, want the 3 translation words", d);
    pm_interp_repl = 0;
}

/* light block: values of the RSP table to the pm5 layout */
static void test_lights(void) {
    NdLight lt[9];
    int8_t lk[2][3] = { { 127, -127, 0 }, { 5, 0, -64 } };
    PmFx fx;
    memset(lt, 0, sizeof lt);
    for (int i = 0; i < 9; i++) { lt[i].col[0] = (uint8_t)(10 + i); lt[i].col[1] = (uint8_t)(20 + i); lt[i].col[2] = (uint8_t)(200 + i); lt[i].dir[0] = (int8_t)(i * 10 - 40); lt[i].dir[1] = -127; lt[i].dir[2] = 127; }
    nd_mtx_reset();
    nd_lights(&fx, 1, lt, 3, lk);
    CHECK(fx.lflag[0] == 1.f && fx.lflag[1] == 1.f && fx.lflag[2] == 0.f, "light flags");
    CHECK(fx.amb[0] == 13.f && fx.amb[1] == 23.f && fx.amb[2] == 203.f && fx.amb[3] == 0.f, "ambient is slot n");
    CHECK(fx.col[2][2] == 202.f && fx.col[3][0] == 0.f, "colour of 3 lights");
    CHECK(fx.dir[1][0] == (float)-30 / 127.f && fx.dir[1][1] == -1.f && fx.dir[1][2] == 1.f, "direction over 127");
    CHECK(fx.lk0[0] == 1.f && fx.lk0[1] == -1.f && fx.lk1[0] == (float)5 / 127.f && fx.lk1[2] == (float)-64 / 127.f, "look-at vectors");
    nd_lights(&fx, 0, lt, 8, lk);
    CHECK(fx.lflag[1] == 0.f && fx.col[6][0] == 16.f && fx.amb[0] == 18.f, "more than 7 lights: 7 kept, ambient is slot 8");
}

int main(void) {
    test_unpack();
    test_stack();
    test_combine();
    test_cmul();
    test_replace();
    test_lights();
    puts(fails ? "nd_mtx: FAIL" : "nd_mtx: PASS");
    return fails != 0;
}
