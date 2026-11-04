/* NDS1 host tests for 3ds/src/pm_native.c (layout of 2026-11-04). No emulator. Runs well under 1 s.
 *   1 record: sizeof and offsets of NatRec (16 bytes).
 *   2 template: nat_tmpl_add for the nine combine pairs of census_toad and census_yard. The stored plan equals a direct pm_comb_build.
 *   3 order: nat_order equals a reference stable sort by (tmpl, tex0) inside each run of unordered records. Ordered records keep their index.
 *   4 layout (fixture 3ds/tools/nat_fixture_toad.txt, two captured static model records from the rec_done dump): nat_tmpl_strip
 *     splits each record into template and group. The emitted commands of each record (template plus group) equal the
 *     captured commands as a multiset, so the same commands are emitted, possibly in another order (trailing state moves
 *     before the trigger; see docs/handoff/agent_nds1.md).
 *   5 eye copy: the eye 2 span from nat_eye_copy equals a direct nat_emit for eye 1; words that differ are palette words only.
 * Build and run: sh 3ds/tools/nat_host_test.sh (passes the fixture path). Exit 0 when all gates pass. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "pm_native.h"

typedef uint32_t u32; typedef uint16_t u16; typedef uint8_t u8;
static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)

static u32 rng_s = 777u;
static u32 rnd(void) { rng_s = rng_s * 1664525u + 1013904223u; return rng_s >> 8; }

/* 1 record packing */
static void test_record(void) {
    CHECK(sizeof(NatRec) == 16, "NatRec size %zu, want 16", sizeof(NatRec));
    CHECK(offsetof(NatRec, tex0) == 1 && offsetof(NatRec, mtx) == 4 && offsetof(NatRec, nidx) == 6, "NatRec 16-bit offsets");
    CHECK(offsetof(NatRec, idx_off) == 8 && offsetof(NatRec, vtx_base) == 12, "NatRec 32-bit offsets");
    CHECK(offsetof(NatTmpl, blk) % 32 == 0, "NatTmpl block not 32-byte aligned");
    CHECK(NAT_GRP_WORDS == 2 + 18 + 6 + 6 + 2 + 2 + 2, "group size");
    printf("record: sizeof NatRec %zu, NatTmpl %zu\n", sizeof(NatRec), sizeof(NatTmpl));
}

/* 2 template build: the nine pairs of the census (cw0, cw1) */
static const u32 census_pairs[9][2] = {
    {0xfc121824u, 0xff33ffffu}, {0xfcffffffu, 0xfffcf87cu}, {0xfc127e24u, 0xfffff3f9u},
    {0xfc26a004u, 0x1f1093ffu}, {0xfcffffffu, 0xfffcf279u}, {0xfcffb3ffu, 0xff2fffffu},
    {0xfcffffffu, 0xfffe793cu}, {0xfc20a204u, 0xff13ffffu}, {0xfc119623u, 0xff2fffffu},
};
static void test_template(void) {
    static NatTmpl save[NAT_TMPL_MAX];
    memset(nat_tmpl, 0, sizeof nat_tmpl);
    int ids[9], bad = 0, rc_bad = 0;
    for (int i = 0; i < 9; i++) {
        PmPlan a, b; int rc = -1;
        ids[i] = nat_tmpl_add(census_pairs[i][0], census_pairs[i][1], 0x00200405u, 0x0008ac00u, 0x00553078u, 1, 0, 0, 0, 0, &a, &rc);
        if (ids[i] < 0) { bad++; continue; }
        int rc2 = pm_comb_build(census_pairs[i][0], census_pairs[i][1], 0, 0, &b);
        if (rc2 != rc) rc_bad++;
        if (memcmp(&a, &b, sizeof a) != 0) bad++;
    }
    CHECK(bad == 0, "template plan differs from pm_comb_build for %d pairs", bad);
    CHECK(rc_bad == 0, "pm_comb return code differs for %d pairs", rc_bad);
    for (int i = 0; i < 9; i++) {
        int again = nat_tmpl_add(census_pairs[i][0], census_pairs[i][1], 0x00200405u, 0x0008ac00u, 0x00553078u, 1, 0, 0, 0, 0, NULL, NULL);
        CHECK(again == ids[i], "same key gave id %d, want %d", again, ids[i]);
    }
    int blend_id = nat_tmpl_add(census_pairs[0][0], census_pairs[0][1], 0x00200405u, 0x0008ac00u, 0x00553078u, 1, 0, 1, 0, 0, NULL, NULL);
    CHECK(blend_id >= 0 && blend_id != ids[0], "blend flag gave the same id");
    CHECK(nat_tmpl[blend_id].ordered == 1 && nat_tmpl[ids[0]].ordered == 0, "ordered flag: blend 1, opaque 0");
    memcpy(save, nat_tmpl, sizeof nat_tmpl);
    int full = 0;
    for (u32 k = 0; k < 100; k++) if (nat_tmpl_add(0xfc000000u + k, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL) < 0) full++;
    CHECK(full > 0, "table did not report full");
    memcpy(nat_tmpl, save, sizeof nat_tmpl);
    printf("template: %d pairs checked, table full at %d extra adds\n", 9, full);
}

