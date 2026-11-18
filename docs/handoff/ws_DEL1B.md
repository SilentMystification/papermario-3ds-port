# ws_DEL1B (Haiku): the entity path runs its gap and fallback units with nd_ent_units, not with ENT_GBI (nd_ops_run). Counter: C8 reader nd_ent.c 5 -> 0, total 387 -> 382 (FACT listcen 3c2469a282; nd_ent.c tokens: ENT_GBI 4, nd_ops_run 1). Gate: 2 drec byte-equal.
Designer: oracle DEL helper. Start commit main 3c2469a282. No WAIT: NREC4, NCONV2C, DELCEN, NCONV2D are merged. Overlaps: none (this row adds lines after nd_ops_run, nd_walk.c:454). Worktree build3ds/wt/DEL1B, branch DEL1B. PM_ROW=DEL1B PM_LANE=DEL1B.
Edit only (Edit tool, keep CRLF): 3ds/src/nd_ent.c (485-500), 3ds/src/nd_walk.c (after line 454), 3ds/include/nd_int.h (after line 321).
## Facts (code read, main 3c2469a282; find the quoted text, line numbers move)
- FACT ENT_GBI (nd_ent.c:485) is `(nd_ent_gbi++, nd_ops_run((l_), (n_) >> 3, 1))`. Its uses: ent_gap (:488, n units), ent_run_seg_interp (:499 one VTX unit per call, :500 the segment ops). The units are the ND_FS units of k->fw (nd_ent.c:284-288).
- FACT nd_ops_run with keep 1 (nd_walk.c:439-455): census test, gv_epoch++, dlc_on = 1, gp_prev_op = -1, pm_walk_side++, ex_depth + 1, nd_walk_state, restore, gv_streak_flush, tex_streak_flush, census end. nd_nr_exec is nd_walk_state (nd_walk.c:413). nd_ent_units below has the same steps in the same order: byte-equal by construction.
- FACT no host test builds nd_ent.c or nd_walk.c. No host.d script seds them (grep 3ds/tests/host.d).
## Edits
1. nd_walk.c: after the closing `}` of nd_ops_run (the line after `    if (cen) ops_cen_end();`) add:
```c
/* DEL1B: n flat units of an entity list (a gap, the loads and ops of a refused segment) with the list start and end of nd_ops_run (keep 1).
 * ENT_GBI (nd_ent.c) ran them with nd_ops_run; the steps and their order are the same. */
void nd_ent_units(const uint32_t* u, unsigned n) {
    PM_WALK_OWN();
    const int d = ex_depth, cen = __builtin_expect(pm_walk_dbg & (WD_CEN | WD_C2), 0) && d == 0;
    if (cen) ops_cen_begin(u);
    gv_epoch++; dlc_on = 1; gp_prev_op = -1; /* GV1b, as nd_ops_run */
    pm_walk_side++; ex_depth = d + 1;
    nd_nr_exec(u, n);
    ex_depth = d; pm_walk_side--;
    gv_streak_flush(); tex_streak_flush();
    if (cen) ops_cen_end();
}
```
2. nd_int.h: after the line `int nd_ent_pieces(const EntPiece* p, unsigned n); ...` add `void nd_ent_units(const uint32_t* u, unsigned n); /* nd_walk.c, core 0 (DEL1B): n flat units of an entity list, the list start and end of nd_ops_run (keep 1) */`.
3. nd_ent.c. Delete line 485 (`#define ENT_GBI(l_, n_) ...`). Replace (Before -> After):
   :488 `    ENT_GBI((void*)l, n * 8u);` -> `    nd_ent_gbi++; nd_ent_units((const uint32_t*)(const void*)l, n); /* DEL1B */`
   :499 `... == ND_FS_VTX) ENT_GBI((void*)(fw + i), 8u);` -> `... == ND_FS_VTX) { nd_ent_gbi++; nd_ent_units((const uint32_t*)(const void*)(fw + i), 1u); } /* DEL1B */`
   :500 `    if (e->op1 > e->op0) ENT_GBI((void*)(fw + e->op0), (e->op1 - e->op0) * 8u);` -> `    if (e->op1 > e->op0) { nd_ent_gbi++; nd_ent_units((const uint32_t*)(const void*)(fw + e->op0), e->op1 - e->op0); } /* DEL1B */`
   :486 comment: `with nd_ops_run's list start and end` -> `with the list start and end of nd_ent_units`.
4. `git grep -n "ENT_GBI\|nd_ops_run" -- 3ds/src/nd_ent.c > build3ds/out/del1b_grep.txt`: empty.
## Steps (one command per step, output to build3ds/out/, read the last 20 lines; Bash `timeout: 600000`; 1 lane build; at most 4 Azahar starts of 8)
1. Edits 1-4. `sh 3ds/tools/nd_host_test.sh > build3ds/out/del1b_host.txt 2>&1`: PASS. Commit `DEL1B 1: entity units run with nd_ent_units, not ENT_GBI`.
2. `python 3ds/tools/bench.py base entcape_kmr03_drec --commit $(git merge-base HEAD main) > build3ds/out/del1b_base1.txt 2>&1`. Then `sh 3ds/tools/rowloop.sh --row DEL1B --bench entcape_kmr03_drec --no-host --why "DEL1B entity units" > build3ds/out/del1b_loop1.txt 2>&1` (it builds).
3. The same 2 commands for spot_mac01_drec (del1b_base2.txt, del1b_loop2.txt). Then `python 3ds/tools/listcen.py > build3ds/out/del1b_listcen.txt`.
## Gate (exact)
Host PASS. Both rowloops exit 0 (byte-equal). The grep of edit 4 is empty. listcen: no `file reader 3ds/src/nd_ent.c` line, total reader 382, every other total as at the start commit.
## Fix rights and stop
Fix rights: the 3 files, compile errors only. Stop: a DIFF, a crash, a fatal line, a tool fault, a total off the gate. Commit the outputs (`git add -f`), send the paths to the oracle of the launch prompt with SendMessage, wait. Pitfalls: one host test at a time. One bench per rowloop call. PM_CACHE=off only as a prefix of one command.
Hand-back: last commit `DEL1B 2: gate. entcape and spot_mac01 drec equal; reader nd_ent.c 5 -> 0` deletes this page. One SubagentHandback: commit hash, the listcen reader total.
