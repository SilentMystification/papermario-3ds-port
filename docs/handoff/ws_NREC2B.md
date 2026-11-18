# ws_NREC2B (Haiku): every core 0 replay of a combine decision writes sl_const, sl_env and sl_cc. This fixes the NREC2 snapchk fault (fxops bad 469, name "sl"). Counter: none of C1-C12. Gate: fxops_snapchk bad 0, entcape_kmr03_drec byte-equal.
Designer: oracle NREC. Start from main (bfe522f1fa or later). No WAIT: no other open page edits these files. Worktree build3ds/wt/NREC2B, branch NREC2B. PM_ROW=NREC2B PM_LANE=NREC2B.
Edit only (Edit tool, keep CRLF): 3ds/src/nd_comb.c (scm_replay), 3ds/src/nd_direct.c (line 28, nd_direct_bind), 3ds/tools/nd_direct_test.c (line 29). New file 3ds/tests/host.d/nrec2b.sh (LF, as hwfix1.sh).
## Facts (code read, main bfe522f1fa)
- FACT nd_snap.c:174 (sc_compare): `if (sl_const != o->cc_on || (o->cc_on && sl_cc != o->cc)) sc_add("sl");`. The compare runs before nd_snap_comb (nd_snap.c:226-229).
- FACT the writers of sl_*: nd_comb_apply (nd_comb.c:313-314), sc_replay (nd_scomp.c:210), s6_put (nd_scomp.c:305, debug). scm_replay (nd_comb.c:181, the memo hit of sync_combine, nd_comb.c:351) and the decision block of nd_direct_bind (nd_direct.c:66-80) set the GPU constant but no sl_*.
- FACT base fxops_snapchk (4d10b0ebb5) f500: bad 469 fbad 469. The 8 logged lines name only "sl". No "gconst" line: the GPU state is right, only sl_* is stale.
- FACT the readers of sl_*: nd_snap.c:174 (snapchk), nd_comb.c:563 (combchk), nd_comb.c:294 (out->cc when cc_on is 0, never sent to the GPU), nd_scomp.c:297 (debug). So the edit changes no draw: drec stays byte-equal.
## Edits
1. nd_comb.c scm_replay. Before: `    if (m->cc_on) pm_gpu_set_const(m->cc);` (first line of the body). After: insert above it
   `    sl_const = m->cc_on; sl_env = m->env_on; if (m->cc_on) sl_cc = m->cc; /* NREC2B: a memo hit leaves sl_* as nd_comb_apply does (snapchk sl) */`
2. nd_direct.c line 28. Before: `extern int ls_use, ls_why, ls_cyc, ls_dual, ls_frame; /* nd_comb.c: ... */`. After: append ` extern int sl_const, sl_env; extern unsigned sl_cc; /* NREC2B */` to the end of the line.
3. nd_direct.c nd_direct_bind. Before: `        ls_use = o->use; ls_why = o->why; ls_cyc = o->cyc; ls_dual = o->dual; ls_frame = rm_frame;`. After: below it insert
   `        sl_const = o->cc_on; sl_env = o->env_on; if (o->cc_on) sl_cc = o->cc; /* NREC2B: as nd_comb_apply */`
4. nd_direct_test.c line 29. Before: `int ls_use, ls_why, ls_cyc, ls_dual, ls_frame;`. After: append ` int sl_const, sl_env; unsigned sl_cc; /* NREC2B */`.
5. New 3ds/tests/host.d/nrec2b.sh (form of hwfix1.sh). L1 = the line of edit 1, L2 = the line of edit 3 (exact text, grep -cF). Check A: L1 count 1 in nd_comb.c. Check B: L2 count 1 in nd_direct.c. Seed: `sed 's/sl_const = m->cc_on; //' 3ds/src/nd_comb.c > /tmp/nrec2b/seed/nd_comb.c`. The seed must differ from the file and fail check A. Print `nrec2b ok 2`. Any failure prints `nrec2b FAIL <A|B|seed>` and exits 1.
## Steps (one command per step, output to build3ds/out/, read the last 20 lines; Bash `timeout: 600000`; 1 lane build; at most 2 Azahar starts of 8)
1. Edits 1-5. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nrec2b_host.txt 2>&1`: PASS and `nrec2b ok 2`. Commit `NREC2B 1: scm_replay and nd_direct_bind write sl_*`.
2. `python 3ds/tools/bench.py base entcape_kmr03_drec --commit $(git merge-base HEAD main) > build3ds/out/nrec2b_base.txt 2>&1`. Then `sh 3ds/tools/rowloop.sh --row NREC2B --bench entcape_kmr03_drec --no-host --why "NREC2B sl" > build3ds/out/nrec2b_rowloop.txt 2>&1` (it builds).
3. `python 3ds/tools/bench.py base fxops_snapchk --commit $(git merge-base HEAD main) > build3ds/out/nrec2b_snapbase.txt 2>&1`. Then grep `snapchk f500\|snapchk bad` from the run.log of that base entry (path in the output) to build3ds/out/nrec2b_snapbase_lines.txt. This is the seeded-fault proof: base bad above 0 (GUESS 469).
4. `PM_ROW=NREC2B PM_LANE=NREC2B python 3ds/tools/bench.py run fxops_snapchk --row NREC2B --why "NREC2B sl" > build3ds/out/nrec2b_run.txt 2>&1`. Then `grep -a "snapchk f\|snapchk bad\|lsnap f\|ndc: stream" build3ds/azt/logs/fxops_snapchk.txt > build3ds/out/nrec2b_lines.txt`.
Gate (exact): host PASS with `nrec2b ok 2`. entcape_kmr03_drec byte-equal (rowloop exit 0). Base f500 bad above 0. Head last snapchk line (f500): bad 0. Head last lsnap line: tmis 0.
Stop: a DIFF, head bad above 0 (send the bad lines), base bad 0, a crash, a fatal line, a tool fault. Commit the outputs (`git add -f`), send the paths to oracle NREC (the agent that started you) with SendMessage, wait. Fix rights: the files of this page, compile errors only. Pitfalls: one host test at a time. One bench per rowloop call.
Hand-back: last commit `NREC2B 2: gate. entcape drec equal; fxops snapchk bad 0 (base <n>)` deletes this page. One SubagentHandback: commit hash, the base and head snapchk f500 lines, the lsnap f500 line.
