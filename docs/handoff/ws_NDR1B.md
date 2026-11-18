# ws_NDR1B (Haiku): the texture rectangle compares its snap after the box fold, then draws with the snap combine (stage S4/W5; counter: snapchk bad)
Designer: oracle FIN. WAIT: branch from main after the NDR0E2 merge (1b01e81eee). Worktree build3ds/wt/NDR1B, branch NDR1B. PM_ROW=NDR1B PM_LANE=NDR1B.
Edit only: 3ds/src/nd_rect.c (2 lines in nd_texrect). NDR0F1 (nd_snap.c, nd_set.c, nd_geom.c) may run at the same time.
## Facts (code read, branch NDR0E2 1b01e81eee)
- NDR0E2 data: nimgm_strip_snapchk bad 486 fbad 4374, all names `use_t1 box_folded tst leg mode5 pat plan_on ls`; wscmb_pause_snapchk bad 0 (3789 compares).
- Cause: nd_texrect compares at its first line (nd_rect.c:221), before its own box_fold_sync (:240). The core 1 snap holds the folded combine (nd_snap_in.c:22).
  A folded tst and use_t1 give another decision, so leg, mode5, pat, plan_on and ls follow. Mesh triangles compare after the fold (nd_geom.c:341-342).
- nd_snapchk_tri (nd_snap.c:125-129) compares (switch snapchk) and then draws with the snap combine (nd_comb_apply). nd_snapchk_rect (:130) compares only.
- The early returns of nd_texrect (depth query, rect_to_image, before :240) draw no texture rectangle with the walker combine.
## Edits
1. Commit 1 (compare after the fold). nd_texrect first line (:221) becomes `const unsigned char sn_armed = nd_snap_arm; nd_snap_arm = 0; /* NDR1B: compared after the fold */`.
   After `box_fold_sync();` (:240) add `if (sn_armed) nd_snapchk_rect(); /* NDR1B */`.
   Before (:240): `    box_fold_sync();` After: `    box_fold_sync();` and the new line `    if (sn_armed) nd_snapchk_rect(); /* NDR1B */`.
2. Commit 2 (only after step 3 passes): replace `nd_snapchk_rect()` in that line by `nd_snapchk_tri()` (the compare, then the snap combine; NDR0E2 code).
## Steps and tests (output to build3ds/out/, read the last 20 lines; 2 builds, 6 Azahar starts)
1. Edit 1. `sh 3ds/tools/nd_host_test.sh` PASS. Commit `NDR1B 1: texture rectangle snap compare after the box fold`.
2. bench.py base nimgm_strip_drec,wscmb_pause_drec (merge-base, not charged). rowloop --row NDR1B --bench the same 2: LOOP PASS, byte-equal.
3. bench.py run nimgm_strip_snapchk and wscmb_pause_snapchk; grep -a the snapchk lines > build3ds/out/ndr1b_snapchk.txt. Pass: bad 0 in the last counter line of
   both (GUESS: bad 0). Else: commit the grep, send the output path to the oracle, wait (no edit 2).
4. Edit 2. `sh 3ds/build.sh`. rowloop --row NDR1B --bench nimgm_strip_drec,wscmb_pause_drec: LOOP PASS, byte-equal. Commit `NDR1B 2: texture rectangles take the snap combine`.
Fix rights: nd_rect.c; compile errors. A DIFF, a crash: stop, send the output path to the oracle. Wait. Stop: last commit `NDR1B 3: gate. 2 benches byte-equal;
snapchk bad <2> fbad <2> miss <2>` deletes this page. One SubagentHandback.
