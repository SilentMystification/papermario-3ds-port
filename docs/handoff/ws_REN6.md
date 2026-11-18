# ws_REN6 (Haiku): the core 1 state and the game unit helpers name the unit ids ND_FS_ in place of NDO_ (counter: listcen reader lower)
Designer: oracle FIN. Branch from main. Worktree build3ds/wt/REN6, branch REN6. PM_ROW=REN6 PM_LANE=REN6.
Edit only: 3ds/src/nd_ds.c, 3ds/include/nd_ds.h, 3ds/src/nd_seam.c, 3ds/include/nd_vcopy.h, 3ds/include/nd_vcount.h, 3ds/include/nd_lkey.h, 3ds/include/nd_mesh.h,
3ds/include/nd_imgfx.h. Running rows (NDR0D, NDR5, REN5, NST2G) edit none of them.
## Facts (code read, main f45c77160f)
- nd_fs.h (row NCONV1A) defines every NDO_ name these 8 files use as ND_FS_ with the same value (oracle grep: no name missing). nd_fs_test.c asserts the values.
- listcen reader counts `\bNDO_[A-Z0-9_]+` in 3ds/src/*.c and 3ds/include/*.h, comments stripped (listcen.py:52). ND_FS_ names count as core1 (:54).
- Code tokens (oracle count, comments stripped): nd_ds.c 45, nd_seam.c 9, nd_vcopy.h 3, nd_vcount.h 2, nd_lkey.h 2, nd_mesh.h 2, nd_imgfx.h 2, nd_ds.h 0. Sum 65.
- The host seed scripts lkey.sh:5, meshst.sh:5, pipefx.sh:5, vcopy.sh:11, vcount.sh:5 sed other text of these headers (no NDO_ token): no script edit.
## Edit (one shape for every file)
1. Replace every token `NDO_<NAME>` by `ND_FS_<NAME>` in the 8 files, in code and comments: `sed -i -E 's/\bNDO_/ND_FS_/g' <files>`.
   Before (nd_vcount.h:10): `if ((w0 >> 24) == NDO_END) break;`  After: `if ((w0 >> 24) == ND_FS_END) break;`
2. Add `#include "nd_fs.h"` after the last #include of each file. Keep every existing #include (other files reach nd_op.h through them).
3. A name that nd_fs.h does not define is a compile error: stop, send the error to the oracle (do not add names to nd_fs.h).
## Steps and tests (output to build3ds/out/, read the last 20 lines; 1 build, 1 Azahar start)
1. Edits 1-2. `sh 3ds/tools/nd_host_test.sh` PASS. `python 3ds/tools/listcen.py > build3ds/out/REN6_listcen.txt`, the same on the merge-base: reader lower by 65
   (FACT: the oracle count above), core1 higher by 65 (GUESS: the same file scope), every other `listcen total` line equal. Commit `REN6 1: ND_FS_ names in nd_ds.c, nd_seam.c and 6 headers`.
2. bench.py base spot_mac01_drec (merge-base, not charged). rowloop --row REN6 --bench spot_mac01_drec: LOOP PASS, byte-equal.
## Fix rights: the 8 files; compile errors of the rename. A DIFF, a changed other total, a missing name: stop, send the output path to the oracle. Wait.
Stop: last commit `REN6 2: gate. spot_mac01_drec byte-equal; reader <before> -> <after>` deletes this page. One SubagentHandback.
