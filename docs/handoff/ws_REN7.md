# ws_REN7 (Haiku): the record runners name the unit ids ND_FS_ in place of NDO_ (counter: listcen reader lower)
Designer: oracle FIN. Branch from main. Worktree build3ds/wt/REN7, branch REN7. PM_ROW=REN7 PM_LANE=REN7.
Edit only: 3ds/src/nd_recs.c, 3ds/src/nd_model.c, 3ds/src/nd_scomp.c, 3ds/src/nd_ent.c. Running rows (NDR0D, NDR5, REN5, REN6, NST2G) edit none of them.
## Facts (code read, main 796ae535ae)
- nd_fs.h (row NCONV1A) defines every NDO_ name these 4 files use as ND_FS_ with the same value (oracle grep: no name missing). No `NDO_##` paste, no NDO_ string.
- listcen reader counts `\bNDO_[A-Z0-9_]+` in 3ds/src/*.c and 3ds/include/*.h, comments stripped (listcen.py:52). ND_FS_ names count as core1 (:54).
- Code tokens (oracle count, comments stripped): nd_recs.c 45, nd_model.c 32, nd_scomp.c 44, nd_ent.c 19. Sum 140.
- No host seed script seds a line of these 4 files (oracle grep of 3ds/tests). host.d geounit.sh, hunit.sh, nmdl0.sh, ns3unit.sh compile nd_model.c with -I 3ds/include.
## Edit (one shape for every file)
1. Replace every token `NDO_<NAME>` by `ND_FS_<NAME>` in the 4 files, in code and comments: `sed -i -E 's/\bNDO_/ND_FS_/g' <files>`.
   Before (nd_ent.c:142): `if (op == NDO_END) return 1;`  After: `if (op == ND_FS_END) return 1;`
2. Add `#include "nd_fs.h"` after the last #include of each file. Keep every existing #include (other names reach nd_op.h through them).
3. A name that nd_fs.h does not define is a compile error: stop, send the error to the oracle (do not add names to nd_fs.h).
## Steps and tests (output to build3ds/out/, read the last 20 lines; 1 build, 1 Azahar start)
1. Edits 1-2. `sh 3ds/tools/nd_host_test.sh` PASS. `python 3ds/tools/listcen.py > build3ds/out/REN7_listcen.txt`, the same on the merge-base: reader lower by 140
   (FACT: the oracle count above), core1 higher by 140 (GUESS: the same file scope), every other `listcen total` line equal. Commit `REN7 1: ND_FS_ names in 4 record runner files`.
2. bench.py base spot_mac01_drec (merge-base, not charged). rowloop --row REN7 --bench spot_mac01_drec: LOOP PASS, byte-equal.
## Fix rights: the 4 files; compile errors of the rename. A DIFF, a changed other total, a missing name: stop, send the output path to the oracle. Wait.
Stop: last commit `REN7 2: gate. spot_mac01_drec byte-equal; reader <before> -> <after>` deletes this page. One SubagentHandback.
