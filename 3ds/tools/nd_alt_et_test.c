/* nd_alt_et_test.c: host test of the cut table of a flat copy (NAT5A, nd_alt_et_begin, nd_alt_eput, nd_alt_et_end). Prints "nat5a cases 6 bad <b>". */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ultra64.h"
#include "variables.h"
#include "pm_port.h"
#include "gbi_custom.h"
#include "nd.h"
#include "nd_api.h"
#include "nd_gtab.h"
#include "nd_cmd.h"
#include "nd_gbi.h"
#include "nd_st.h"
#include "nd_ds.h" /* NAT4A: the test state of run_one */
_Static_assert(ND_SOM_H == G_SETOTHERMODE_H, "ND_SOM_H");

/* the redirect target of the macro form: gMainGfxPos is the top-level packet pointer (nd_gbi.h D9) */
Gfx* gMainGfxPos;
s32 gCurrentDisplayContextIndex;
const NdNDev* const nd_rm_dev[0x3d]; /* nd_rmtab.c: the render mode lists, all NULL here */
int is_host(u32 p) { (void)p; return 0; }
char __end__[1];
u8 gMapShapeData[1];
void pm_log(const char* fmt, ...) { (void)fmt; }
void pm_log_e(const char* fmt, ...) { (void)fmt; }
void pm_log_evt(const char* fmt, ...) { (void)fmt; }
void pm_fatal_stop(const char* msg) { (void)msg; }
int pm_debug_has(const char* name) { (void)name; return 0; } /* the switches are off in the host test (nd_model.c PM5) */
unsigned pm_dbg_gen;
int pm_interp_repl;
const float (*pm_interp_find(const void* m))[4] { (void)m; return 0; }
uint32_t f32tof24(float f) { (void)f; return 0; } /* nd_tmpl.c: libctru, not used here */
void nd_walk_rec_model(const Gfx* q) { (void)q; }
void nd_walk_spr(const Gfx* q) { (void)q; }
void nd_walk_ent(const Gfx* q, unsigned words) { (void)q; (void)words; }
void nd_texrect_run(const uint32_t* p) { (void)p; }
void nd_parts_run(const uint32_t* rec) { (void)rec; }
void nd_parts_run_v(const uint32_t* rec) { (void)rec; }
const NdGTab* nd_gtab_find(const void* list) { (void)list; return 0; }
uint32_t nd_mkey_a(const uint32_t* rec) { (void)rec; return 0; }

