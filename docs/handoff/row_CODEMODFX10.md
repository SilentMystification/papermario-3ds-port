# Row CODEMODFX10: codemod run on src/effects/shimmer_burst.c (9 value calls to native records)

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. Recipe of CODEMODFX1, merged 177ac50be.)
## Facts (ORACLE8, main 0250d5221)
- 3ds/tools/codemod.py rewrites the value calls (gSPMatrix -> nd_xf_push, gSPPopMatrix -> nd_xf_pop, gDPPipeSync removed,
  gDPSetPrimColor -> nd_val_prim, gDPSetEnvColor -> nd_val_env, gSPSegment -> nd_val_seg, gDPSetTileSize -> nd_val_tile_size).
  It keeps nd_rec_dl and gSPDisplayList (CONV2F is not merged) and the state calls. Each record runs the walker's own code.
- `python 3ds/tools/codemod.py --dry src/effects/shimmer_burst.c` on main: `value 9 keep 3 state 0 dl 2`. The 3 kept calls write into a local Gfx array (first argument is not gMainGfxPos++): they stay.
- The file has no TARGET_3DS line. Its last include is `#include "nd_api.h"`. The tool adds no #include.
- Scene: fxpic_50 (fxtest 50 = shimmer_burst, pm_fxtest.c FXLIST), kind azahar_pic, shots 412 to 415, in bench_defs.json.
- Seed source (ORACLE8, 2026-11-13, by the seed rule of row_TEMPLATE.md): shimmer_burst.c:224 `gSPMatrix(gMainGfxPos++, ..., G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW)`, no guard (the env call at :225 is dead for the gate: its unk_40 is not shown read by the combine). Resume after 6efe2bf94.
Start: `sh 3ds/tools/wt_new.sh CODEMODFX10`. PM_ROW=CODEMODFX10 PM_LANE=CODEMODFX10 on every call.
## Touch only (places)
src/effects/shimmer_burst.c only (the 9 value call lines and 2 include lines). No other file.
## Exact edits
1. `python 3ds/tools/codemod.py src/effects/shimmer_burst.c` (rewrites in place).
2. After `#include "nd_api.h"` add `#include "nd_val.h"` and `#include "nd_xf.h"` (skip a line that is already there).
3. Commit: `CODEMODFX10: codemod run on shimmer_burst.c (9 value calls to native records)`.
## Gate (expected values)
1. `python 3ds/tools/codemod.py --dry src/effects/shimmer_burst.c` after the edit: `value 0 keep 3 state 0 dl 2`.
2. `python 3ds/tools/listcen.py --group src/effects` before edit 1 and after: the build count of shimmer_burst.c falls by 9. Hand back
   both numbers.
3. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row CODEMODFX10 --why "CODEMODFX10"`: PASS.
4. Byte compare: `bench.py base fxpic_50` at the merge-base, then `bench.py run fxpic_50 --row CODEMODFX10 --why "CODEMODFX10 byte gate"` and
   `bench.py cmp fxpic_50 --row CODEMODFX10`: every shot byte-equal.
5. Seeded fault first (a value change, then undo): in the nd_xf_push call of :224, `G_MTX_LOAD` -> `G_MTX_MUL`. The ELF sha1 must differ from the real edit. cmp fxpic_50 must
   show a difference. If it shows none: stop and hand back.
6. Budget 8 Azahar starts. Counter: C1 -9, C2 -9.
## Stop and end
A cmp difference with the real edit, a seed with no difference, a base pair that differs (BASEKEEP line: tool fault), or a build
error: stop, commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
