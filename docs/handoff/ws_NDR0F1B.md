# ws_NDR0F1B (Haiku): fix of the NDR0F1 gate: the render compare flag lives for one triangle only (stage S4; counter: snapchk bad, rnd names to 0)
Designer: oracle FIN. Continue on branch NDR0F1 (head 74dc64fe54), worktree build3ds/wt/NDR0F1. PM_ROW=NDR0F1 PM_LANE=NDR0F1. Then finish ws_NDR0F1.md (its stop line).
Edit only: 3ds/src/nd_snap.c, 3ds/include/nd_snap.h (1 comment), 3ds/src/nd_geom.c (draw_tri).
## Cause (code read, NDR0F1 74dc64fe54)
- nd_snap.c:128 sets `nd_snap_rpend = 1` in sc_compare. A texture rectangle runs sc_compare too (nd_rect.c nd_snapchk_rect on this base). A rectangle has no
  draw_tri, so the flag stays 1 with sc_cur = the rectangle snap. A rectangle bumps gv_epoch (nd_tex_set.c:231-233 TX_OP), so the next draw opens a new epoch.
  When that draw has no snap (a static list triangle in nd_nr_run, a model record, nd_geom.c:384 particles, nd_shape.c:433 shapes), gv_epoch_enter
  (nd_geom.c:218) compares the rectangle snap rs with the render of that other draw: `rnd rs`.
- Also: an armed triangle that gv_tri_core rejects before nd_geom.c:252 (all outside :238/:243, or SCR, TSS, LIT :245-249) keeps the flag for a later unarmed epoch.
- 74dc64fe54 covers only the triangle with no new epoch. GUESS: the 2 paths above give all bad lines (wscmb f37-f44: title screen rectangles and lists).
## Edits
1. nd_snap.c sc_compare: delete the line `nd_snap_rpend = 1; /* NDR0F1: ... */` (:128).
2. nd_snap.c nd_snapchk_rnd: after `nd_snap_rpend = 0;` (:133) add `if (!PMDL("snapchk")) return; /* NDR0F1B: draw_tri sets the flag in every run */`.
3. nd_geom.c draw_tri. Before (:345): `    if (nd_snap_arm) { nd_snapchk_tri(); if (ep_epoch == gv_epoch && nd_snap_rpend) nd_snapchk_rnd(); } /* NDR0A ... */`
   After: one new line above it: `    nd_snap_rpend = nd_snap_arm; /* NDR0F1B: only the epoch of this triangle compares its snap; a rectangle never sets it */` (the :345 line stays).
   Before (:347): `        if (vert_ring[i0] != GV_NONE && vert_ring[i1] != GV_NONE && vert_ring[i2] != GV_NONE && gv_tri(idx)) return;`
   After: `        const int done = vert_ring[i0] != GV_NONE && vert_ring[i1] != GV_NONE && vert_ring[i2] != GV_NONE && gv_tri(idx);`
   and `        nd_snap_rpend = 0; /* NDR0F1B */` and `        if (done) return;`
4. nd_snap.h :38 comment and nd_snap.c :66 comment: `set by draw_tri for its own triangle, cleared after gv_tri and by nd_snapchk_rnd`.
## Steps and tests (output to build3ds/out/, read the last 20 lines; 1 build, 2 Azahar starts; the budget is reset once for this correction)
1. Edits 1-4. `sh 3ds/tools/nd_host_test.sh` PASS. Commit `NDR0F1 4: fix: the render compare flag lives for its triangle only (ws_NDR0F1B)`.
2. `sh 3ds/tools/rowloop.sh --row NDR0F1 --bench nimgm_strip_drec,wscmb_pause_drec`: LOOP PASS, byte-equal (the base runs of NDR0F1 2 stay valid: same merge-base).
3. `python 3ds/tools/bench.py run nimgm_strip_snapchk,wscmb_pause_snapchk --row NDR0F1 --why "NDR0F1B rnd scope"`; `grep -a snapchk` of each log > build3ds/out/ndr0f1b_snapchk_<bench>.txt.
   Pass: no `rnd` line; bad equal to NDR0E2 (wscmb 0; nimgm 486, the rectangle fold names; FACT 4df41cf45a).
Fix rights: the listed files; compile errors. A DIFF, a crash, a `rnd` line left: stop, commit the outputs, send the paths to the oracle with SendMessage. Wait.
Stop: delete this page in the NDR0F1 stop commit of ws_NDR0F1.md (both pages). One SubagentHandback.
