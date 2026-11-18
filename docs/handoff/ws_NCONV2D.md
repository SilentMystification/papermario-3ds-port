# ws_NCONV2D (Haiku): a list with a snap table runs without its combine units on core 0 (OML, PRIM, ENV, FOG, BLEND, RARE, COMB). Core 1 gives them: range snaps, range ep flags, the end state. Counter: none of C1-C12 (stage S5). Gate: 2 drec byte-equal.
Designer: oracle NCONV2RA. WAIT: branch from main after the NCONV2B and NCONV2C merges (this row edits their code). PRE (from the NCONV2C hand-back, both scenes): lscut epm 0, epx 0, rect 0; lsnap over 0. A PRE value off: do not start, send the NCONV2C lines to the oracle.
Worktree build3ds/wt/NCONV2D, branch NCONV2D. PM_ROW=NCONV2D PM_LANE=NCONV2D. Edit only (Edit tool, keep CRLF): 3ds/include/nd_snap.h, 3ds/src/nd_snap_in.c (nd_snap_rnd), 3ds/src/nd_snap.c, 3ds/src/nd_walk.c (new ls_omh, ls_run; nd_ntab_run).
## Facts (code read, main 73359b439a; find the quoted text, line numbers move)
- FACT the skipped units have these core 0 GPU calls: PRIM pm_gpu_set_prim (nd_set.c:76), ENV pm_gpu_set_env (nd_set.c:102), RARE conv pm_comb_set_k4 (nd_set.c:149), COMB pm_gpu_flush on a new combine (nd_comb.c:497), each then sync_combine. OML, FOG, BLEND only write fields (nd_set.c:93, :43, :71).
- FACT a change of any of their fields changes the snap inputs (snap_build memo, nd_snap.c:51), so the next list triangle arms a new range (NCONV1D2) and sc_adopt (NCONV2B) runs. prim_lod has 8 bits in the snap (in.prim_lo8): this row adds the 16-bit word in the free pad2 (no reader: git grep).
- FACT OMH and TEXTURE skip tests read sc_ok and sc_sig (nd_set.c:125, :138): ls_run writes them without the skip test and the sync. GEOM reads geom only: it keeps nd_op_same.
- FACT nd_ntab_run (nd_walk.c:634) runs every v2 list unit copy: gt_body (:637), walk_call inside a list (:277), the moved parts list (:476, :524).
## Edits
1. nd_snap.h: `uint32_t pad2; /* NDR0E1: size to a multiple of 8 */` becomes `uint32_t plod; /* NCONV2D: prim_lod, the 16-bit prim LOD word (was pad2) */`. After the NCONV2C macros add `#define ND_LS_DSB ((((unsigned)sizeof(NdDs)) + 7u) & ~7u) /* NCONV2D: the core 1 end state after the snaps of a list table */` and `void nd_lsr_geo(void); /* NCONV2D */`.
2. nd_snap_in.c nd_snap_rnd: append ` r->plod = s->w.prim_lod; /* NCONV2D */`. nd_snap.c snap_build memo: `r.mixk == sn_last.mixk` becomes `r.plod == sn_last.plod && r.mixk == sn_last.mixk`.
3. nd_snap.c nd_lsc_end_at: `nd_rec_data_st(h + ls_n * (unsigned)sizeof ls_s[0])` becomes `nd_rec_data_st(h + ls_n * (unsigned)sizeof ls_s[0] + ND_LS_DSB)`. Before `*dst = (uint32_t)(uintptr_t)b;` add `memcpy((char*)b + h + ls_n * sizeof ls_s[0], &nd_ds, sizeof nd_ds); /* NCONV2D: the end state */ `.
4. nd_snap.c, above `static void sc_adopt(void) {` add:
```c
static unsigned char lr_geo; /* NCONV2D: the open list runs ls_run (nd_walk.c) */
void nd_lsr_geo(void) { lr_geo = 1; }
void pm_comb_set_k4(unsigned k4); void pm_gpu_set_prim(unsigned rgba, unsigned lod); void pm_gpu_set_env(unsigned rgba); /* pm_gpu.h:79, :92, :96 */
/* NCONV2D: the GPU calls of the units that ls_run skips, when the walker fields differ from the new ones (nd_set.c:76, :102, :149; nd_comb.c:497). */
static void ls_gpu(uint32_t w0, uint32_t w1, int cset, uint32_t prim, uint32_t plod, uint32_t env, uint32_t k4) {
    if (nd_ws.comb_w0 != w0 || nd_ws.comb_w1 != w1 || (cset && !comb_set)) pm_gpu_flush();
    if (nd_ws.prim != prim || nd_ws.prim_lod != plod) pm_gpu_set_prim(prim, plod);
    if (nd_ws.envc != env) pm_gpu_set_env(env);
    if (conv_k4 != k4) pm_comb_set_k4(k4);
    nd_ws.prim_lod = plod;
}
/* NCONV2D: the end of a list that ran ls_run: the fields of the skipped units from the core 1 end state (dschk: core 1 equals core 0 at each
 * command). The tst1 and use_t1 block is dir_adopt (nd_dschk.c:79-87, copy it as it is): a folded core 0 keeps its folded combine. */
static void ds_take(const NdDs* c) {
    extern unsigned box_save[4]; /* nd_comb.c */
    ls_gpu(c->w.comb_w0, c->w.comb_w1, c->comb_set, c->w.prim, c->w.prim_lod, c->w.envc, c->conv_k4);
    nd_ws.omode_l = c->w.omode_l; nd_ws.om_kl = c->w.om_kl; nd_ws.prim = c->w.prim; nd_ws.envc = c->w.envc;
    nd_ws.comb_w0 = c->w.comb_w0; nd_ws.comb_w1 = c->w.comb_w1;
    /* here the block of nd_dschk.c:79-87 */
    fogc = c->fogc; blendc = c->blendc; keyc = c->keyc; keys = c->keys; conv_k4 = c->conv_k4;
    comb_set = c->comb_set; box_prod = c->box_prod; comb_alpha_shade = c->comb_alpha_shade; dual_on = c->dual_on;
    sc_ok = 0; /* the next combine unit of the stream syncs again; it falls in the cut of the next command */
}
```
5. nd_snap.c sc_adopt: new first statement after `const NdCombIn* i = &sc_cur.in;`: `    if (lr_geo) ls_gpu(i->w0, i->w1, i->comb_set, i->prim, sc_cur.plod, i->env, i->conv_k4); /* NCONV2D */`.
6. nd_snap.c: nd_lsr_begin, after `lr_nx = 0;` add ` lr_geo = 0;`. nd_lsr_tri, before `    if (lr_b[2u + r] & ND_LS_DUP) return;` add `    if (lr_geo && (lr_b[2u + r] & ND_LS_EP)) gv_epoch++; /* NCONV2D: the cut of the units ls_run skipped */`. nd_lsr_end. Before: `void nd_lsr_end(void) { if (nd_ls_on && nd_ls_ord != lr_b[1]) lr_tmis++; nd_ls_on = 0; }` After: `void nd_lsr_end(void) { if (nd_ls_on && nd_ls_ord != lr_b[1]) lr_tmis++; if (nd_ls_on && lr_geo) ds_take((const NdDs*)(const void*)((const char*)lr_b + ND_LS_HDR(lr_b[0]) + lr_b[0] * sizeof sc_cur)); lr_geo = 0; nd_ls_on = 0; } /* NCONV2D */`.
7. nd_walk.c, after `void nd_nr_run(const uint32_t* rec) { ... }` add ls_omh and ls_run:
```c
void pm_tex_set_filter(int linear); /* pm_tex.c */
static void ls_omh(const uint32_t* q) { /* NCONV2D: the H word as set_om writes it (nd_set.c:84-91), without the combine sync */
    const uint32_t m = nd_op_om_mask(q[0] & 0xffffffu), old_tf = nd_ws.omode_h & (3u << 12);
    nd_ws.omode_h = (nd_ws.omode_h & ~m) | (q[1] & m); nd_ws.om_kh |= m;
    if ((nd_ws.omode_h & (3u << 12)) != old_tf) { pm_gpu_flush(); pm_tex_set_filter((nd_ws.omode_h & (3u << 12)) != 0); }
}
/* NCONV2D: the unit copy of a list with a snap table: nd_walk_state without the combine units (core 1: snaps, ep flags, ds_take). */
static void ls_run(const uint32_t* q, unsigned n) {
    const unsigned dbg = pm_walk_dbg;
    for (const uint32_t* e = q + 2u * n; q < e; q += 2) {
        const unsigned id = q[0] >> 24;
        if (q[0] == ND_NOP_A) continue;
        if (id == ND_FS_OML || (id >= ND_FS_PRIM && id <= ND_FS_BLEND) || id == ND_FS_RARE || id == ND_FS_COMB) continue;
        if (id == ND_FS_OMH) { ls_omh(q); continue; }
        if (id == ND_FS_TEXTURE) { nd_set_texture(q[0] & 0xffffffu, q[1]); continue; }
        if (id == ND_FS_GEOM) { if (!nd_op_same(q)) nd_op_apply(q); continue; }
        if (__builtin_expect(!nd_op_typed(id), 0)) { op_unknown(dbg, q); continue; }
        if (nd_op_geo(id)) op_geo(dbg, q, q);
        else if (nd_op_ctl(id)) { const unsigned r = op_ctl(dbg, q, q, e); if (r == ~0u) q = e - 2; else q += 2u * r; }
        else if (__builtin_expect(id == ND_FS_DROP, 0)) { q += 2; if (q < e) t4_check(dbg, q); }
        else if (id == ND_FS_END) break;
        else op_typed(dbg, q, q);
    }
}
```
8. nd_walk.c. Before: `void nd_ntab_run(const NdNDev* e) { nd_nr_exec(e->u, e->nu); }` After: `void nd_ntab_run(const NdNDev* e) { if (nd_ls_on) { nd_lsr_geo(); ls_run(e->u, e->nu); } else nd_nr_exec(e->u, e->nu); } /* NCONV2D: a list with a snap table runs without its combine units */`.
## Steps (one command per step, output to build3ds/out/, read the last 20 lines; Bash `timeout: 600000`; 1 lane build; at most 4 Azahar starts of 8)
1. Edits 1-8. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nconv2d_host.txt 2>&1`: PASS. Commit `NCONV2D 1: lists with a snap table run without their combine units`.
2. For fxops_drec, then entcape_kmr03_drec: `python 3ds/tools/bench.py base <bench> --commit $(git merge-base HEAD main) > build3ds/out/nconv2d_base_<bench>.txt 2>&1`, then `sh 3ds/tools/rowloop.sh --row NCONV2D --bench <bench> --no-host --why "NCONV2D ls_run" > build3ds/out/nconv2d_loop_<bench>.txt 2>&1`.
3. `PM_ROW=NCONV2D PM_LANE=NCONV2D python 3ds/tools/bench.py run fxops_snapchk --row NCONV2D --why "NCONV2D lsnap" > build3ds/out/nconv2d_run.txt 2>&1`, then `grep -a "lsnap f\|lscut\|ndc: stream" build3ds/azt/logs/fxops_snapchk.txt > build3ds/out/nconv2d_lines.txt`.
Gate (exact): host PASS. Both rowloops exit 0 (byte-equal). Last lsnap line: tmis 0. Report the lscut line (epx rises now: data, not judged) and any `ndc: stream` line.
Stop: a DIFF, tmis above 0, a stream overflow line, a crash, a fatal line, a tool fault. Commit the outputs (`git add -f`), send the paths to the oracle of the launch prompt with SendMessage, wait. Fix rights: the files of this page, compile errors only. Pitfalls: one host test at a time. One bench per rowloop call. PM_CACHE=off only as a prefix of one command.
Hand-back: last commit `NCONV2D 2: gate. fxops and entcape drec equal; lists skip the combine units` deletes this page. One SubagentHandback: commit hash, the lsnap and lscut lines.
