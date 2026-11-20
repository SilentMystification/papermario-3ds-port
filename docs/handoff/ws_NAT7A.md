# ws_NAT7A (Haiku: deletions of dead code in 2 source files and 1 host script): the dead combine work and the weak host render fallback go. Counter: C13 c0gate calls 90 to 86, interp 34, state 23 (FACT: c0gate.py on a clean git archive of main 9576c03383 with these deletions, ELF build3ds/basebin/ae5aded95a75afbd).
Designer: oracle NATIVE6 (a0b4e9ef29efcb10a). Start commit: main. Worktree build3ds/wt/NAT7A, branch NAT7A. PM_ROW=NAT7A PM_LANE=NAT7A. Owns: 3ds/src/nd_comb.c (only the lines below), 3ds/src/nd_geom.c (:16), 3ds/tests/host.d/nrec2b.sh. Do not edit nd_comb_sync, box_fold_sync, comb_in_walker or the skip state (NAT6).
## Facts (main 9576c03383)
- FACT sync_combine_work_timed (nd_comb.c:301) has no caller in 3ds/src, 3ds/include, src, 3ds/tests or 3ds/tools. sync_combine_work (:333) has only that caller. scw_census (:308) and scm_sig (:161) have only sync_combine_work. scm_find, scm_store, scm_replay (:169-193) have no caller. None of these is a FUNC symbol in the ELF.
- FACT no host unit compiles nd_geom.c. The strong nd_snap_render is nd_snap.c:144. nd_direct_test.c links nd_snap_link_stub.c:5 for its own.
- FACT host.d/nrec2b.sh check A greps the scm_replay line :183. nd_comb.c and nd_geom.c have CRLF line ends: use the Edit tool, never sed.
## Edits (one commit)
1. nd_comb.c: delete :45 (typedef ScMemo), :61 (`static void sync_combine_work(void);`), :68-69 (scm, scm_next), :161-193 (scm_sig, scm_find, scm_store, the nd_snap_cdiff declaration line, scm_replay), :301-313 (sync_combine_work_timed, scw_census), :333-343 (sync_combine_work). Delete the blank line after each deleted block. Example, before:
```c
void sync_combine_work_timed(void) {
    if (!pm_ph_cen || !ph_pick()) { sync_combine_work(); return; } /* PROF2-2: phase mode only */
    unsigned t0 = pm_ticks();
    sync_combine_work();
    pm_gbi_sync_ticks += (pm_ticks() - t0) * 8u; /* W2: one call in 8 timed */
}
```
   After: no line. Keep :59 (ph_pick declaration), pm_gbi_sync_ticks (nd_cen.c, pm_phase.c) and nd_snap_cdiff in nd_snap.c.
2. nd_comb.c comments. Line 2, before: `* nd_comb_sync work call pm_gpu_*. The typedef ScMemo is a copy of the former pm_gbi.c one (ST-2). The decision itself is nd_decide.c (NPSO2). */`. After: `* nd_comb_sync work call pm_gpu_*. The decision itself is nd_decide.c (NPSO2). */`. Line 315, before: `/* NDR0E1: the GPU part of sync_combine_work. o is`. After: `/* NDR0E1: the GPU part of a combine decision. o is` (rest of the line as it is).
3. nd_geom.c: delete :16 `__attribute__((weak)) void nd_snap_render(void) { nd_rnd_sync(); } /* NDR0F2: strong in nd_snap.c */`.
4. nrec2b.sh: delete L1, chk_a, the `chk_a 3ds/src/nd_comb.c` line and the nd_comb.c seed lines. Add `chk_b() { [ "$(grep -cF "$L2" "$1")" = 1 ]; }`. Check B becomes `chk_b 3ds/src/nd_direct.c || { echo "nrec2b FAIL B"; exit 1; }`. The seed: `sed 's/sl_const = o->cc_on; //' 3ds/src/nd_direct.c > $N/seed/nd_direct.c`, then the same `cmp -s` and `if chk_b $N/seed/nd_direct.c` lines as before, on nd_direct.c. The last line prints `nrec2b ok 1`. Edit the header comments to name nd_direct_bind only.
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`)
1. Edits 1-4. Commit `NAT7A 1: the dead combine work and the weak host render fallback are deleted`.
2. For B in spot_mac01_drec, entcape_kmr03_drec, fxparts_drec, wscmb_pause_drec: `python 3ds/tools/bench.py base <B> > build3ds/out/nat7a_base_<B>.txt 2>&1`.
3. `sh 3ds/tools/rowloop.sh --row NAT7A --bench spot_mac01_drec,entcape_kmr03_drec,fxparts_drec,wscmb_pause_drec --why "NAT7A: dead combine work" > build3ds/out/nat7a_loop.txt 2>&1`. It runs the host test, 1 build, 1 bench run (2 on a cold start).
4. `python 3ds/tools/c0gate.py build3ds/lane-NAT7A/pm_3ds.elf > build3ds/out/nat7a_c0gate.txt 2>&1`.
Budget: 1 build, 4 Azahar starts.
## Gate (exact)
- rowloop exit 0: host PASS, `nrec2b ok 1` in build3ds/out/loop_NAT7A_host.txt, 4 drec cmp equal against `git merge-base HEAD main`.
- Last line of nat7a_c0gate.txt: `c0gate calls 86 interp 34 state 23`. When main moved before your start: calls = the start-commit calls minus 4, interp and state as the start commit.
## Fix rights and stop
- You may fix: a compile error from a declaration that only a deleted function used (delete that declaration), the nrec2b.sh text, a lkey count in 3ds/tests/lkey/allow.txt.
- Stop on a DIFF, another c0gate line, a crash or a tool fault. Send the output path and the first DIFF line to oracle NATIVE6 (a0b4e9ef29efcb10a) with SendMessage. Wait for the reply.
## Hand-back
Last commit `NAT7A 2: gate. <rowloop summary line> <c0gate line>` with `git add -f build3ds/out/nat7a_loop.txt build3ds/out/nat7a_c0gate.txt`. It deletes this page. One SubagentHandback: hashes, gate line per bench, c0gate line.
