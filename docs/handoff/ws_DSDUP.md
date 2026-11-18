# ws_DSDUP (Haiku): nd_scomp_rec feeds the core 1 state of a compiled def once, not twice. Counter: none of C1-C12 (a core 1 fault fix). Gate: drec byte-equal and the snapchk and lsnap lines equal to the base run, so a change of the core 1 snap state shows.
Designer: oracle DEL helper. Start commit main (2f28f3b558 or later). No WAIT. Shared files: DEL3B edits nd_scomp.c 127-137 and 169-346 (3 lines below this row's 163-165; no shared line). DEL3A (W9, nd_rec_nr) may edit rec_ops (nd_scomp.c:140-153): this row edits only 154-166. DEL5B2 shares no file. Worktree build3ds/wt/DSDUP, branch DSDUP. PM_ROW=DSDUP PM_LANE=DSDUP.
Edit only (Edit tool, keep CRLF): 3ds/src/nd_scomp.c (154-166).
## Facts (code read, main 2f28f3b558)
- FACT nd_scomp_rec (nd_scomp.c:154-167) calls `nd_ds_scomp(&nd_ds, ...)` at :164 (inside `#ifndef ND_SCOMP_HOST`, :163-165) and again at :166. The whole core 1 and core 0 part is already inside `#ifndef ND_SCOMP_HOST` (:120-352), so both calls run on every compiled record. NST2C (118f24b6c6) added both.
- FACT nd_ds_scomp (nd_ds.c:569) calls nd_ds_unit per unit. nd_ds_unit (nd_rec.c:252-257) applies the unit and runs the ep rule nd_ds_ep_rule with sc_lost and sc_dirty. GUESS: the second pass applies equal fields but can raise a core 1 epoch (DS_EP). So the snap state can change: this row has its own snapchk gate.
## Edits
1. nd_scomp.c: delete the 3 lines from `#ifndef ND_SCOMP_HOST /* NST2C: the host builds link this file without nd_ds.c */` (:163) to its `#endif` (:165). Keep :166 (`nd_ds_scomp(...); /* NST2C */`).
2. nd_scomp.c, nd_scomp_rec: after the line `nd_rec_napi(nd_scomp_run, p, 1u + nv);` add `{ static unsigned char seen; if (!seen) { seen = 1; pm_log_evt("scomp rec f%u", pm_frame_count()); } } /* DSDUP: a compiled record was taken */`.
3. `grep -c "nd_ds_scomp(" 3ds/src/nd_scomp.c > build3ds/out/dsdup_grep.txt`: 1.
## Steps (one command per step, output to build3ds/out/, read the last 20 lines; Bash `timeout: 600000`; 1 lane build; at most 4 Azahar starts of 8)
1. Edits 1-3. `sh 3ds/tools/nd_host_test.sh > build3ds/out/dsdup_host.txt 2>&1`: PASS. Commit `DSDUP 1: one core 1 feed per compiled def record`.
2. `python 3ds/tools/bench.py base entcape_kmr03_drec --commit $(git merge-base HEAD main) > build3ds/out/dsdup_base1.txt 2>&1`. Then `sh 3ds/tools/rowloop.sh --row DSDUP --bench entcape_kmr03_drec --no-host --why "DSDUP drec" > build3ds/out/dsdup_loop1.txt 2>&1` (it builds).
3. `python 3ds/tools/bench.py base entcape_kmr03_snapchk --commit $(git merge-base HEAD main) > build3ds/out/dsdup_base2.txt 2>&1`. Then `PM_ROW=DSDUP PM_LANE=DSDUP python 3ds/tools/bench.py run entcape_kmr03_snapchk --row DSDUP --why "DSDUP snapchk" > build3ds/out/dsdup_run2.txt 2>&1`.
4. `grep -a "snapchk f\|lsnap f\|scomp rec f" build3ds/azt/logs/entcape_kmr03_snapchk.txt > build3ds/out/dsdup_head.txt`. `grep -a "snapchk f\|lsnap f" build3ds/basecache/$(git merge-base HEAD main)/entcape_kmr03_snapchk/run.log > build3ds/out/dsdup_basel.txt`.
## Gate (exact)
Host PASS. The grep of edit 3 is 1. The rowloop exits 0 (byte-equal). dsdup_head.txt holds a `scomp rec f` line. The last `snapchk f` line and the last `lsnap f` line of dsdup_head.txt equal those of dsdup_basel.txt from the word `snapchk` (or `lsnap`) to the line end.
## Fix rights and stop
Fix rights: nd_scomp.c, compile errors only. Stop: a DIFF, a snapchk or lsnap value that differs, no `scomp rec` line, a crash, a tool fault. A differing value is a fact about the core 1 state, not a failure to fix: commit the outputs (`git add -f`), send the paths to the oracle of the launch prompt with SendMessage, wait. Pitfalls: one host test at a time. One bench per rowloop call. PM_CACHE=off only as a prefix of one command.
Hand-back: last commit `DSDUP 2: gate. entcape drec equal; snapchk and lsnap equal base` deletes this page. One SubagentHandback: commit hash, the last snapchk and lsnap lines of head and base.
