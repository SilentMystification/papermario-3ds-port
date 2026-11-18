# ws_NREC5 (Haiku): a compiled def inside a list or an entity gap runs as its units on core 0. nd_scomp_exec and nd_scomp_exec_own go. Counter: C11 elf_func -2 (GUESS: both names are in the ELF now). Data: fxops snapchk bad, not higher than base (the sl names are NREC2B's fix, not this row).
Designer: oracle NCONV2RA. Start from main (73359b439a). No WAIT: NREC4 edits nd_walk.c 457-530, this row edits nd_walk.c 581-632 and no NREC4 file else. Worktree build3ds/wt/NREC5, branch NREC5. PM_ROW=NREC5 PM_LANE=NREC5.
Edit only (Edit tool, keep CRLF): 3ds/src/nd_scomp.c (347-359), 3ds/include/nd_scomp.h (42-43), 3ds/src/nd_walk.c (gt_ops, nd_ent_pieces), 3ds/src/nd_ent.c (ent_gap), 3ds/include/nd_gtab.h (line 16 comment).
## Facts (code read, main 73359b439a; find the quoted text, line numbers move)
- FACT nd_scomp_exec_own has 3 callers: nd_walk.c:586 (gt_ops), nd_walk.c:623 (nd_ent_pieces), nd_ent.c:489 (ent_gap). nd_scomp_exec has 1 caller (nd_scomp.c:358). No host test calls them (git grep).
- FACT the compiled run (sc_exec, nd_scomp.c:260-266) ends with `sr_ok = 0` and `gv_epoch++`. Its combine decision is a cache per key (sc_decide :226, sc_replay :207 writes sl_const and sl_cc).
- FACT the units of such a def run in def order with every value unit first (nv == pre), so the op path gives the same state order. Each unit run takes the walker decision (sync_combine) as the op path does.
- FACT ENT_GBI (nd_ent.c:485) is nd_ops_run(l, n, 1): gv_epoch++, dlc_on = 1, the units, the streak flushes (nd_walk.c:436-452). The old compiled branch of ent_gap did the same plus sc_exec.
- FACT the next snap draw applies its own combine (nd_snap_comb, nd_snap.c:233), so the combine of the op path does not reach a list draw. GUESS: drec stays equal.
## Edits
1. nd_scomp.c. Replace `int nd_scomp_exec(...) {` to the end of `int nd_scomp_exec_own(...) { ... }` (lines 347-359, before `#endif`) with:
```c
/* NREC5, core 0: 1 when the compiler takes d (st 1). The callers run the units of d and keep the cut and the render resync of the old compiled run (sc_exec). */
int nd_scomp_ok(NdSDef* d) {
    if (__builtin_expect(!d->st, 0)) d->st = nd_scomp_analyse(d) ? 1u : 2u;
    return d->st == 1u;
}
```
2. nd_scomp.h. Replace the 2 lines `int nd_scomp_exec(...` and `int nd_scomp_exec_own(...` with `int nd_scomp_ok(NdSDef* d); /* NREC5, core 0: 1 when the compiler takes d */`.
3. nd_walk.c gt_ops, case NDG_DEF. Before (2 lines):
   `            if (g->d.nv == g->d.pre && nd_scomp_exec_own((NdSDef*)&g->d)) break; /* st is 1: no write to the table */`
   `            nd_nr_exec((const uint32_t*)g->d.u, g->d.n);`
   After (2 lines):
   `            nd_nr_exec((const uint32_t*)g->d.u, g->d.n);`
   `            if (g->d.nv == g->d.pre && nd_scomp_ok((NdSDef*)&g->d)) { gv_epoch++; sr_ok = 0; } /* NREC5: the cut and render resync of the old compiled run (sc_exec); st is 1: no write to the table */`
4. nd_walk.c nd_ent_pieces, the ENP_DEF branch: the same shape. Before: `if (g->d.nv == g->d.pre && nd_scomp_exec_own((NdSDef*)&g->d)) { sc++; continue; }` then the `nd_nr_exec` line. After: the `nd_nr_exec` line, then `if (g->d.nv == g->d.pre && nd_scomp_ok((NdSDef*)&g->d)) { gv_epoch++; sr_ok = 0; } /* NREC5 */`. Keep `int sc = 0;` and `return sc;` (it returns 0 now).
5. nd_ent.c ent_gap. Delete the 3 lines from `    if (d && d->st == 1u && d->nv == d->pre) {` to `        if (ok) { nd_ent_sc++; gv_streak_flush(); tex_streak_flush(); return; } }`. After `    ENT_GBI((void*)l, n * 8u);` add `    if (d && d->st == 1u && d->nv == d->pre) sr_ok = 0; /* NREC5: the render resync of the old compiled run */`. Keep the analyse line above.
6. nd_gtab.h:16. Before: `(nd_scomp_exec_own(&def->d) runs it)` After: `(its units run as ops, NREC5)`.
## Steps (one command per step, output to build3ds/out/, read the last 20 lines; Bash `timeout: 600000`; 1 lane build; at most 4 Azahar starts of 8)
1. Edits 1-6. `git grep -n "nd_scomp_exec" -- 3ds/src 3ds/include src include > build3ds/out/nrec5_grep.txt`: empty. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nrec5_host.txt 2>&1`: PASS. Commit `NREC5 1: compiled defs in lists and entity gaps run as units`.
2. `python 3ds/tools/bench.py base fxops_drec --commit $(git merge-base HEAD main) > build3ds/out/nrec5_base1.txt 2>&1`. Then `sh 3ds/tools/rowloop.sh --row NREC5 --bench fxops_drec --no-host --why "NREC5 defs as units" > build3ds/out/nrec5_loop1.txt 2>&1` (it builds).
3. The same 2 commands for entcape_kmr03_drec (nrec5_base2.txt, nrec5_loop2.txt).
4. `python 3ds/tools/bench.py base fxops_snapchk --commit $(git merge-base HEAD main) > build3ds/out/nrec5_base3.txt 2>&1`. Then `PM_ROW=NREC5 PM_LANE=NREC5 python 3ds/tools/bench.py run fxops_snapchk --row NREC5 --why "NREC5 sl" > build3ds/out/nrec5_run3.txt 2>&1`. Then `grep -a "snapchk f\|snapchk bad\|lsnap f" build3ds/azt/logs/fxops_snapchk.txt > build3ds/out/nrec5_lines.txt`.
Gate (exact): host PASS. Both rowloops exit 0 (byte-equal). Last fxops_snapchk `snapchk f` line: bad not higher than the base run. Last lsnap line: tmis 0. Report bad and the first bad names (data).
Stop: a DIFF, bad higher than base, a crash, a fatal line, a tool fault. Commit the outputs (`git add -f`), send the paths to the oracle of the launch prompt with SendMessage, wait. Fix rights: the files of this page, compile errors only. Pitfalls: one host test at a time. One bench per rowloop call. PM_CACHE=off only as a prefix of one command.
Hand-back: last commit `NREC5 2: gate. fxops and entcape drec equal; snapchk bad <base> to <n>` deletes this page. One SubagentHandback: commit hash, the last snapchk and lsnap lines.
