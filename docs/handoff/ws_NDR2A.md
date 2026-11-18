# ws_NDR2A (Haiku): a fill record takes a core 1 snap of its pso state; snapchk compares it with nd_pso_decide (stage S4, measure; counter: none of C1-C12)
Designer: oracle FIN. WAIT: branch from main after the NDR1C merge (snap_take(int zc) of NDR1C, sc_count of NDR0G2; NDR3 is merged: nd_rec_napi_s, the nd_snap.h include of nd_direct.c). Worktree build3ds/wt/NDR2A, branch NDR2A. PM_ROW=NDR2A PM_LANE=NDR2A. Edit only: 3ds/src/nd_snap.c, 3ds/include/nd_snap.h, 3ds/src/nd_direct.c, 3ds/include/nd_api.h, src/screen_overlays.c, src/battle/190B20.c, 3ds/tools/nd_direct_test.c (host stubs).
## Facts (code read, main 94901a28aa; find each line by its text)
- nd_direct_bind (nd_direct.c:96-141) writes the pso words into the walker (omode_h by omh_m, omode_l but its low 3 bits, op_setcombine when the words differ or
  comb_set is 0, dual_on 0, prim, prim_lod 0, env), then decides with nd_pso_decide (:69, walker state). nd_ds.c ds_comb (:55) is the core 1 copy of op_setcombine.
- The 3 fill producers record `nd_rec_napi(nd_fill_run, &rec, 3u);` and `nd_fill_feed(...)`: screen_overlays.c:170-171, :180-181, battle/190B20.c:3017-3018. nd_fill_run (nd_direct.c:289-309) builds the pso from kFillKind[k] (:244-250) and f->rgba, env = nd_ws.envc.
## Edits
1. nd_snap.c: `snap_take(int zc)` becomes `snap_take(const NdDs* s, int zc)`; in its body `&nd_ds` becomes `s` and `nd_ds.` becomes `s->` (`sed -b -i` on its line range only: keep CRLF). nd_snap_mesh and nd_snap_rect pass `&nd_ds`. New:
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
   (nd_direct.c:131 does this to cur_plan), memcmp -> "dplan". Then `sc_count("dir");` (NDR0G2 helper: line `snapchk bad f<n> dir <names>`).
   `static unsigned sc_dir;` and append ` dir %u` with sc_dir to the 100-frame line of nd_snapchk_tick. nd_snap.h: declare nd_snap_pso and nd_snapchk_dir.
3. nd_direct.c: `static void fill_pso(unsigned k, NdPsoN64* d)` = the 5 `d.` lines of nd_fill_run (:298-302) with `d->`; nd_fill_run calls `fill_pso(k, &d);`. New core 1 function:
   New core 1 `void nd_dir_rec(NdNapiRun run, const void* rec, unsigned units, const NdPsoN64* pd, uint32_t prim) { nd_snap_pso(pd, prim, nd_ds.w.envc); nd_rec_napi_s(run, rec, units); } /* NDR2A: snap and record of a direct draw */`,
   `void nd_fill_rec(const NdFillRec* f) { NdPsoN64 d; fill_pso(f->kind & 3u, &d); nd_dir_rec(nd_fill_run, f, 3u, &d, f->rgba); nd_fill_feed((const uint32_t*)f); }` and core 0 `void nd_dir_drop(void) { nd_snap_arm = 0; nd_snap_live = 0; } /* NDR2A: a fallback walk never takes the pso snap (its tiles predate the record feed) */`; nd_fill_walk first line `nd_dir_drop();`. Core 1 getter `uint32_t nd_dir_prim1(void) { return nd_ds.w.prim; } /* NDR2A: nd_direct_prim at record time (game files do not include nd_ds.h) */`.
   nd_direct_bind, first line: `const unsigned char sn = nd_snap_arm; nd_snap_arm = 0; /* NDR2A: the snap of a converted direct record */`. After `if (!d) return 0;` add `if (sn) nd_snapchk_dir(&d->out); /* NDR2A */`.
4. nd_api.h after nd_fill_feed: declare nd_fill_rec, nd_dir_rec, nd_dir_prim1 (core 1) and nd_dir_drop (core 0), one comment each.
   nd_direct_test.c (it includes nd_direct.c and links no nd_snap.c, nd_rec.c, nd_ds.c; FACT nd_host_test.sh:238), after the nd_ds_feed_dir stub: `NdDs nd_ds;` and empty stubs
   `void nd_rec_napi_s(NdNapiRun run, const void* rec, unsigned units)`, `void nd_snap_pso(const NdPsoN64* p, uint32_t prim, uint32_t env)`, `void nd_snapchk_dir(const NdCombOut* w)` (each `(void)` its arguments), and `unsigned char nd_snap_live;`.
5. The 3 producer sites. Before: `nd_rec_napi(nd_fill_run, &rec, 3u);` + `nd_fill_feed((const uint32_t*)&rec);` After: `nd_fill_rec(&rec); /* NDR2A */` (190B20.c: `&q`).
## Steps (output to build3ds/out/, read the last 20 lines; 1 build, 5 Azahar starts)
1. Edits 1-5. `sh 3ds/tools/nd_host_test.sh > build3ds/out/ndr2a_host.txt 2>&1`: PASS. `grep -c "nd_rec_napi(nd_fill_run" src/screen_overlays.c src/battle/190B20.c`: 0 each. Commit `NDR2A 1: fill records take a pso snap, snapchk dir`.
2. One call each: `python 3ds/tools/bench.py base <b>` and `sh 3ds/tools/rowloop.sh --row NDR2A --bench <b> --no-host` for b = wscmb_pause_drec, axns_sam2_drec, curt_title_drec: byte-equal each.
3. One call each: `python 3ds/tools/bench.py run <s> --row NDR2A --why "NDR2A snapchk dir"` for s = nimgm_strip_snapchk, wscmb_pause_snapchk. `grep -a snapchk` of both logs > build3ds/out/ndr2a_snapchk.txt.
Gate (exact): host PASS, 3 benches byte-equal (FACT: the snap is a state op; core 0 still applies nd_pso_decide), snapchk bad 0, wst not above the NDR1C hand-back. Report the dir counts (0 is not a fault). Stop: a DIFF, a crash, a `dir` bad line, a tool fault: commit the outputs, send the paths to the oracle with SendMessage, wait. Fix rights: the listed files; compile errors.
Pitfalls: `bench.py run` does not build. Run `PM_LANE=NDR2A sh 3ds/build.sh` after each code change, before step 3 (FACT NDR0G2: STALE BUILD exit 5). One host test at a time in a worktree (rowloop takes --no-host). PM_CACHE=off only as a prefix of one command, never exported. One bench per rowloop call. Each scene in its own call, no timeout option.
Keep each file's line endings (CRLF). A snap record is about 640 bytes (GUESS), the stream has 16 chunks of 32 KB per context (FACT nd_rec.c:30-31): the memo must stop a snap when the inputs stay the same.
Hand-back: last commit `NDR2A 2: gate. 3 benches byte-equal; snapchk bad <2>, dir <2>` deletes this page. One SubagentHandback: commit hash, gate values, dir counts.
