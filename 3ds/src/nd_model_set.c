/* nd_model_set.c: the native draw of a model record (plan S3N-3c, MOVE-5 split from nd_model.c). Target only: it calls the
 * walker and the GPU (pm_gbi_gv_at, pm_gpu_*, pm_eye_e, pm_dlc_native_ranges). The state it fills (nat, nat_n, nat_rng, nat_nr, nat_geom)
 * stays in nd_model.c, the cache store reads it there. */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "ultra64.h"
#include "gbi_custom.h"
#include "nd_int.h"
#include "pm_gpu.h"
#include "nd.h"
#include "nd_ops.h"

extern NdNat nat[ND_NAT_MAX]; extern int nat_n; extern uint64_t nat_vm; extern int nat_vm_ok; extern int nat_nr; extern uint32_t nat_geom;

extern float pm_eye_e, pm_eye_conv;
void pm_gbi_gv_at(unsigned tss, uint32_t geo, int tw, int th, NdGvP* o);
void pm_gpu_pcomp_native(float o[16], const float m[4][4]);
void pm_gpu_native_submit_draw(NdDraw* d, const uint16_t* sd, unsigned nidx, int tex);
void pm_gpu_flush(void);

int nd_model_draw(const uint32_t* rec, const NdMCtx* c0, const void* list, int emit) {
    static NdRng rng[512];
    float comb[4][4], pal0[16];
    nat_n = 0; nd_nat_why = 0;
    nd_tmpl_pin(); /* FB-2: the template ids of this record stay valid until its draws are submitted */
    if (!c0->mode || !c0->mode->ok) return (nd_nat_why = 1, -1);
    nd_mtx_combine(comb, pm_eye_e, pm_eye_conv);
    pm_gpu_pcomp_native(pal0, comb);
    const uint32_t entry = (c0->geom_base & c0->mode->gand) | c0->mode->gor;
    int nr = pm_dlc_native_ranges(list, entry, rng, 512u);
    if (nr < 0) return (nd_nat_why = nr == -2 ? 11 : 12, -1);
    nat_nr = nr <= ND_RAW_MAX ? nr : -1; nat_geom = entry;
    nat_vm = pm_dlc_vm; nat_vm_ok = pm_dlc_vm_ok;
    for (int r = 0; r < nr; r++) {
        NdMCtx c = *c0;
        NdGvP gv;
        pm_gbi_gv_at(c0->tss, rng[r].geom, c0->tex_w, c0->tex_h, &gv);
        c.gv = &gv; c.rgb_scale = gv.boost ? 2 : 1; /* the walker sets rgb_scale from the boost of the batch (dlc_seg) */
        c.geom_at = rng[r].geom; c.geom_at_set = 1;
        NdDraw dd; NdMState ss;
        int rc = nd_model_state(rec, &c, &dd, &ss);
        if (rc != 1) { nat_n = 0; nd_nat_why = ss.why; return rc < 0 ? -2 : -1; }
        const int d2 = ss.k.pass == 2; /* S5-1: decal2, no merge (each range has its own pass A) */
        if (!d2 && nat_n && nat[nat_n - 1].d.tmpl == dd.tmpl && nat[nat_n - 1].sd + nat[nat_n - 1].nidx == rng[r].sd) { nat[nat_n - 1].nidx += rng[r].nidx; continue; }
        if (nat_n + (d2 ? 2 : 1) > ND_NAT_MAX) { nat_n = 0; return (nd_nat_why = 11, -1); }
        if (d2) { /* pass A: the stencil mark, no colour, no depth write */
            NdTmplKey ka = ss.k; ka.pass = 1;
            int ida = nd_tmpl_build(&ka);
            if (ida < 0) { nat_n = 0; return (nd_nat_why = 7, -1); }
            NdNat* a = &nat[nat_n++];
            memcpy(a->pal, pal0, sizeof pal0);
            a->d = dd; a->d.tmpl = ida; a->d.pal = (const float(*)[16])a->pal; a->d.npal = 1;
            a->s = ss; a->sd = rng[r].sd; a->nidx = rng[r].nidx; a->tk = ka;
            if (a->d.use5) a->d.p5 = &a->s.p5; /* PERF2: the pm5 block of this copy */
        }
        NdNat* n = &nat[nat_n++];
        memcpy(n->pal, pal0, sizeof pal0);
        dd.pal = (const float(*)[16])n->pal; dd.npal = 1;
        n->d = dd; n->s = ss; n->sd = rng[r].sd; n->nidx = rng[r].nidx; n->zp = 0; n->zn = 0.f; n->tk = ss.k;
        if (n->d.use5) n->d.p5 = &n->s.p5; /* PERF2: the pm5 block of this copy */
        if (ss.k.zmark) { /* S5-2 (a): zmark pass 2 after the pass 1 draw: the same triangles, constant depth zn * w (pal row 2 := zn * row 3 = z clip of the walker's b2.z), stencil EQUAL cleared */
            if (nat_n >= ND_NAT_MAX) { nat_n = 0; return (nd_nat_why = 11, -1); }
            NdTmplKey kb = ss.k; kb.pass = 3; kb.zmark = 0;
            int idb = nd_tmpl_build(&kb);
            if (idb < 0) { nat_n = 0; return (nd_nat_why = 7, -1); }
            NdNat* b = &nat[nat_n++];
            memcpy(b->pal, pal0, sizeof pal0);
            for (int i = 0; i < 4; i++) b->pal[8 + i] = c.zn * pal0[12 + i];
            b->d = dd; b->d.tmpl = idb; b->d.pal = (const float(*)[16])b->pal; b->d.npal = 1;
            b->s = ss; b->s.k = kb; b->sd = rng[r].sd; b->nidx = rng[r].nidx; b->zp = 1; b->zn = c.zn; b->tk = kb;
            if (b->d.use5) b->d.p5 = &b->s.p5; /* PERF2: the pm5 block of this copy */
        }
    }
    if (emit) {
        pm_gpu_flush(); /* the walker batch open before this record goes out first */
        extern unsigned mfr_idx; /* TIK19 debug: the indices of the native model draws */
        for (int j = 0; j < nat_n; j++) { pm_gpu_native_submit_draw(&nat[j].d, nat[j].sd, nat[j].nidx, nat[j].d.u[0].dim != 0); mfr_idx += nat[j].nidx; }
    }
    return nat_n;
}
