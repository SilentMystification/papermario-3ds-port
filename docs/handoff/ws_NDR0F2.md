# ws_NDR0F2 (Haiku): a mesh triangle takes its render state from the snap at its epoch (stage S4; counter: none of C1-C12, snapchk bad stays 0)
Designer: oracle FIN. WAIT: branch from main after the NDR0F1 merge (its gate shows no `rnd` name). Worktree build3ds/wt/NDR0F2, branch NDR0F2. PM_ROW=NDR0F2 PM_LANE=NDR0F2.
Edit only: 3ds/src/nd_set.c (sync_render), 3ds/include/nd_int.h (after nd_mix1 :168), 3ds/src/nd_snap.c (below sc_compare only), 3ds/include/nd_snap.h (prototypes only), 3ds/src/nd_geom.c.
Runs in parallel with NDR0G1 (it edits nd_snap_mesh, sc_compare and the NdSnapChk struct).
## Facts (code read, NDR0F1 138c5a5a2a; find each line by its text)
- sync_render (nd_set.c:211-254): skip test on sg, the mix, nd_render_state, the GPU calls. The snap holds the same inputs (snapchk: omh, oml, geom, role equal) and rs, mixk, mixc.
- NDR0F1 sets nd_snap_rpend in sc_compare (nd_snap.c:128); a texture rectangle runs sc_compare too (nd_rect.c:241), so the flag stays set for a later unarmed epoch. This row scopes it to draw_tri.
- pm_gpu_set_zmark flushes on a change (pm_gpu.c:3722-3728). Keep the skip test: no extra set_zmark(0) may run before nd_geom.c:221.
## Edits
1. nd_set.c: delete the NDR0F1 globals line (:210) and its 4 assignments. Split sync_render (keep every comment of the moved lines):
   ```
   void nd_render_apply(const NdRender* rs, int mixk, unsigned mixc, unsigned omh, unsigned oml, unsigned sg) { /* NDR0F2: the GPU part of sync_render */
       sr_sig = sg; sr_ok = 1; pm_gpu_set_mix(mixk, mixc); fogmix_on = mixk == 2;
       /* the 7 GPU lines of today, pm_gpu_set_nocolor .. pm_gpu_render_src, with rs-> for rs., oml for m, omh for nd_ws.omode_h */
   }
   int nd_sr_same(unsigned sg) { if (sr_ok && sg == sr_sig) { GP_SET(6, 1); return 1; } nd_ent_rfull += nd_ent_in; GP_SET(6, 0); return 0; } /* NDR0F2 */
   void sync_render(void) {
       const unsigned sg = nd_ws.omode_l ^ (nd_ws.omode_h * 2654435761u) ^ nd_ws.geom ^ (unsigned)nd_img_role(0);
       if (nd_sr_same(sg)) return;
       const unsigned m = nd_ws.omode_l, p = (m >> 30) & 3u, a = (m >> 26) & 3u, mm = (m >> 22) & 3u, b = (m >> 18) & 3u;
       unsigned mixc = 0; int mixk = 0; NdRender rs;
       if (nd_mix1(m, &mixc)) mixk = 1;
       else if (!pm_nofog && p == 3u && a == 2u && mm == 0u && b == 0u) { mixk = 2; mixc = (fogc & 0xffffff00u) | 0xffu; } /* F08 */
       nd_render_state(nd_ws.omode_h, m, nd_img_role(0) == ROLE_DEPTH, &rs); nd_render_apply(&rs, mixk, mixc, nd_ws.omode_h, m, sg);
   }
   ```
2. nd_int.h after the nd_mix1 line: the 2 prototypes (nd.h, included at :10, defines NdRender).
3. nd_snap.c: delete `nd_snap_rpend = 1;` in sc_compare, and in nd_snapchk_rnd the `extern NdRender nd_sr_rs ...` line and `nd_snap_rpend = 0;`. Compare with the walker decision:
   `NdRender w; unsigned wc; const int wk = nd_snap_mix(nd_ws.omode_l, fogc, blendc, pm_nofog, &wc); nd_render_state(nd_ws.omode_h, nd_ws.omode_l, nd_img_role(0) == ROLE_DEPTH, &w);`
   then `memcmp(&sc_cur.rs, &w, sizeof w)` and `sc_cur.mixk != wk || sc_cur.mixc != wc`. Add after nd_snapchk_rnd:
   `void nd_snap_render(void) { const unsigned sg = sc_cur.oml ^ (sc_cur.in.omode_h * 2654435761u) ^ sc_cur.geom ^ (unsigned)sc_cur.role; if (PMDL("snapchk")) nd_snapchk_rnd(); if (nd_sr_same(sg)) return; nd_render_apply(&sc_cur.rs, sc_cur.mixk, sc_cur.mixc, sc_cur.in.omode_h, sc_cur.oml, sg); } /* NDR0F2 */`
4. nd_snap.h: declare nd_snap_render. Comments: draw_tri sets nd_snap_rpend; nd_snap_render calls nd_snapchk_rnd.
5. nd_geom.c :16 becomes `__attribute__((weak)) void nd_snap_render(void) { sync_render(); } /* NDR0F2: strong in nd_snap.c */`. gv_epoch_enter :217-218 become
   `if (nd_snap_rpend) nd_snap_render(); /* NDR0F2: the snap of this triangle */` and `else sync_render();`. draw_tri :345: add `nd_snap_rpend = nd_snap_arm; /* NDR0F2 */` before
   `if (nd_snap_arm) nd_snapchk_tri();`. GV1 block :347 becomes `const int done = vert_ring[i0] != GV_NONE && vert_ring[i1] != GV_NONE && vert_ring[i2] != GV_NONE && gv_tri(idx); nd_snap_rpend = 0; if (done) return;`
## Steps and tests (output to build3ds/out/, read the last 20 lines; 1 build, 3 Azahar starts)
1. Edits 1-5. `sh 3ds/tools/nd_host_test.sh` PASS (no new case: no new pure code; the drec compare decides). Commit `NDR0F2 1: mesh render state from the snap`.
2. `python 3ds/tools/bench.py base nimgm_strip_drec,wscmb_pause_drec,spot_mac01_drec` (merge-base, not charged). `sh 3ds/tools/rowloop.sh --row NDR0F2 --bench nimgm_strip_drec,wscmb_pause_drec,spot_mac01_drec`: LOOP PASS, 3 byte-equal.
3. `python 3ds/tools/bench.py run nimgm_strip_snapchk,wscmb_pause_snapchk --row NDR0F2 --why "NDR0F2 snapchk"`; `grep -a snapchk` of both logs > build3ds/out/ndr0f2_snapchk.txt. Pass: bad 0, no `rnd` line.
Expected: byte-equal (FACT: same calls, same values, same skip test). A build after a partial shape cache: `PM_CACHE=off sh 3ds/build.sh`. Fix rights: the listed files; compile errors.
A DIFF, a crash, a `rnd` name: stop, commit the outputs, send the paths to the oracle with SendMessage. Wait.
Stop: last commit `NDR0F2 2: gate. 3 benches byte-equal; snapchk bad <2>` deletes this page. One SubagentHandback.
