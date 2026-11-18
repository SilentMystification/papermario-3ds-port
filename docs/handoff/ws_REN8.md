# ws_REN8 (Haiku): the core 1 recorder names the unit ids ND_FS_ in place of NDO_ (counter: listcen reader lower)
Designer: oracle FIN. WAIT: branch from main after the NDR0E2 merge (order on nd_rec.c: NST2H, NST2I, NDR0E2, REN8). Worktree build3ds/wt/REN8, branch REN8. PM_ROW=REN8 PM_LANE=REN8.
Edit only: 3ds/src/nd_rec.c.
## Facts (code read, main f1c6cf10ef)
- nd_fs.h (row NCONV1A) defines every NDO_ name nd_rec.c uses as ND_FS_ with the same value (oracle grep: no name missing). No `NDO_##` paste, no NDO_ string.
- listcen reader counts `\bNDO_[A-Z0-9_]+` in 3ds/src/*.c and 3ds/include/*.h, comments stripped (listcen.py:52). ND_FS_ names count as core1 (:54).
- Code tokens of nd_rec.c (oracle count, comments stripped, main f1c6cf10ef): 73. GUESS: NDR5 adds none (its nd_rec_xf has no NDO_ token, ws_NDR5.md edit 1).
- stunit.sh:12 seds `p[0] = NDC_HDR(NDC_NAPI, 1, 0);` in nd_rec.c (no NDO_ token: the rename keeps it).
## Edit (one shape)
1. `sed -i -E 's/\bNDO_/ND_FS_/g' 3ds/src/nd_rec.c` (code and comments). Before (nd_rec.c:153): `if ((w0 >> 24) == NDO_END) break;` After: `if ((w0 >> 24) == ND_FS_END) break;`
2. Add `#include "nd_fs.h"` after the last #include of nd_rec.c. Keep every existing #include.
3. Run `grep -c 'p\[0\] = NDC_HDR(NDC_NAPI, 1, 0);' 3ds/src/nd_rec.c`: 1 expected. 0: stop, send it to the oracle. A name that nd_fs.h does not define: stop, send the error.
## Steps and tests (output to build3ds/out/, read the last 20 lines; 1 build, 1 Azahar start)
1. Edits 1-3. `sh 3ds/tools/nd_host_test.sh` PASS. `python 3ds/tools/listcen.py > build3ds/out/REN8_listcen.txt`, the same on the merge-base: reader lower by the
   nd_rec.c token count of the merge-base (GUESS 73), core1 higher by the same, every other `listcen total` line equal. Commit `REN8 1: ND_FS_ names in nd_rec.c`.
2. bench.py base spot_mac01_drec (merge-base, not charged). rowloop --row REN8 --bench spot_mac01_drec: LOOP PASS, byte-equal.
## Fix rights: nd_rec.c; compile errors of the rename. A DIFF, a changed other total, a missing name: stop, send the output path to the oracle. Wait.
Stop: last commit `REN8 2: gate. spot_mac01_drec byte-equal; reader <before> -> <after>` deletes this page. One SubagentHandback.
