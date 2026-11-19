# ws_NAT2E (Haiku): a particle instance applies the combine of its snap (UD-N2 yes: expected diff). Counter: c0gate unchanged, expected `c0gate calls 127 interp 37 state 23`. c0src fxparts wcomb falls.
Designer: oracle NATIVE. Branch from main. Worktree build3ds/wt/NAT2E, branch NAT2E. PM_ROW=NAT2E PM_LANE=NAT2E. Wave W1 with NAT1B.
Edit only: 3ds/src/nd_geom.c (line 404 and 1 new line above line 370), 3ds/src/nd_snap.c (new lines after line 218 only. NAT1B edits nd_snap.c lines 132-134 and 177: no overlap).
This row does not touch nd_walk.c, nd_set.c, nd_comb.c or nd_direct.c.
## Facts (main 18ecf83ea0)
- FACT nd_part_draw (nd_geom.c:370) arms the instance snap (nd_lsr_tri, :401) but only clears the arm (:404). It never applies the snap combine. draw_tri applies it with nd_snapchk_tri (:353).
- FACT nd_parts_run_v sets pm_comb_vprim 1 and sc_ok 0 with no sync (nd_walk.c:466). So the walker can draw a particle with the vprim 0 plan. The core 1 instance snap has vprim 1 (nd_snap.c:69).
- FACT oracle_handoff.md:26 (NREC4): the walker draws snowfall with the stale vprim 0 plan (drec r 0cb, a TEV constant that the stage setup does not read). This is the expected diff of UD-N2.
- FACT drec_cmp (bench.py:1658) names only the first DIFF draw: `drec draw <seq> DIFF`. Each drec frame logs `drec frame <f> seq <s>` (pm_gpu.c:607). fxparts_drec and spot_mac01_drec run at loglevel debug.
## Edits
1. nd_snap.c, after line 218 (the end of the file), add:
```c
/* NAT2E: one debug line per drec frame in which a particle instance snap gives another combine than the walker has now (UD-N2 expected diff) */
void nd_snap_plog(void) {
    static unsigned ps_fr = ~0u;
    if (!pm_drec_now() || pm_frame_count() == ps_fr) return;
    const NdCombOut* o = &sc_cur.o;
    if (o->plan_on != plan_active || (o->plan_on ? memcmp(&o->plan, &cur_plan, sizeof cur_plan) != 0 : o->leg != leg_mode)) {
        ps_fr = pm_frame_count();
        pm_log_d("partsnap f%u", ps_fr);
    }
}
```
   (plan_active, cur_plan, leg_mode: nd_int.h:253-254. pm_drec_now, pm_frame_count: nd_snap.c:94-95.)
2. nd_geom.c, a new line above `int nd_part_draw(const NdNDev* t) {` (line 370): `void nd_snap_plog(void); /* NAT2E: nd_snap.c */`.
3. nd_geom.c line 404 before: `            nd_snap_rpend = nd_ls_on; if (sn) { nd_snap_arm = 0; if (ep_epoch == gv_epoch) nd_snap_rpend = 0; }`
   after: `            nd_snap_rpend = nd_ls_on; if (sn) { nd_snap_plog(); nd_snapchk_tri(); if (ep_epoch == gv_epoch) nd_snap_rpend = 0; } /* NAT2E: the instance snap combine, as draw_tri :353 */`
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, 1 build, 4 Azahar starts of 8)
1. Edits 1-3. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat2e_host.txt 2>&1`: PASS (the 3 CACHECHECK lines are known, oracle_handoff.md:49). Commit `NAT2E 1: particle instance snap combine`.
2. For B in fxparts_drec, spot_mac01_drec, one call each: `python 3ds/tools/bench.py base <B> > build3ds/out/nat2e_base_<B>.txt 2>&1`.
3. `sh 3ds/tools/rowloop.sh --row NAT2E --bench fxparts_drec,spot_mac01_drec --no-host --why "NAT2E: particle snap combine, expected diff" > build3ds/out/nat2e_loop.txt 2>&1`.
4. `grep -H "drec frame\|partsnap f\|c0src f\|drec draw .* DIFF" build3ds/azt/logs/fxparts_drec.txt build3ds/azt/logs/spot_mac01_drec.txt build3ds/out/loop_NAT2E_cmp_*.txt > build3ds/out/nat2e_diff.txt`.
5. `python 3ds/tools/c0gate.py build3ds/lane-NAT2E/pm_3ds.elf > build3ds/out/nat2e_c0gate.txt 2>&1`.
## Gate (exact, expected-diff rule)
Host PASS. Build rc 0. For each bench: the cmp is equal, or its first DIFF draw seq S lies in frame F (the `drec frame F seq s` line with the largest s at or below S) and the same log has `partsnap fF`.
c0gate last line `c0gate calls 127 interp 37 state 23`. Both benches have a c0src line.
Fix rights: compile errors in the edited files. A DIFF in a frame with no partsnap line, a crash or a tool fault: stop, send build3ds/out/nat2e_diff.txt to oracle NATIVE with SendMessage.
## Hand-back
Last commit `NAT2E 2: gate. expected-diff: fxparts <equal or DIFF draw S frame F partsnap>, spot_mac01 <same form>` with `git add -f build3ds/out/nat2e_diff.txt build3ds/out/nat2e_loop.txt`. It deletes this page.
The word `expected-diff` must be in that message (the S9 gates find the new drec base with it). One SubagentHandback: the commit hash, the gate line per bench, the 2 c0src lines.