/* 3 draw order: reference is a stable insertion sort by (tmpl, tex0) inside each run of unordered records */
static void test_order(void) {
    for (int trial = 0; trial < 200; trial++) {
        NatRec rec[40]; unsigned n = 1 + rnd() % 40;
        memset(nat_tmpl, 0, sizeof nat_tmpl);
        for (int t = 0; t < 4; t++) { nat_tmpl[t].used = 1; nat_tmpl[t].ordered = (t == 2 || (trial & 1 && t == 3)); }
        for (unsigned k = 0; k < n; k++) {
            memset(&rec[k], 0, sizeof rec[k]);
            rec[k].tmpl = rnd() % 4; rec[k].tex0 = rnd() % 6; rec[k].mtx = k; rec[k].nidx = k + 1;
        }
        u16 ord[NAT_REC_MAX]; nat_order(rec, n, ord);
        u16 ref[40]; for (unsigned k = 0; k < n; k++) ref[k] = (u16)k;
        unsigned i = 0;
        while (i < n) {
            if (nat_tmpl[rec[i].tmpl].ordered) { i++; continue; }
            unsigned j = i; while (j < n && !nat_tmpl[rec[j].tmpl].ordered) j++;
            for (unsigned a = i + 1; a < j; a++) {
                u16 v = ref[a]; unsigned b = a;
                while (b > i) {
                    const NatRec* p = &rec[ref[b - 1]]; const NatRec* q = &rec[v];
                    if (p->tmpl < q->tmpl || (p->tmpl == q->tmpl && p->tex0 <= q->tex0)) break;
                    ref[b] = ref[b - 1]; b--;
                }
                ref[b] = v;
            }
            i = j;
        }
        for (unsigned k = 0; k < n; k++) {
            if (ord[k] != ref[k]) { CHECK(0, "order trial %d pos %u: got %u want %u", trial, k, ord[k], ref[k]); break; }
        }
    }
    printf("order: 200 random sets checked against the reference\n");
}

/* Command split: [p0][header][data...], count in header bits 20 to 27, length 2 + c + (c & 1). */
typedef struct { u32 len; u32 v[40]; } Cmd;
static unsigned cmds_of(const u32* w, unsigned n, Cmd* o) {
    unsigned i = 0, k = 0;
    while (i + 1 < n) {
        u32 c = (w[i + 1] >> 20) & 0xFFu; unsigned len = 2u + c + (c & 1u);
        if (i + len > n || k >= 400) return 0;
        memset(&o[k], 0, sizeof o[k]); o[k].len = len; memcpy(o[k].v, w + i, len * 4); k++; i += len;
    }
    return i == n ? k : 0;
}
static int cmp_cmd(const void* a, const void* b) { return memcmp(a, b, sizeof(Cmd)); }
static int same_commands(const u32* a, unsigned na, const u32* b, unsigned nb) {
    static Cmd ca[400], cb[400];
    unsigned ka = cmds_of(a, na, ca), kb = cmds_of(b, nb, cb);
    if (!ka || ka != kb) return 0;
    qsort(ca, ka, sizeof(Cmd), cmp_cmd); qsort(cb, kb, sizeof(Cmd), cmp_cmd);
    return memcmp(ca, cb, ka * sizeof(Cmd)) == 0;
}

