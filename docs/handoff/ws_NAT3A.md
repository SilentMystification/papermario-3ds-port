# ws_NAT3A (Haiku): core 0 makes no combine decision: nd_comb_sync keeps only its skip state. Counter: c0gate calls -3, expected `c0gate calls 123 interp 37 or lower, state 23`. Gate: drec byte-equal.
Designer: oracle NATIVE. WAIT: `Merge NAT1D` and `Merge NAT2D`, and the NAT1D hand-back shows `c0diff ... wdiff 0` in all 3 scenes with no c0dwho line for bit 0 draws.
When wdiff is above 0, oracle NATIVE rewrites this page before the start. Worktree build3ds/wt/NAT3A, branch NAT3A. PM_ROW=NAT3A PM_LANE=NAT3A. Wave W2b with NAT3D.
Edit only: 3ds/src/nd_comb.c, function nd_comb_sync (main b485faae44 lines 342-361). nd_comb.c line overlap: NAT1D edits lines 181 and 339 (merged first). No edit in nd_walk.c, nd_set.c, nd_direct.c.
## Facts (main b485faae44)
- FACT nd_comb_sync (nd_comb.c:342-361) has 2 parts. The skip state: sc_sig, sc_ok, sr_ok (:345-349, :356). nd_unit_same reads sc_ok and sc_sig (nd_set.c:126-142), so it decides the gv_epoch cuts.
  The decision and its GPU calls: scm_find, scm_replay, sync_combine_work_timed, combchk (:351-355, :357-359).
- FACT every draw kind takes its combine from a snap: mesh and stream triangles (draw_tri nd_geom.c:353, NAT2G), list and entity ranges (nd_lsr_tri, dlc_seg nd_shape.c:436), particles (nd_geom.c:405, NAT2E), texture rectangles (nd_rect.c:244-245), direct draws (nd_direct.c:65-85).
- FACT a draw with no new snap keeps the GPU state of the last applied snap. NAT1D wdiff 0 says each removed walker decision equals that state.
## Edit: nd_comb_sync becomes (the DIP2 comment of the old first block stays)
```c
void nd_comb_sync(void) {
    {   /* DIP2: ... (the comment stays as it is) */
        unsigned sg = sc_sig_now();
        if (sc_ok && sg == sc_sig) { GP_SET(5, 1); return; }
        sc_sig = sg;
        sc_ok = 0; sr_ok = 0; /* set again below when the work is done */
        nd_ent_cfull += nd_ent_in;
    }
    sc_ok = 1; /* NAT3A: the combine of each draw comes from its snap (core 1, plan_native.md P1). Core 0 keeps the skip state of nd_unit_same only. */
}
```
Delete only lines inside nd_comb_sync. Keep scm_replay, sync_combine_work, sync_combine_work_timed, combchk_run (row NAT6 deletes them). A compile warning for an unused static: add `__attribute__((unused))` to it.
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, 1 build, 6 Azahar starts of 8)
1. The edit. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat3a_host.txt 2>&1`: PASS (hwfix1.sh and nrec2b.sh grep lines in nd_comb_pick and scm_replay: they stay). Commit `NAT3A 1: nd_comb_sync without the decision`.
2. For B in spot_mac01_drec, entcape_kmr03_drec, fxparts_drec: `python 3ds/tools/bench.py base <B> > build3ds/out/nat3a_base_<B>.txt 2>&1`.
3. `sh 3ds/tools/rowloop.sh --row NAT3A --bench spot_mac01_drec,entcape_kmr03_drec,fxparts_drec --no-host --why "NAT3A: no core 0 combine decision" > build3ds/out/nat3a_loop.txt 2>&1`.
4. `grep -H "c0src f\|c0diff\|drec frame\|drec draw .* DIFF" build3ds/azt/logs/spot_mac01_drec.txt build3ds/azt/logs/entcape_kmr03_drec.txt build3ds/azt/logs/fxparts_drec.txt build3ds/out/loop_NAT3A_cmp_*.txt > build3ds/out/nat3a_c0src.txt`.
5. `python 3ds/tools/c0gate.py build3ds/lane-NAT3A/pm_3ds.elf > build3ds/out/nat3a_c0gate.txt 2>&1`.
## Gate (exact)
Host PASS. Rowloop exit 0 (3 drec equal). c0gate: calls 123, interp 37 or lower, state 23. The 3 c0src lines show wcomb 0.
Fix rights: compile errors and unused warnings in nd_comb.c. A DIFF, wcomb above 0 or a tool fault: stop, send build3ds/out/nat3a_c0src.txt to oracle NATIVE with SendMessage, and wait.
## Hand-back
Last commit `NAT3A 2: gate. 3 drec equal; c0gate calls 123` with `git add -f build3ds/out/nat3a_c0src.txt build3ds/out/nat3a_loop.txt`. It deletes this page. One SubagentHandback: hash, c0gate last line, the 3 c0src lines.
