# ws_DELLC (Haiku): the lstcen debug tool goes (user D2 A: the walk census goes). Counters: C11 elf_func -1 (nd_lstcen, elfgate_patterns.txt:1), C7 gfxtype 116 -> 113 (GUESS), C8 reader 387 -> 386 (GUESS).
Designer: oracle DEL (coach). Start commit main e3d2f6523b. No WAIT: NCONV2D edits nd_walk.c after nd_nr_run and nd_ntab_run only; this row edits nd_walk.c 176-270. Worktree build3ds/wt/DELLC, branch DELLC. PM_ROW=DELLC PM_LANE=DELLC.
Edit only (Edit tool, keep CRLF): 3ds/src/nd_walk.c, 3ds/src/nd_rec.c (line 619), 3ds/src/nd_recs.c (lines 45, 56), 3ds/include/nd.h (line 554), 3ds/tools/nd_at_test.c, nd_geo_test.c, nd_st_test.c, nd_tx_test.c, nd_val_test.c (2 stub lines each), 3ds/tools/azt_scenarios.json, 3ds/tools/bench_defs.json (the 2 lstcen entries).
## Facts (code read, main e3d2f6523b; line numbers of that commit)
- FACT nd_lstcen (nd_walk.c:225-248) runs only with the switch lstcen=<frame> (lc_on, :221-224). Its callers: nd_walk.c:176, :194, :270; nd_rec.c:619; nd_recs.c:45, :56. Each caller tests `nd_lc_frame != -1` first.
- FACT the host stubs `void nd_lstcen(...)` and `int nd_lc_frame = -1;` are at nd_at_test.c:42-43, nd_geo_test.c:42-43, nd_st_test.c:41-42, nd_tx_test.c:43-44, nd_val_test.c:42-43.
- FACT the only users of the lstcen lines are the scenarios lstcen_mac00_det (azt_scenarios.json:14211-14234) and lstcen_btl_det (:14235-14258) and their benches (bench_defs.json:8423-8438, :8439-8454). Each object ends with `},` and the next line is `{`.
## Edits
1. nd_walk.c: delete lines 216-248 (the lstcen comment, nd_lc_frame, lc_tab, lc_on, nd_lstcen). Delete line 176 (`if (__builtin_expect(nd_lc_frame != -1, 0)) nd_lstcen(q, 3u);`), line 194 (`if (!drawn && ... nd_lstcen(q, 4u);`) and line 270 (`if (__builtin_expect(nd_lc_frame != -1, 0) && !nt) nd_lstcen(next, ex_depth ? 2u : 1u);`). Delete the bottom line first, so the upper line numbers hold.
2. nd_rec.c: delete line 619 (`if (__builtin_expect(nd_lc_frame != -1, 0)) nd_lstcen(l, 5u);`). nd_recs.c: delete lines 56 and 45 (the 2 nd_lstcen lines). nd.h: delete line 554 (`extern int nd_lc_frame; void nd_lstcen(...)`).
3. The 5 host tests: delete the 2 stub lines each (Facts).
4. azt_scenarios.json: delete lines 14211-14258. bench_defs.json: delete lines 8423-8454. Then `python -c "import json; json.load(open('3ds/tools/azt_scenarios.json')); json.load(open('3ds/tools/bench_defs.json')); print('json ok')"`: `json ok`.
5. `grep -rn "lstcen\|nd_lc_frame" 3ds/src 3ds/include 3ds/tools/*.c 3ds/tools/*.json src include > build3ds/out/dellc_grep.txt`: no line except comment lines that only name the old tool. Delete such a comment phrase when it is a full comment line about lstcen.
## Steps (one command per step, output to build3ds/out/, read the last 20 lines; Bash `timeout: 600000`; 1 build, at most 2 Azahar starts of 8)
1. Edits 1-5. `sh 3ds/tools/nd_host_test.sh > build3ds/out/dellc_host.txt 2>&1`: PASS. Commit `DELLC 1: lstcen debug tool deleted`.
2. `python 3ds/tools/bench.py base spot_mac01_drec --commit $(git merge-base HEAD main)`. `sh 3ds/tools/rowloop.sh --row DELLC --bench spot_mac01_drec --no-host --why "DELLC lstcen delete"` (it builds).
3. `python 3ds/tools/listcen.py > build3ds/out/dellc_listcen.txt`. `python 3ds/tools/elfgate.py build3ds/pm_3ds.elf > build3ds/out/dellc_elfgate.txt 2>&1`. `python -c "print(open('build3ds/pm_3ds.elf','rb').read().count(b'nd_lstcen'))"`: 0.
## Gate (exact)
Host PASS. Build rc 0. LOOP PASS (byte-equal). json ok. listcen gfxtype and reader not higher than 116 and 387, other totals equal. The ELF holds no `nd_lstcen` string (count 0). Report the elfgate func line (data).
## Fix rights
The listed files. A compile error for a deleted name: delete that use and name it in the commit. A DIFF, a crash, a total that rises: stop. Commit the outputs (`git add -f`), send the paths to the oracle of the launch prompt with SendMessage, wait.
## Hand-back
Last commit `DELLC 2: gate. spot_mac01 drec equal; elf func <a> -> <b>, gfxtype 116 -> <n>` deletes this page. One SubagentHandback: commit hash, listcen totals, the elfgate line.