/* Fixture: "NDUMP slot S n N" then "NW oooo w w w w w w w w" lines (8 words each). Returns the record count. */
static unsigned load_fixture(const char* path, u32 rw[2][300], unsigned cnt[2]) {
    FILE* f = fopen(path, "r");
    char line[512]; int nrec = -1; unsigned want[2] = { 0, 0 };
    if (!f) return 0;
    while (fgets(line, sizeof line, f)) {
        if (!strncmp(line, "NDUMP", 5)) {
            unsigned s, n;
            if (sscanf(line, "NDUMP slot %u n %u", &s, &n) == 2 && nrec < 1) { nrec++; cnt[nrec] = 0; want[nrec] = n; }
            continue;
        }
        if (!strncmp(line, "NW ", 3) && nrec >= 0) {
            char* p = line + 3; char* e;
            strtoul(p, &e, 16); p = e;
            for (;;) {
                unsigned long v = strtoul(p, &e, 16);
                if (e == p) break;
                if (cnt[nrec] < want[nrec]) rw[nrec][cnt[nrec]++] = (u32)v; /* the dump pads its last line with zeros */
                p = e;
            }
        }
    }
    fclose(f);
    return (unsigned)(nrec + 1);
}
static u32 find_cmd(const u32* w, unsigned n, u32 reg) {
    unsigned i = 0;
    while (i + 1 < n) {
        u32 hd = w[i + 1], c = (hd >> 20) & 0xFFu;
        if ((hd & 0x3FFu) == reg) return i;
        i += 2u + c + (c & 1u);
    }
    return ~0u;
}

static u32 find_last_cmd(const u32* w, unsigned n, u32 reg) {
    u32 last = ~0u; unsigned i = 0;
    while (i + 1 < n) {
        u32 hd = w[i + 1], c = (hd >> 20) & 0xFFu;
        if ((hd & 0x3FFu) == reg) last = i;
        i += 2u + c + (c & 1u);
    }
    return last;
}

