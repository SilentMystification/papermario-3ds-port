# ws_NDR2A (Haiku): a fill record takes a core 1 snap of its pso state; snapchk compares it with nd_pso_decide (stage S4, measure; counter: none of C1-C12)
Designer: oracle FIN. WAIT: branch from main after the NDR1C and NDR3 merges (snap_take of NDR1C; nd_rec_napi_s and the nd_snap.h include of nd_direct.c of NDR3).
Worktree build3ds/wt/NDR2A, branch NDR2A. PM_ROW=NDR2A PM_LANE=NDR2A. Edit only: 3ds/src/nd_snap.c, 3ds/include/nd_snap.h, 3ds/src/nd_direct.c, 3ds/include/nd_api.h, src/screen_overlays.c, src/battle/190B20.c.
## Facts (code read, main 94901a28aa; find each line by its text)
- nd_direct_bind (nd_direct.c:96-141) writes the pso words into the walker (omode_h by omh_m, omode_l but its low 3 bits, op_setcombine when the words differ or
  comb_set is 0, dual_on 0, prim, prim_lod 0, env), then decides with nd_pso_decide (:69, walker state). nd_ds.c ds_comb (:55) is the core 1 copy of op_setcombine.
- The 3 fill producers record `nd_rec_napi(nd_fill_run, &rec, 3u);` and `nd_fill_feed(...)`: screen_overlays.c:170-171, :180-181, battle/190B20.c:3017-3018.
  nd_fill_run (nd_direct.c:289-309) builds the pso from kFillKind[k] (:244-250) and f->rgba, env = nd_ws.envc.
## Edits
1. nd_snap.c: `snap_take(int zc)` becomes `snap_take(const NdDs* s, int zc)`; in its body `&nd_ds` becomes `s` and `nd_ds.` becomes `s->` (sed on its line range only).
   nd_snap_mesh and nd_snap_rect pass `&nd_ds`. New:
   ```
   static NdDs sn_tmp; /* NDR2A, core 1: nd_ds with the pso words of a direct draw */
   void nd_snap_pso(const NdPsoN64* p, uint32_t prim, uint32_t env) {   /* NDR2A: the state words of nd_direct_bind, in its order */
       sn_tmp = nd_ds;
       sn_tmp.w.omode_h = (sn_tmp.w.omode_h & ~p->omh_m) | (p->omh & p->omh_m); sn_tmp.w.om_kh |= p->omh_m;
       sn_tmp.w.omode_l = (sn_tmp.w.omode_l & 7u) | (p->oml & ~7u); sn_tmp.w.om_kl |= ~7u;
       if (sn_tmp.w.comb_w0 != p->cw0 || sn_tmp.w.comb_w1 != p->cw1 || !sn_tmp.comb_set) nd_ds_apply(&sn_tmp, ND_FS_W0(ND_FS_COMB, p->cw0), p->cw1);
       sn_tmp.dual_on = 0; sn_tmp.w.prim = prim; sn_tmp.w.prim_lod = 0u; sn_tmp.w.envc = env;
       snap_take(&sn_tmp, 0);
   }
   ```
2. nd_snap.c core 0, new `void nd_snapchk_dir(const NdCombOut* w)`: return without PMDL("snapchk"). `sc_dir++; sc_nb = 0;` with `const NdCombOut* o = &sc_cur.o;`:
   `o->leg != w->leg || o->mode5 != w->mode5 || o->pat != w->pat || o->plan_on != w->plan_on` -> "dleg"; `o->use != w->use || o->cyc != w->cyc || o->dual != w->dual` -> "dls";
   `o->cc_on != w->cc_on || o->env_on != w->env_on` -> "dcc"; when both plan_on: copies a = o->plan, b = w->plan, `pm_comb_colours(&x, nd_ws.prim, nd_ws.envc, (u8)nd_ws.prim_lod, keyc, keys)` on each
   (nd_direct.c:131 does this to cur_plan), memcmp -> "dplan". Then the count and log block of nd_snapchk_rnd with the text `snapchk bad f%u dir%s`.
   `static unsigned sc_dir;` and append ` dir %u` with sc_dir to the 100-frame line of nd_snapchk_tick. nd_snap.h: declare nd_snap_pso and nd_snapchk_dir.
3. nd_direct.c: `static void fill_pso(unsigned k, NdPsoN64* d)` = the 5 `d.` lines of nd_fill_run (:298-302) with `d->`; nd_fill_run calls `fill_pso(k, &d);`. New core 1 function:
   `void nd_fill_rec(const NdFillRec* f) { NdPsoN64 d; fill_pso(f->kind & 3u, &d); nd_snap_pso(&d, f->rgba, nd_ds.w.envc); nd_rec_napi_s(nd_fill_run, f, 3u); nd_fill_feed((const uint32_t*)f); } /* NDR2A */`
   nd_direct_bind, first line: `const unsigned char sn = nd_snap_arm; nd_snap_arm = 0; /* NDR2A: the snap of a converted direct record */`. After `if (!d) return 0;` add `if (sn) nd_snapchk_dir(&d->out); /* NDR2A */`.
4. nd_api.h after nd_fill_feed: `void nd_fill_rec(const NdFillRec* f); /* nd_direct.c, core 1, NDR2A: snap, record (nd_rec_napi_s) and feed of one fill */`.
5. The 3 producer sites. Before: `nd_rec_napi(nd_fill_run, &rec, 3u);` + `nd_fill_feed((const uint32_t*)&rec);` After: `nd_fill_rec(&rec); /* NDR2A */` (190B20.c: `&q`).
## Steps and tests (output to build3ds/out/, read the last 20 lines; 1 build, 2 Azahar starts)
1. Edits 1-5. `sh 3ds/tools/nd_host_test.sh` PASS. `grep -c "nd_rec_napi(nd_fill_run" src/screen_overlays.c src/battle/190B20.c`: 0 each. Commit `NDR2A 1: fill records take a pso snap, snapchk dir`.
2. `python 3ds/tools/bench.py base wscmb_pause_drec,axns_sam2_drec,curt_title_drec` (merge-base, not charged). `sh 3ds/tools/rowloop.sh --row NDR2A --bench wscmb_pause_drec,axns_sam2_drec,curt_title_drec`: LOOP PASS, 3 byte-equal.
3. `python 3ds/tools/bench.py run nimgm_strip_snapchk,wscmb_pause_snapchk --row NDR2A --why "NDR2A snapchk dir"`; `grep -a snapchk` of both logs > build3ds/out/ndr2a_snapchk.txt.
   Pass: bad 0. Report the dir counts (GUESS: above 0 in wscmb_pause; 0 is not a fault: state it in the hand-back).
Expected: byte-equal (FACT: the snap is a state op; core 0 still applies nd_pso_decide). A build after a partial shape cache: `PM_CACHE=off sh 3ds/build.sh`.
Fix rights: the listed files; compile errors. A DIFF, a crash, a `dir` bad line: stop, commit the outputs, send the paths to the oracle with SendMessage. Wait.
Stop: last commit `NDR2A 2: gate. 3 benches byte-equal; snapchk bad <2>, dir <2>` deletes this page. One SubagentHandback.
