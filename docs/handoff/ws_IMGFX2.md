# ws_IMGFX2 (Haiku): the word translator ND_X0, ND_X1 and its helpers leave nd_op.h for a host header (3ds/tools/nd_x.h); no device file translates a list word. Counter: C8 reader -11 (FACT: the READER_RX tokens of ND_X0, ND_X1 at nd_op.h:79-120 are G_SETOTHERMODE_H :79, _L :80, G_SETCOMBINE :94, G_VTX :101, G_TRI1 :102, G_TRI2 :103, G_MTX :104, G_POPMTX :105, G_DL :107, G_TEXRECT :109, :120; listcen.py:52). Gate: host PASS, 1 drec byte-equal, the 4 generator outputs byte-equal.
Designer: oracle IMGFX. WAIT: branch from main after the merges of DEL3C2 (nd_gbi_host.h holds ND_GX, ND_GS; nd_at.h NDCTL_ND has no ND_X0) and DEL3C3 (nd_op.h in ND_FS_ names). Worktree build3ds/wt/IMGFX2, branch IMGFX2. PM_ROW=IMGFX2 PM_LANE=IMGFX2. Before edit 1: `PM_LANE=IMGFX2 PM_KEY_ONLY=1 sh 3ds/build.sh > build3ds/out/imgfx2_key0.txt 2>&1`, then copy 3ds/src/nd_gfx_tab.c, nd_ntab_descs.c, nd_pso_tab.c, nd_imgfx_tab.c to build3ds/out/imgfx2_gen0/. FACT the gfx table converter TUs reach ND_GS through nd_gbi.h -> nd_gbi_host.h (DEL3C2), so they need nd_x.h too: edit 3 gives it.
Edit only: new 3ds/tools/nd_x.h; 3ds/include/nd_op.h; 3ds/tools/nd_shape_conv.c, nd_seam_test.c, nd_dlc_test.c, nd_cmd_test.c; 3ds/tools/gbi_inc/nd_gbi_host.h; 3ds/tests/lkey/allow.txt (count only).
## Facts (code read, main 58969bb804; find the quoted text, the lines move after DEL3C3)
- FACT the users of ND_X0, ND_X1 and ND_OPB outside nd_op.h after the 3 WAIT rows: nd_shape_conv.c:155-158 (the shape tool, SH_CMD build.sh:103), nd_seam_test.c:128, nd_dlc_test.c:24, :178, nd_cmd_test.c:152-206, nd_gbi_host.h (ND_GX, ND_GS; old nd_gbi.h:18-19). Comments only: nd_nr.h:2-3, src/model.c:3638.
- FACT ND_NOP_A (nd_op.h:77) has device users (nd_dlc.c, nd_ds.c, nd_geom.c, nd_rec.c, nd_scomp.c, nd_st.c, nd_walk.c): it stays in nd_op.h.
- FACT inc_scan.py:36-37 scans the folder of the including file for a quoted include, so the shape key holds 3ds/tools/nd_x.h; the shape set runs once again (SH_MISS).
## Edits
1. New 3ds/tools/nd_x.h: guard ND_X_H; a 2-line comment (IMGFX2: the F3DEX2 word to typed unit translator of the host tools and tests; no device file includes it); `#include <stdint.h>`, `#include "ultra64.h"`, `#include "nd_op.h"`; then the nd_op.h lines from `/* HC4e1: the typed unit of an F3DEX2 word` to the last line of ND_X1 (`    (uint32_t)(w1))`), moved as they are, without the `#define ND_NOP_A` line.
2. nd_op.h: delete the moved lines; `#define ND_NOP_A ...` stays at its place. Header comment: the word translator (ND_X0, ND_X1) is 3ds/tools/nd_x.h (IMGFX2, host only).
3. Before and after (nd_shape_conv.c:23; the same in nd_seam_test.c, nd_dlc_test.c, nd_cmd_test.c after their `#include "nd_op.h"`):
```c
#include "nd_op.h"
```
```c
#include "nd_op.h"
#include "nd_x.h" /* IMGFX2: ND_X0, ND_X1 */
```
   nd_gbi_host.h: after its `#include "nd_gbi.h"` add `#include "../nd_x.h" /* IMGFX2: ND_X0, ND_X1 of ND_GX, ND_GS */`.
4. `grep -rnE "ND_X[01] *\(|ND_OPB *\(|\"nd_x\.h\"" 3ds/src 3ds/include src include > build3ds/out/imgfx2_grep.txt`: no line.
## Steps (one command per step, output to build3ds/out/, read the last 20 lines; Bash `timeout: 600000`; 1 lane build; at most 4 Azahar starts of 8)
1. Edits 1-4. `sh 3ds/tools/nd_host_test.sh > build3ds/out/imgfx2_host.txt 2>&1`: PASS. Commit `IMGFX2 1: ND_X0, ND_X1 to the host header nd_x.h`.
2. `python 3ds/tools/bench.py base spot_mac01_drec --commit $(git merge-base HEAD main) > build3ds/out/imgfx2_base.txt 2>&1`. `sh 3ds/tools/rowloop.sh --row IMGFX2 --bench spot_mac01_drec --no-host --why "IMGFX2 translator to host" > build3ds/out/imgfx2_loop.txt 2>&1`.
3. `for f in nd_gfx_tab.c nd_ntab_descs.c nd_pso_tab.c nd_imgfx_tab.c; do cmp build3ds/out/imgfx2_gen0/$f 3ds/src/$f; done > build3ds/out/imgfx2_gencmp.txt 2>&1`. Seed: `cp build3ds/out/imgfx2_gen0/nd_gfx_tab.c build3ds/out/imgfx2_seed.c; printf x >> build3ds/out/imgfx2_seed.c; cmp build3ds/out/imgfx2_gen0/nd_gfx_tab.c build3ds/out/imgfx2_seed.c > /dev/null; echo "seed rc $?" > build3ds/out/imgfx2_genseed.txt`. `python 3ds/tools/listcen.py > build3ds/out/imgfx2_listcen.txt`.
## Gate (exact)
Host PASS. Rowloop exit 0 (byte-equal). imgfx2_gencmp.txt empty; imgfx2_genseed.txt `seed rc 1`. imgfx2_grep.txt empty. listcen reader 11 below the start commit; no `listcen file reader 3ds/include/nd_op.h` line.
## Fix rights and stop
Fix rights: the listed files; a host file that fails on a missing ND_X0, ND_X1 or ND_OPB: add the include line of edit 3 to it and name it in the commit. A device file that fails on them: stop and send it (do not move the macros back). `lkey census bad <file> <n> listed <m>`: set that count in allow.txt to n. Stop: a DIFF, a crash, a tool fault, a counter off the gate. Commit the outputs (`git add -f`), send the paths to the oracle of the launch prompt with SendMessage, wait. Pitfalls: one host test at a time; one bench per rowloop call; `sed -b` or the Edit tool only.
## Hand-back
Last commit `IMGFX2 2: gate. spot_mac01 drec equal; reader -11` deletes this page. One SubagentHandback: commit hash, listcen totals.
