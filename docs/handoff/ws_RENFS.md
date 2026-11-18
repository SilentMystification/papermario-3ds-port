# ws_RENFS (Haiku, 5 rows REN1-REN5): the setter files name the unit ids ND_FS_ in place of NDO_ (counter: listcen reader lower)
Designer: oracle FIN. One coder per row. Worktree build3ds/wt/<row>, branch <row>. PM_ROW=<row> PM_LANE=<row>.
| row | edit only | WAIT: branch from | reader GUESS (grep incl. comments) |
|---|---|---|---|
| REN1 | 3ds/src/nd_st.c, 3ds/src/nd_val.c, 3ds/src/nd_xf.c | NCONV1A gate commit | about -8 |
| REN2 | 3ds/include/nd_tx.h | NCONV1A gate commit | about -40 |
| REN3 | 3ds/include/nd_at.h | NCONV1A gate commit | about -21 |
| REN4 | 3ds/include/nd_nr.h | NCONV1A gate commit | about -16 |
| REN5 | 3ds/src/nd_geo.c | NCONV1A and NDR0C merged (both edit or need it) | about -9 |
## Facts (code read, main 59a0ffc265)
- nd_fs.h (row NCONV1A) gives every NDO_ id, sub id and macro as ND_FS_ with the same value, proven by _Static_assert in nd_fs_test.c.
- listcen reader counts `\bNDO_[A-Z0-9_]+` in 3ds/src/*.c and 3ds/include/*.h, comments stripped (listcen.py:52, :300-301). ND_FS_ names count as core1.
- nd_tx.h and nd_at.h are macro files that game code includes (77 files include nd_nr.h, nd_tx.h or nd_at.h). The values do not change, so the ELF does not change.
## Edit (one shape for every file of the row)
1. Replace every token `NDO_<NAME>` by `ND_FS_<NAME>` in the row's files, in code and comments: `sed -i -E 's/\bNDO_/ND_FS_/g' <files>`.
   Before (nd_val.c:30): `val_rec(NDO_TILESIZE, ((uls & 0xfffu) << 12) | (ult & 0xfffu), ...);`
   After: `val_rec(ND_FS_TILESIZE, ((uls & 0xfffu) << 12) | (ult & 0xfffu), ...);`
2. Add `#include "nd_fs.h"` after the last #include of each file. Keep every existing #include (other files reach nd_op.h through them).
3. Host seed scripts sed an NDO_ token of a renamed file. Rename the token in the script too (fix right): REN3 3ds/tests/host.d/hunit.sh:10
   (`NDO_ENV, 0u` and `NDO_PRIM, 0u`), REN2 3ds/tests/host.d/ns3unit.sh:10 (`NDO_RECT_TEX |`). A seed that matches nothing makes the seed run pass and the script fail.
4. A name that nd_fs.h does not define is a compile error: stop, send the error to the oracle (do not add names to nd_fs.h).
## Steps and tests (output to build3ds/out/, read the last 20 lines; 1 build, 1 Azahar start)
1. Edits 1-3. `sh 3ds/tools/nd_host_test.sh` PASS. `python 3ds/tools/listcen.py > build3ds/out/<row>_listcen.txt`: reader lower than the merge-base by
   the token count of the row's files (code only), every other `listcen total` line equal. Commit `<row> 1: ND_FS_ names in <files>`.
2. bench.py base spot_mac01_drec (merge-base, not charged). rowloop --row <row> --bench spot_mac01_drec: LOOP PASS, byte-equal.
## Fix rights: the row's files; compile errors of the rename. A DIFF, a changed other total, a missing name: stop, send the output path to the oracle. Wait.
Stop: last commit `<row> 2: gate. spot_mac01_drec byte-equal; reader <before> -> <after>`. A row does not edit this page (5 branches would
conflict): the orchestrator deletes it after the last of the 5 merges. One SubagentHandback.
