# ws_DELT4 (Haiku): the t4chk debug check goes (user D2 A: the walk census goes). Counters: C8 reader 386 -> 385 (GUESS: the NDO_DROP arm of nd_walk_state); listcen core1 -3 (GUESS: 3 ND_FS_DROP uses; not in the end proof).
Designer: oracle DEL (coach). Start commit main 3cc4803302 (DELLC merged). No WAIT: DEL1B adds lines after nd_ops_run (nd_walk.c 419), DEL5B1 edits nd_walk.c in nd_parts_run; this row edits nd_walk.c 293-306, 371, 388. Worktree build3ds/wt/DELT4, branch DELT4. PM_ROW=DELT4 PM_LANE=DELT4.
Edit only (Edit tool, keep CRLF): 3ds/src/nd_walk.c, 3ds/src/nd_rec.c (141, 414, 442-446), 3ds/src/nd_model.c (290), 3ds/tools/azt_scenarios.json, 3ds/tools/bench_defs.json (the hc4_t4chk_mac01 entries), 3ds/tests/lkey/allow.txt (count only).
## Facts (code read, main 3cc4803302; line numbers of that commit)
- FACT the recorder writes ND_FS_DROP only when the switch t4chk is on (nd_rec.c:442-446: `if (!t4_sw) return;`). Without it no DROP unit exists, so the DROP arms (nd_walk.c:371, :388) and the model skip (nd_model.c:290) never run.
- FACT the enum ids NDO_DROP (nd_op.h:28) and ND_FS_DROP (nd_fs.h:25) stay: removing an id moves every later id (gconv tables, host words).
- FACT the only user of the t4chk lines is the scenario hc4_t4chk_mac01 (azt_scenarios.json:1171-1191) and its bench (bench_defs.json:3926-3940). Each object ends with `},` and the next line is `{`.
## Edits (bottom line first in each file, so the upper line numbers hold)
1. nd_walk.c:388. Before: `        else if (__builtin_expect(id == ND_FS_DROP, 0)) { q += 2; if (q < e) t4_check(dbg, q); } else if (id == ND_FS_END) break; else op_typed(dbg, q, q);` After: `        else if (id == ND_FS_END) break; else op_typed(dbg, q, q);`
2. nd_walk.c:371. Before: `        } else if (__builtin_expect(id == NDO_DROP, 0)) { q += 2; if (q < e) t4_check(dbg, q); } /* HC4b t4chk: an op the recorder dropped */` After: `        }`
3. nd_walk.c: delete 293-306 (the t4chk comment, the t4_ counters, t4_check).
4. nd_rec.c:442-446. Before: the 5 lines from `    if (sh_drop(t)) {` to its `    }`. After: `    if (sh_drop(t)) return; /* HC4b: a typed op the shadow drops is not written */`. Line 414: delete ` (t4chk: written after ND_FS_DROP)` (keep the `.` and ` */`). Delete line 141 (`static int t4_sw = -1; ...`).
5. nd_model.c: delete line 290 (`if ((p[2 + 2 * k] >> 24) == ND_FS_DROP) { k++; i--; continue; } ...`).
6. bench_defs.json: delete lines 3926-3940. azt_scenarios.json: delete lines 1171-1191. Then `python -c "import json; json.load(open('3ds/tools/azt_scenarios.json')); json.load(open('3ds/tools/bench_defs.json')); print('json ok')"`: `json ok`.
7. `grep -rn "t4_check\|t4_sw\|t4chk" 3ds/src 3ds/include 3ds/tools/*.c 3ds/tools/bench_defs.json > build3ds/out/delt4_grep.txt`: only the comment of nd_op.h:4 and nd_op.h:28 and the reason text of azt_scenarios.json that names hc4_t4chk_mac01.
## Steps (one command per step, output to build3ds/out/, read the last 20 lines; Bash `timeout: 600000`; 1 build, at most 2 Azahar starts of 8)
1. Edits 1-7. `sh 3ds/tools/nd_host_test.sh > build3ds/out/delt4_host.txt 2>&1`: PASS. Commit `DELT4 1: t4chk debug check deleted`.
2. `python 3ds/tools/bench.py base spot_mac01_drec --commit $(git merge-base HEAD main)`. `sh 3ds/tools/rowloop.sh --row DELT4 --bench spot_mac01_drec --no-host --why "DELT4 t4chk delete"` (it builds).
3. `python 3ds/tools/listcen.py > build3ds/out/delt4_listcen.txt`.
## Gate (exact)
Host PASS. Build rc 0. LOOP PASS (byte-equal). json ok. listcen reader not higher than 386; build, data, gfxtype, model equal to the start commit.
## Fix rights
The listed files. `lkey census bad <file> <n> listed <m>` (host.d/lcen.sh): set the count of that file in 3ds/tests/lkey/allow.txt to n, note it in the commit. The 3 lines `CACHECHECK FAIL shots main_bench` do not fail the suite. A compile error for a deleted name: delete that use, name it in the commit. A DIFF, a crash, a total that rises: stop. Commit the outputs (`git add -f`), send the paths to the oracle of the launch prompt with SendMessage, wait.
## Hand-back
Last commit `DELT4 2: gate. spot_mac01 drec equal; reader 386 -> <n>` deletes this page. One SubagentHandback: commit hash, listcen totals.