/* the walker side: nd_stream_run hands each state run to nd_walk_state; the double logs each unit (word pair) */
static uint32_t ulog[512]; static unsigned nulog;
void nd_nr_run(const uint32_t* q) { const unsigned n = (q[-2] >> 16) & 0xffu;
    for (unsigned i = 0; i < n && nulog < 256u; i++) { ulog[2u * nulog] = q[2u * i]; ulog[2u * nulog + 1u] = q[2u * i + 1u]; nulog++; }
}
#include "nd_sv.h" /* NSET2 */
void nd_sci_run(const uint32_t* r); void nd_omh_run(const uint32_t* r); void nd_alt_put(uint32_t w0, uint32_t w1);
static NdNapiRun fam(uint32_t w0) { const unsigned id = w0 >> 24; return nd_op_geo(id) ? nd_geo_rec : nd_op_ctl(id) ? nd_ctl_rec : ((id >= ND_FS_OMH && id <= ND_FS_COMB) || (id >= ND_FS_TIMG && id <= ND_FS_LOADTLUT)) ? nd_cpy_run : nd_nr_run; } /* NAT5G; NAT6B */
#define U(id, b) ND_FS_W0(id, 0u), (uint32_t)(b)
static unsigned bad;
static void chk(int ok, const char* why) { if (!ok) { bad++; printf("nat5a bad: %s\n", why); } }
/* units of the range ab in order (follows NDC_JUMP, the headers of the runs) */
static unsigned walk(const uint32_t* ab, uint32_t* out, unsigned max, const uint32_t* stop_hdr[], unsigned* nstop) {
    const uint32_t* p = (const uint32_t*)(uintptr_t)ab[0]; const uint32_t* const e = (const uint32_t*)(uintptr_t)ab[1]; unsigned n = 0; *nstop = 0;
    while (p != e) {
        if ((p[0] >> 24) == NDC_JUMP) { p = (const uint32_t*)(uintptr_t)p[1]; continue; }
        const unsigned k = (p[0] >> 16) & 0xffu;
        if (p[0] >> 24 != NDC_NAPI || p[1] != (uint32_t)(uintptr_t)nd_nr_run && p[1] != (uint32_t)(uintptr_t)nd_geo_rec && p[1] != (uint32_t)(uintptr_t)nd_ctl_rec && p[1] != (uint32_t)(uintptr_t)nd_cpy_run) { bad++; printf("nat5a bad: header\n"); return n; }
        stop_hdr[(*nstop)++] = p;
        for (unsigned i = 0; i < k && n < max; i++) { if (p[1] == (uint32_t)(uintptr_t)nd_cpy_run) { out[2u * n] = ND_FS_W0(p[2u + 2u * i] & 0xffu, 0u); out[2u * n + 1u] = 0u; n++; i++; continue; } /* NSET2: a slot and its values give the unit id */
            out[2u * n] = p[2u + 2u * i]; out[2u * n + 1u] = p[3u + 2u * i]; n++; }
        p += 2u + 2u * k;
    }
    return n;
}
int main(void) {
    gMainGfxPos = nd_rec_task(); (void)nd_rec_mark();
    { /* case 1 */
        static const uint32_t l[] = { U(ND_FS_OMH, 0), U(ND_FS_GEOM, 0), U(ND_FS_VTX, 0), U(ND_FS_VTX, 0), U(ND_FS_TRI1, 0), U(ND_FS_TRI2, 0), ND_NOP_A, 0u,
            U(ND_FS_TRI1, 0), U(ND_FS_COMB, 0), U(ND_FS_PRIM, 0), U(ND_FS_VTX, 0), U(ND_FS_TRI1, 0), U(ND_FS_END, 0) };
        static const int want[12] = { 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1 };
        uint32_t ab[2]; uint32_t* const br = nd_alt_open(); nd_alt_raw_on = 2; nd_alt_et_begin();
        nd_ds_list(&nd_ds, l, 0);
        nd_alt_raw_on = 0; nd_alt_close(br, ab); const uint32_t* const t = nd_alt_et_end(ab[1]);
        chk(t != NULL, "c1 null");
        if (t) {
            chk(t[0] == 12u, "c1 n");
            for (unsigned i = 0; i < 12u; i++) {
                const uint32_t* const h = (const uint32_t*)(uintptr_t)t[1u + i];
                chk((h != NULL) == (want[i] != 0), "c1 cut set");
                if (h && want[i]) { chk((h[0] >> 24) == NDC_NAPI && h[1] == (uint32_t)(uintptr_t)fam(l[2u * i]), "c1 hdr"); chk(h[1] == (uint32_t)(uintptr_t)nd_cpy_run ? (h[2] & 0xffu) == (l[2u * i] >> 24) : (h[2] == l[2u * i] && h[3] == l[2u * i + 1u]), "c1 unit"); /* NAT6B */ }
            }
            chk(t[13] == ab[1], "c1 end");
            uint32_t out[64]; const uint32_t* hs[32]; unsigned ns; const unsigned n = walk(ab, out, 32u, hs, &ns);
            chk(n == 12u, "c1 walk n");
            for (unsigned i = 0; i < n && i < 12u; i++) chk(out[2u * i] == l[2u * i] && out[2u * i + 1u] == l[2u * i + 1u], "c1 order");
        }
    }
    { /* case 2: a call is flat in mode 2 */
        static const uint32_t in[] = { U(ND_FS_VTX, 0), U(ND_FS_TRI1, 0), U(ND_FS_END, 0) };
        const uint32_t l[] = { U(ND_FS_GEOM, 0), ND_OP_CALL_W0, (uint32_t)(uintptr_t)in, U(ND_FS_OMH, 0), U(ND_FS_END, 0) };
        uint32_t ab[2]; uint32_t* const br = nd_alt_open(); nd_alt_raw_on = 2; nd_alt_et_begin();
        nd_ds_list(&nd_ds, l, 0);
        nd_alt_raw_on = 0; nd_alt_close(br, ab); const uint32_t* const t = nd_alt_et_end(ab[1]);
        chk(t != NULL && t[0] == 4u, "c2 n");
        uint32_t out[32]; const uint32_t* hs[16]; unsigned ns; const unsigned n = walk(ab, out, 16u, hs, &ns);
        chk(n == 4u && out[0] == l[0] && out[2] == in[0] && out[4] == in[2] && out[6] == l[4], "c2 flat");
    }
    { /* case 4 NAT5G: a geometry record, a control record with a texture rectangle and its 2 half units, a state record */
        uint32_t ab[2]; uint32_t* const br = nd_alt_open();
        nd_alt_put(ND_FS_W0(ND_FS_VTX, 0), 0u); nd_alt_put(ND_FS_W0(ND_FS_RECT, 0), ND_FS_RECT_TEX); nd_alt_put(ND_NOP_A, 1u); nd_alt_put(ND_NOP_A, 2u); nd_alt_put(ND_FS_W0(ND_FS_OMH, 0), 0u);
        nd_alt_close(br, ab);
        uint32_t out[32]; const uint32_t* hs[16]; unsigned ns; const unsigned n = walk(ab, out, 16u, hs, &ns);
        chk(n == 5u && ns == 3u, "c4 n ns");
        if (ns == 3u) { chk(hs[0][1] == (uint32_t)(uintptr_t)nd_geo_rec, "c4 geo"); chk(hs[1][1] == (uint32_t)(uintptr_t)nd_ctl_rec && ((hs[1][0] >> 16) & 0xffu) == 3u && (hs[1][0] & 0xffu) == 1u, "c4 ctl"); chk(hs[2][1] == (uint32_t)(uintptr_t)nd_cpy_run && ((hs[2][0] >> 16) & 0xffu) == 2u, "c4 st"); /* NAT6B */ }
    }
    { /* case 5 NAT6B: the slot of a copy state unit, patched on the apply */
        nd_ds.sc_lost = 0; nd_ds.sc_dirty = 0;
        static const uint32_t l[] = { U(ND_FS_ENV, 0x12345678), U(ND_FS_ENV, 0x12345678), U(ND_FS_END, 0) };
        uint32_t ab[2]; uint32_t* const br = nd_alt_open(); nd_alt_raw_on = 1; nd_ds_list(&nd_ds, l, 0); nd_alt_raw_on = 0; nd_alt_close(br, ab);
        uint32_t out[32]; const uint32_t* hs[16]; unsigned ns; const unsigned n = walk(ab, out, 16u, hs, &ns);
        chk(n == 2u && ns == 1u, "c5 n ns");
        if (ns == 1u) { chk(hs[0][1] == (uint32_t)(uintptr_t)nd_cpy_run && ((hs[0][0] >> 16) & 0xffu) == 4u, "c5 hdr"); chk(hs[0][2] == (ND_C6_MARK | ND_FS_ENV | ND_SV_SYNC) && hs[0][3] == (uint32_t)(uintptr_t)nd_sv_env && hs[0][4] == 0u && hs[0][5] == 0x12345678u && hs[0][6] == (ND_NOP_A | ND_FS_ENV | ND_SV_SYNC) && hs[0][9] == 0x12345678u, "c5 slots"); }
    }
    { /* case 6 NAT6E1: a call unit of a raw fallback range is a nested raw copy (CALL | 2), the top range keeps 3 units */
        static const uint32_t in6[] = { U(ND_FS_VTX, 0), U(ND_FS_END, 0) };
        const uint32_t au6[] = { U(ND_FS_OMH, 0), ND_OP_CALL_W0, (uint32_t)(uintptr_t)in6, U(ND_FS_OMH, 0) }; const uint32_t r6[2] = { 0u, 0u }; /* NAT6E1 */
        nd_rec_alt_raw(nd_sci_run, r6, 1u, 0, au6, 3u);
        const uint32_t* const ab6 = nd_rec_last_pay() + 2u; /* NAT6E1 */
        uint32_t out[32]; const uint32_t* hs[16]; unsigned ns; const unsigned n = walk(ab6, out, 16u, hs, &ns);
        chk(n == 3u && out[0] == au6[0] && out[2] == (ND_OP_CALL_W0 | 2u) && out[4] == au6[4], "c6 call");
    }
    chk(nd_alt_et_end(0u) == NULL, "c3 end");
    printf("nat5a cases 6 bad %u\n", bad);
    return bad ? 1 : 0;
}
