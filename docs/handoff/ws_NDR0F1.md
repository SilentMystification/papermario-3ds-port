# ws_NDR0F1 (Haiku): the snap carries the render block and the blender mix; snapchk compares them with sync_render at the triangle epoch (stage S4; counter: snapchk bad)
Designer: oracle FIN. WAIT: branch from main after the NDR0E2 merge (NDR0E2 edits nd_snap.c). Worktree build3ds/wt/NDR0F1, branch NDR0F1. PM_ROW=NDR0F1 PM_LANE=NDR0F1.
Edit only: 3ds/include/nd_snap.h, 3ds/src/nd_snap.c, 3ds/src/nd_snap_in.c, 3ds/tools/nd_snap_test.c, 3ds/src/nd_set.c (sync_render, 3 globals), 3ds/src/nd_geom.c (3 lines).
## Facts (code read, main b47413357f)
- sync_render (nd_set.c:210-262) picks the mix (:223-236: kind 1 = nd_mix1 colour, kind 2 = fog shade alpha `(fogc & 0xffffff00u) | 0xffu`, else kind 0 colour 0),
  then nd_render_state(omode_h, omode_l, role == ROLE_DEPTH) (:239, pure, nd.h:342) and the GPU calls. nd_mix1 (:191-197) reads the globals blendc, fogc, pm_nofog.
- A mesh triangle compares the snap in nd_snapchk_tri (nd_geom.c:342), then gv_tri runs gv_epoch_enter (:252) and sync_render (:215) at a new epoch.
- NDR0C snapchk: role, oml, geom, fogc, blendc equal on both cores. NdDs holds fogc, blendc, w.omode_h, w.omode_l.
## Edits
1. nd_snap_in.c (pure): `int nd_snap_mix(unsigned ol, unsigned fogc, unsigned blendc, int nofog, unsigned* col)`: the kind of sync_render :223-236 on the arguments
   (kind 1: p 2 or 3, a 1, m 0, b 0, not (p 3 and nofog), col = ((p == 2 ? blendc : fogc) & 0xffffff00u) | (fogc & 0xffu); kind 2: not nofog, p 3, a 2, m 0, b 0,
   col = (fogc & 0xffffff00u) | 0xffu; else 0, col 0). p = ol >> 30 & 3, a = ol >> 26 & 3, m = ol >> 22 & 3, b = ol >> 18 & 3. Declare it in nd_snap.h.
2. nd_snap.h NdSnapChk: add `NdRender rs; int32_t mixk; uint32_t mixc;` (fix the size assert with a pad word if needed).
3. nd_snap.c core 1 (nd_snap_mesh) next to the role line: `r.mixk = nd_snap_mix(nd_ds.w.omode_l, nd_ds.fogc, nd_ds.blendc, pm_nofog, &r.mixc);`
   `nd_render_state(nd_ds.w.omode_h, nd_ds.w.omode_l, r.role == ROLE_DEPTH, &r.rs);` (`extern int pm_nofog;`, a read-only option). Add rs, mixk, mixc to the skip memcmp.
4. nd_set.c: globals `NdRender nd_sr_rs; int nd_sr_mixk; unsigned nd_sr_mixc; /* NDR0F1: the last sync_render work (snapchk) */`. In sync_render set nd_sr_mixk and
   nd_sr_mixc in each mix branch (1, 2, 0 with colour 0), and `nd_sr_rs = rs;` after nd_render_state (:239). No other change.
5. nd_snap.c core 0: global `unsigned char nd_snap_rpend;` (set by draw_tri, edit 6; a texture rectangle never sets it); nd_snap.h declares it and
   nd_snapchk_rnd. New:
   ```
   void nd_snapchk_rnd(void) {   /* NDR0F1: after sync_render of the triangle epoch (nd_geom.c gv_epoch_enter) */
       extern NdRender nd_sr_rs; extern int nd_sr_mixk; extern unsigned nd_sr_mixc;
       nd_snap_rpend = 0;
       if (!PMDL("snapchk")) return;
       sc_nb = 0;
       if (memcmp(&sc_cur.rs, &nd_sr_rs, sizeof nd_sr_rs)) sc_add("rs");
       if (sc_cur.mixk != nd_sr_mixk || sc_cur.mixc != nd_sr_mixc) sc_add("mix");
       /* then a copy of the `if (sc_nb) { ... }` count and log block of sc_compare, log text "snapchk bad f%u rnd%s" */
   }
   ```
6. nd_geom.c: weak `unsigned char nd_snap_rpend __attribute__((weak));` and `__attribute__((weak)) void nd_snapchk_rnd(void) {}` next to the weak nd_snapchk_tri (:14).
   draw_tri (:342): `if (nd_snap_arm) { nd_snapchk_tri(); nd_snap_rpend = 1; } /* NDR0F1 */`. In gv_epoch_enter after `sync_render();` (:215): `if (nd_snap_rpend) nd_snapchk_rnd(); /* NDR0F1 */`.
7. nd_snap_test.c: nd_snap_mix with fogc 0x11223344, blendc 0x55667788. Cases (ol, nofog -> kind, col): (0x84000000, 0 -> 1, 0x55667744),
   (0xC4000000, 0 -> 1, 0x11223344), (0xC4000000, 1 -> 0), (0xC8000000, 0 -> 2, 0x112233ff), (0xC8000000, 1 -> 0), (0x00552078, 0 -> 0). Print `ndr0f mix bad <b>`.
   `--fault` expects kind 1 for (0xC4000000, 1).
## Steps and tests (output to build3ds/out/, read the last 20 lines; 1 build, 4 Azahar starts)
1. Edits 1-7. `sh 3ds/tools/nd_host_test.sh` PASS with `ndr0f mix bad 0` and the seed seen. Commit `NDR0F1 1: snap render block and mix, snapchk at the epoch`.
2. bench.py base nimgm_strip_drec,wscmb_pause_drec (merge-base, not charged). rowloop --row NDR0F1 --bench the same 2: LOOP PASS, byte-equal.
3. bench.py run nimgm_strip_snapchk and wscmb_pause_snapchk; grep -a the snapchk lines > build3ds/out/ndr0f1_snapchk.txt. Pass: recs above 0, no `snapchk bad` line with `rnd` (GUESS: none). Else: commit the grep, send the output path to the oracle, wait.
Fix rights: the listed files; compile errors. A DIFF, a crash: stop, send the output path to the oracle. Wait. Stop: last commit `NDR0F1 2: gate. 2 benches byte-equal; snapchk bad <2>; rnd names <list>` deletes this page. One SubagentHandback.
