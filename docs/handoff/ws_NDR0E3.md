# ws_NDR0E3 (Haiku): the snapchk compare of a texture rectangle runs after box_fold_sync, as at a mesh triangle (stage S4; counter: snapchk bad 486 -> 0)
Designer: oracle FIN. Branch from main (NDR0E2 merged, 4df41cf45a). NDR0F1 runs at the same time (it does not edit nd_rect.c). Worktree build3ds/wt/NDR0E3, branch NDR0E3.
PM_ROW=NDR0E3 PM_LANE=NDR0E3. Edit only: 3ds/src/nd_rect.c (2 lines).
## Facts (code read, main cbdf65c35f)
- NDR0E2 gate (FACT, merge 4df41cf45a): nimgm_strip_snapchk bad 486 fbad 4374, names use_t1 box_folded tst leg mode5 pat plan_on ls; wscmb_pause_snapchk bad 0.
- A mesh triangle compares after the fold: draw_tri runs box_fold_sync (nd_geom.c:341), then nd_snapchk_tri (:342).
- A texture rectangle compares before the fold: nd_snapchk_rect at the first line of nd_texrect (nd_rect.c:221), box_fold_sync at :240.
  box_fold_sync writes box_folded, use_t1 and the tst combine fields, then sync_combine writes leg, mode5, pat, the plan and ls (nd_comb.c:377-392).
  The snap holds the folded values (nd_snap_in.c:22-25). So every name of the list comes from a fold that core 0 makes after the compare.
- The early returns of nd_texrect (:232 depth query, :233 rect_to_image) draw no combine. The armed flag must still clear there (nd_rec.c:672 keeps it over NDC_TEXRECT).
## Edit (one shape)
1. nd_rect.c:221. Before: `    if (nd_snap_arm) nd_snapchk_rect(); /* NDR0E2: the armed snapchk compare of a texture rectangle (nd_snap.c) */`
   After: `    const int snap_rect = nd_snap_arm; nd_snap_arm = 0; /* NDR0E3: the compare waits for box_fold_sync (:240), as draw_tri (nd_geom.c:341-342) */`
2. nd_rect.c after :240 `    box_fold_sync();`: `    if (snap_rect) nd_snapchk_rect(); /* NDR0E3 */`
## Steps and tests (output to build3ds/out/, read the last 20 lines; 1 build, 4 Azahar starts)
1. Edits 1-2. `sh 3ds/tools/nd_host_test.sh` PASS. Commit `NDR0E3 1: texture rectangle snapchk after the box fold`.
2. bench.py base nimgm_strip_drec,wscmb_pause_drec (merge-base, not charged). rowloop --row NDR0E3 --bench the same 2: LOOP PASS, byte-equal (the rectangle compare draws nothing).
3. bench.py run nimgm_strip_snapchk and wscmb_pause_snapchk; grep -a the snapchk lines > build3ds/out/ndr0e3_snapchk.txt.
   Pass: recs above 0 in both, bad 0 and fbad 0 in the last counter line of both (GUESS: bad 0). Else: commit the grep, send the output path (with the `snapchk bad` name lines) to the oracle, wait.
Fix rights: nd_rect.c; compile errors. A DIFF, a crash: stop, send the output path to the oracle. Wait.
Stop: last commit `NDR0E3 2: gate. 2 benches byte-equal; snapchk bad <2> fbad <2>; names <list>` deletes this page. One SubagentHandback.
