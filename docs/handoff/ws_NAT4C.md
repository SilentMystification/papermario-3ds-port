# ws_NAT4C (Haiku): stream combine-class units cut where core 1 marks, and a unit with no mark is skipped (the W3 cut rule). Counter: c0gate unchanged, expected `c0gate calls 122 interp 34 state 23`. Gate: expected-diff (USER DECISION UD-N5).
Designer: oracle NATIVE. WAIT: the user's answer to UD-N5 (default yes). Branch from main (430cdca990 or later). Worktree build3ds/wt/NAT4C, branch NAT4C. PM_ROW=NAT4C PM_LANE=NAT4C.
Edit only: 3ds/src/nd_walk.c (epk_note, main line 237-ish, and nr_set). No other file. No other row edits nd_walk.c now.
## Facts (NAT4B 0e1c0ac93c)
- FACT epbin: all disagreements but 1 are id 45 (ND_FS_COMB) dir 0: core 0 cuts, core 1 has no mark (its change bit, sc_lost and sc_dirty are 0: the combine words are unchanged).
  Core 0 cuts there only for its bookkeeping: sc_sig_now() != sc_sig (sig 0), or box_folded 1 with cmb_gen_seen != comb_gen (nd_unit_same nd_set.c:140-142). 1 case: id 33 (ND_FS_OML) dir 1 (a mark, core 0 skips).
- FACT after NAT3A no walker decision follows such a unit (nd_comb_sync keeps only its skip state), so the core 0 cut only starts a new batch with the same GPU state: the drec draw count falls, each draw's state stays. GUESS: no pixel changes.
## Edits (new and changed lines end with `/* NAT4C */`)
1. epk_note: before its `if (epk_nl < 24u ...` line add `    { static unsigned fr = ~0u; if (pm_drec_now() && pm_frame_count() != fr) { fr = pm_frame_count(); pm_log_evt("epcut f%u", fr); } } /* NAT4C: one line per drec frame with a changed cut */`.
2. nr_set: the first two lines become
```c
    const int same = nd_unit_same(u); int cut = !same; /* NAT4C: the cut of this unit */
    { const unsigned i_ = u[0] >> 24; if (ep_stream && ex_depth == 0 && (i_ == ND_FS_OML || (i_ >= ND_FS_PRIM && i_ <= ND_FS_BLEND) || i_ == ND_FS_RARE || i_ == ND_FS_COMB)) {
        nd_epk_n++; if ((same == 0) != (ep_mark != 0)) { nd_epk_bad++; epk_note(i_, same, u); }
        cut = ep_mark != 0; ep_mark = 0; if (!cut) return; } } /* NAT4C: the core 1 mark decides the cut; no mark: no change (core 1 change bit 0) */
    if (same && !cut) return;
```
   Then `gv_epoch++;` becomes `if (cut) gv_epoch++;`, and `nd_unit_apply(u);` becomes `if (!same) nd_unit_apply(u);`. The other lines stay.
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, 1 build, 6 Azahar starts of 8)
1. Edits 1-2. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat4c_host.txt 2>&1`: PASS. Commit `NAT4C 1: the core 1 mark cuts stream combine units`.
2. For B in spot_mac01_drec, entcape_kmr03_drec, fxparts_drec: `python 3ds/tools/bench.py base <B> > build3ds/out/nat4c_base_<B>.txt 2>&1`.
3. `sh 3ds/tools/rowloop.sh --row NAT4C --bench spot_mac01_drec,entcape_kmr03_drec,fxparts_drec --no-host --why "NAT4C: core 1 cut, expected diff" > build3ds/out/nat4c_loop.txt 2>&1`.
4. `grep -H "epchk\|epcut\|epbin\|c0src f\|drec frame\|drec draw .* DIFF" build3ds/azt/logs/*_drec.txt build3ds/out/loop_NAT4C_cmp_*.txt > build3ds/out/nat4c_epchk.txt`. 5. `python 3ds/tools/c0gate.py build3ds/lane-NAT4C/pm_3ds.elf > build3ds/out/nat4c_c0gate.txt 2>&1`.
## Gate (exact, expected-diff rule)
Host PASS. For each bench: the cmp is equal, or its first DIFF draw seq S lies in frame F (the `drec frame F seq s` line with the largest s at or below S) and the log has `epcut fF`. c0gate `calls 122 interp 34 state 23`.
Fix rights: compile errors. A DIFF in a frame with no epcut line, a crash or a tool fault: stop, send nat4c_epchk.txt to oracle NATIVE with SendMessage, wait.
## Hand-back
Last commit `NAT4C 2: gate. expected-diff: <bench> DIFF draw S frame F epcut, ...` (the word `expected-diff` must be in it) with `git add -f build3ds/out/nat4c_epchk.txt build3ds/out/nat4c_loop.txt`. It deletes this page.
One SubagentHandback: hash, the gate line per bench, the epchk lines, the c0src draws per bench (base and row).