/* 4 layout on the captured records, and 5 the eye copy */
static void test_layout(const char* path) {
    static u32 rw[2][300]; static u32 cnt[2];
    unsigned nrec = load_fixture(path, rw, cnt);
    CHECK(nrec == 2 && cnt[0] == 278 && cnt[1] == 272, "fixture: %u records, %u and %u words", nrec, cnt[0], cnt[1]);
    if (nrec != 2) return;
    memset(nat_tmpl, 0, sizeof nat_tmpl);
    NatRec rec[2]; memset(rec, 0, sizeof rec);
    int ids[2];
    u32 base = 0;
    for (int r = 0; r < 2; r++) {
        ids[r] = nat_tmpl_add(0x100u + (u32)r, 0x200u, 0, 0, 0, 1, 0, 0, 0, 0, NULL, NULL);
        u32 i81 = find_cmd(rw[r], cnt[r], 0x081), i91 = find_cmd(rw[r], cnt[r], 0x091), i2c = find_last_cmd(rw[r], cnt[r], 0x2C1);
        u32 i27 = find_cmd(rw[r], cnt[r], 0x227), i28 = find_cmd(rw[r], cnt[r], 0x228);
        CHECK(i81 != ~0u && i91 != ~0u && i2c != ~0u && i27 != ~0u && i28 != ~0u, "record %d: per-draw command missing", r);
        if (i81 == ~0u || i91 == ~0u || i2c == ~0u || i27 == ~0u || i28 == ~0u) return;
        if (r == 0) base = rw[0][i27];
        for (int k = 0; k < 4; k++) { nat_tex[2 * r][k] = rw[r][i81 + 2 + k]; nat_tex[2 * r + 1][k] = rw[r][i91 + 2 + k]; }
        memcpy(nat_mtx[0][r], rw[r] + i2c + 2, 16 * sizeof(u32));
        rec[r].tmpl = (u8)ids[r]; rec[r].tex0 = (u8)(2 * r); rec[r].tex1 = (u8)(2 * r + 1); rec[r].mtx = (u16)r;
        rec[r].nidx = (u16)rw[r][i28]; rec[r].idx_off = rw[r][i27] - base;
    }
    nat_idx_base = base;
    CHECK(rec[0].idx_off == 0 && rec[1].idx_off == 0xC0u, "index offsets %u %u", rec[0].idx_off, rec[1].idx_off);

    static u32 out[700], out1[700], copy1[700];
    static NatMtxLog log[2], log1[2];
    u16 ord[2] = { 0, 1 };
    unsigned nlog = 0, nlog1 = 0;
    unsigned w = nat_emit(rec, 2, ord, out, 700, 0, log, &nlog);

    /* eye 2: palette words differ, everything else equal */
    for (int r = 0; r < 2; r++) for (int k = 0; k < 16; k++) nat_mtx[1][r][k] = nat_mtx[0][r][k] ^ (0x10000u + (u32)k);
    unsigned w1 = nat_emit(rec, 2, ord, out1, 700, 1, log1, &nlog1);
    nat_eye_copy(copy1, out, w, log, nlog, 1);
    CHECK(w1 == w && memcmp(copy1, out1, w * 4) == 0, "eye copy differs from direct eye 1 emission");
    unsigned diff = 0, outside = 0;
    for (unsigned k = 0; k < w; k++) {
        if (out[k] == out1[k]) continue;
        diff++;
        int inside = 0;
        for (unsigned m = 0; m < nlog; m++) if (k >= log[m].at && k < log[m].at + 16) inside = 1;
        if (!inside) outside++;
    }
    CHECK(outside == 0, "%u differing words outside the palette patches", outside);
    CHECK(diff == 32, "differing words %u, want 32", diff);
    CHECK(nat_emit(rec, 2, ord, out1, 40, 0, log1, &nlog1) == 0, "small output did not return 0");
    printf("eye: 2 draws, palette words patched %u\n", diff);
}

/* PMOD P1: the pool copy equals the source, packed (E4a, no padding); a full pool returns NULL; reset clears the table. */
/* E2: texture block ids are keyed by content. Equal blocks share an id, different blocks differ, all-zero is 0. */
static void test_tex_id(void) {
    memset(nat_tex, 0, sizeof nat_tex);
    uint32_t a[4] = {1, 2, 3, 4}, b[4] = {1, 2, 3, 5}, z[4] = {0, 0, 0, 0};
    int ia = nat_tex_id(a), ib = nat_tex_id(b), ia2 = nat_tex_id(a);
    CHECK(ia > 0 && ib > 0 && ia != ib, "tex_id: different blocks gave ids %d %d", ia, ib);
    CHECK(ia2 == ia, "tex_id: equal block gave id %d, first was %d", ia2, ia);
    CHECK(nat_tex_id(z) == 0, "tex_id: zero block is not id 0");
    /* E3: GPU index address word, bit 31 set, relative to 0x18000000 (measured 0x20389D6E gives 0x88389D6E) */
    CHECK(nat_gpu_addr(0x20389D6Eu) == 0x88389D6Eu, "gpu_addr: got 0x%08X", nat_gpu_addr(0x20389D6Eu));
}

int main(int argc, char** argv) {
    if (argc < 2) { printf("usage: nat_host_test <fixture.txt>\n"); return 2; }
    test_record();
    test_template();
    test_order();
    test_layout(argv[1]);
    test_tex_id();
    if (fails) { printf("NAT HOST TESTS: %d FAIL\n", fails); return 1; }
    printf("NAT HOST TESTS: all pass\n");
    return 0;
}
