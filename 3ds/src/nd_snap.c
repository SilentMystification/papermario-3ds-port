/* nd_snap.c: the snapchk switch (NDR0A, design_nstate.md section 6, stage S4). Core 1 (nd_snap_mesh, from nd_mesh_tri1/2 in nd_geo.c) takes the
 * combine inputs of nd_ds and the table decision of nd_pso_find_i, and records them as an NDC_NAPI run. Core 0 (nd_snapchk_run) copies the
 * record and arms nd_snap_arm; the next triangle draw (nd_snapchk_tri, from draw_tri in nd_geom.c) compares it with the walker inputs.
 * Logs: `snapchk bad f<n> <names>` (the first 8 bad triangles), `snapchk f<n> recs <r> cmp <c> bad <b> fbad <s> miss <m>` every 100 frames. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "ultra64.h"
#include "nd.h"
#include "nd_int.h"
#include "nd_ds.h"
#include "nd_api.h"
#include "nd_snap.h"
#include "pm_port.h"

extern unsigned pm_frame_n; /* pm_main.c: the logic step counter */
void comb_in_walker(NdCombIn* in); /* nd_comb.c: the walker inputs of the combine (core 0) */

/* ---- core 1 ---- */
static int sn_leg, sn_mode5, sn_pat;
static NdSnapChk sn_last;
static unsigned sn_cmds = ~0u;

void nd_snap_mesh(void) {
    if (!PMDL("snapchk")) return;
    NdSnapChk r;
    NdCombOut o;
    memset(&r, 0, sizeof r);
    memset(&o, 0, sizeof o);
    nd_snap_in(&nd_ds, sn_leg, sn_pat, &r.in);
    if (sn_cmds == nd_rec_cmds() && !memcmp(&r.in, &sn_last.in, sizeof r.in)) return; /* no new command, same inputs */
    r.key = nd_pso_find_i(&r.in, 0u, &o);
    r.plan_on = -1;
    if (r.key >= 0) {
        nd_pso_fix(&o, &r.in, sn_mode5);
        sn_leg = o.leg; sn_mode5 = o.mode5; sn_pat = o.pat;
        r.plan_on = o.plan_on;
    }
    r.leg = sn_leg; r.mode5 = sn_mode5; r.pat = sn_pat;
    sn_last = r;
    nd_rec_napi(nd_snapchk_run, &r, sizeof r / 8u);
    sn_cmds = nd_rec_cmds();
}

/* ---- core 0 ---- */
unsigned char nd_snap_arm;
static NdSnapChk sc_cur;
static unsigned sc_recs, sc_cmp, sc_bad, sc_fbad, sc_miss, sc_nlog, sc_bucket;
static int sc_nb;
static const char* sc_nm[8];

static void sc_add(const char* s) {
    if (sc_nb < 8) sc_nm[sc_nb] = s;
    sc_nb++;
}

void nd_snapchk_run(const uint32_t* rec) {
    memcpy(&sc_cur, rec, sizeof sc_cur);
    nd_snap_arm = 1;
    sc_recs++;
}

#define SC_CMP(f, name) do { if (sc_cur.in.f != mine.f) sc_add(name); } while (0)

void nd_snapchk_tri(void) {
    NdCombIn mine;
    char names[192];
    nd_snap_arm = 0;
    comb_in_walker(&mine);
    sc_cmp++;
    sc_nb = 0;
    SC_CMP(w0, "w0"); SC_CMP(w1, "w1"); SC_CMP(omode_h, "omh"); SC_CMP(prim, "prim"); SC_CMP(env, "env");
    SC_CMP(prim_lo8, "plo8"); SC_CMP(key_c, "keyc"); SC_CMP(key_s, "keys"); SC_CMP(conv_k4, "cvk4");
    SC_CMP(tex_on, "tex_on"); SC_CMP(use_t1, "use_t1"); SC_CMP(t1_valid, "t1_valid"); SC_CMP(dual_on, "dual_on");
    SC_CMP(box_prod, "box_prod"); SC_CMP(box_folded, "box_folded"); SC_CMP(comb_set, "comb_set");
    SC_CMP(comb_alpha_shade, "comb_alpha_shade");
    if (memcmp(sc_cur.in.tst, mine.tst, sizeof mine.tst)) sc_add("tst");
    if (sc_cur.key >= 0) {
        if (sc_cur.leg != leg_mode) sc_add("leg");
        if (sc_cur.mode5 != comb_mode5) sc_add("mode5");
        if (sc_cur.pat != box_pat) sc_add("pat");
        if (sc_cur.plan_on != plan_active) sc_add("plan_on");
    } else {
        sc_miss++;
    }
    if (sc_nb) {
        sc_bad++;
        sc_fbad += (unsigned)sc_nb;
        if (sc_nlog < 8) {
            unsigned k = 0;
            names[0] = 0;
            for (int i = 0; i < sc_nb && i < 8; i++) k += (unsigned)snprintf(names + k, sizeof names - k, " %s", sc_nm[i]);
            sc_nlog++;
            pm_log_evt("snapchk bad f%u%s", pm_frame_n, names);
        }
    }
    if (pm_frame_n / 100 != sc_bucket) {
        sc_bucket = pm_frame_n / 100;
        pm_log_evt("snapchk f%u recs %u cmp %u bad %u fbad %u miss %u", pm_frame_n, sc_recs, sc_cmp, sc_bad, sc_fbad, sc_miss);
    }
}
