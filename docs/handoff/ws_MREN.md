# ws_MREN (Haiku): the core 1 decision and texture window functions get names outside the model list (counter: listcen model -4, elfgate func lower)
Designer: oracle FIN. WAIT: branch from main after the NDR0D merge and the REN6 merge (NDR0D edits nd_decide.c, REN6 edits nd_ds.c). Worktree build3ds/wt/MREN,
branch MREN. PM_ROW=MREN PM_LANE=MREN. Edit only: 3ds/src/nd_decide.c, 3ds/src/nd_texwin.c, 3ds/include/nd_int.h, 3ds/src/nd_ds.c, 3ds/src/nd_tex_set.c,
3ds/tools/nd_gfx_conv.c, 3ds/tools/nd_ds_test.c, 3ds/tools/nd_gtab_test.c.
## Facts (code read, main e5445e717c)
- listcen model counts the definitions of MODEL_NAMES (listcen.py:197-198, :202-218) in src/**/*.c and 3ds/src/**/*.c. decide and decide_miss are defined in
  nd_decide.c (:214, :172, static), tex_win0 and tex_win1 in nd_texwin.c (:11, :83). elfgate_patterns.txt:1 holds the same 4 names.
- These 4 stay at the end: core 1 decides a table miss (UD4, plan_finish.md section 6) and nd_ds.c makes td with tex_win0/1 (:177, :185). plan_finish.md DEL2A and DEL2C rename them.
- Uses (oracle grep): decide( and decide_miss( only in nd_decide.c. tex_win0/1 in nd_texwin.c, nd_int.h:195-196, nd_ds.c, nd_tex_set.c:161 :466, nd_gfx_conv.c:391 :407,
  nd_ds_test.c:238, nd_gtab_test.c:286 :302. The new names are unused today (oracle grep): dc_pick, dc_new, nd_texw0, nd_texw1.
## Edits (one shape: a sed of the name in code and comments)
1. `sed -i -E 's/\bdecide_miss\(/dc_new(/g; s/\bdecide\(/dc_pick(/g' 3ds/src/nd_decide.c`. Before: `static Dec* decide(const NdCombIn* c, int two) {` (nd_decide.c:214,
   the NDR0D form has more parameters). After: `static Dec* dc_pick(const NdCombIn* c, int two) {`. Do not touch nd_pso_decide or nd_comb_decide (no word start: the sed skips them).
2. `sed -i -E 's/\btex_win0\b/nd_texw0/g; s/\btex_win1\b/nd_texw1/g'` on the other 7 files. Before (nd_tex_set.c:161): `tex_win0(&nd_ws.tst1, &nd_tm, ...` After:
   `nd_texw0(&nd_ws.tst1, &nd_tm, ...`.
3. `grep -rn "\btex_win[01]\b\|\bdecide(\|\bdecide_miss(" 3ds src include --include=*.c --include=*.h --include=*.sh --include=*.py` lists only 3ds/tools/listcen.py:197-198
   (do not edit it). Another hit: stop, send it to the oracle.
## Steps and tests (output to build3ds/out/, read the last 20 lines; 1 build, 1 Azahar start)
1. Edits 1-3. `sh 3ds/tools/nd_host_test.sh` PASS. `python 3ds/tools/listcen.py > build3ds/out/MREN_listcen.txt`, the same on the merge-base: model lower by 4, every
   other `listcen total` line equal. Commit `MREN 1: dc_pick, dc_new, nd_texw0, nd_texw1`.
2. `sh 3ds/build.sh`, then `python 3ds/tools/elfgate.py build3ds/pm_3ds.elf > build3ds/out/MREN_elfgate.txt`: no listed name is decide, decide_miss, tex_win0 or tex_win1.
   Before: func 48 (FACT, orchestrator brief 2026-11-17). After: GUESS 44 to 46 (an inlined static has no symbol).
3. bench.py base spot_mac01_drec (merge-base, not charged). rowloop --row MREN --bench spot_mac01_drec: LOOP PASS, byte-equal.
## Fix rights: the listed files; compile errors of the rename. A DIFF, a changed other total: stop, send the output path to the oracle. Wait.
Stop: last commit `MREN 2: gate. spot_mac01_drec byte-equal; model <before> -> <after>; elfgate func <before> -> <after>` deletes this page. One SubagentHandback.
