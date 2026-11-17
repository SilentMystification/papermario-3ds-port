# ws_FXSLOT2 (Haiku): captured entries of the 5 FXSLOT sites (C10 recdl -5)
Designer: oracle DIAG1. Worktree build3ds/wt/FXSLOT2, branch FXSLOT2 from main f60e8cd0b4 or later. PM_ROW=FXSLOT2 PM_LANE=FXSLOT2.
Edit only src/effects/chomp_drop.c, src/effects/disable_x.c, src/status_star_shimmer.c (entapply edits them); add
3ds/tools/ntab_ent.d/FXSLOT2.txt.
## Facts (code read, main f60e8cd0b4)
- cap3.txt has 5 lines, 1 tuple per site, no multi (FACT). Sites: status_star_shimmer.c:189, chomp_drop.c:163, :166,
  disable_x.c:308, :310. Each line on main is the plain form `nd_rec_dl(NDN(<list>));` (FACT, grep). Each file has
  `#include "nd_gtab.h"` (FACT: shimmer :2, chomp_drop :3, disable_x :3).
- The 5 lists hold only vertex, triangle and pipesync ops. The shimmer loop gives 1 tuple, so no first/later split (FACT, cap3).
- gen_gfx_tab.sh:28 scans src/effects and the files of ntab_ent.d. status_star_shimmer.c is in src root: it needs the new file.
- recdl on main is 10 (FACT, orchestrator). fxslot_drec (azt.d/FXSLOT.json:32) records f412, f567, f722 and has psochk.
## Edit shape (entapply does it; status_star_shimmer.c:189)
Before: `                    nd_rec_dl(NDN(D_801097D8));`
After:  `                    nd_draw_list(NDN(D_801097D8), pe_status_star_shimmer_189);` and after line 2 the new line
`ND_PSO_DESC(pe_status_star_shimmer_189, 0xfc16120b, 0x540dc93f, ...);` (desc_line, entapply.py:37-41).
## Steps (each output to build3ds/out/, read the last 20 lines)
1. `python 3ds/tools/entapply.py 3ds/tools/fxrest.d/cap3.txt > build3ds/out/fxslot2_apply.txt`. Expected: 5 `entapply site`
   lines (pe_status_star_shimmer_189, pe_chomp_drop_163, pe_chomp_drop_166, pe_disable_x_308, pe_disable_x_310) and the last
   line `entapply sites 5 keyed 0 so 0 multi 0 kmiss 0 mixed 0 skip 0 other 0 files 3`.
2. Add ntab_ent.d/FXSLOT2.txt: line 1 `# FXSLOT2: sites with a captured entry (status_star_shimmer.c:189)`, line 2
   `src/status_star_shimmer.c`. Commit `FXSLOT2 1: captured entries of 5 sites (cap3.txt)`.
3. `python 3ds/tools/bench.py base fxslot_drec --row FXSLOT2 --commit $(git merge-base HEAD main)` (base run, not charged).
4. `PM_HOSTSEL=all sh 3ds/tools/rowloop.sh --row FXSLOT2 --bench fxslot_drec --why "FXSLOT2 gate" > build3ds/out/fxslot2_loop.txt 2>&1`.
5. `grep -h "psochk end" ../../basecache/$(git merge-base HEAD main)/fxslot_drec/run.log <the log path of the ingested line in
   build3ds/out/loop_FXSLOT2_run.txt> > build3ds/out/fxslot2_psochk.txt`. Line 1 is the base, line 2 is the gate.
## Tests that decide (1 build, 1 Azahar start; the base is not charged)
- `LOOP PASS`: host PASS, build rc 0, fxslot_drec byte-equal.
- psochk end: gate bad equal to base bad, gate draws higher than base draws, lkbad 0. GUESS: base bad 185 (capture end f730,
  commit 795963f08a). The base line decides, not the guess.
- listcen on HEAD vs `$(git merge-base HEAD main)`: recdl -5 (10 -> 5), no other change.
## Fix rights: only the 4 files. A skip, multi or other line, a missing desc name in build3ds/out/ntab_entries.txt, a compile
  error, a DIFF, a higher bad or no psochk end line: stop, send the output path to DIAG1 and wait.
## Stop: last commit `FXSLOT2 2: gate. fxslot_drec byte-equal, psochk bad <b> equal base, draws <base> -> <gate>; recdl <n>`
  deletes this page. One SubagentHandback: hashes, LOOP line, psochk lines, counters.
