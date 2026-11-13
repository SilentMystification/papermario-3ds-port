# Row CODEMODFX7: codemod run on src/effects/big_smoke_puff.c (9 value calls to native records)

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. Recipe of CODEMODFX1, merged 177ac50be.)
## Facts (ORACLE8, main 0250d5221)
- 3ds/tools/codemod.py rewrites the value calls (gSPMatrix -> nd_xf_push, gSPPopMatrix -> nd_xf_pop, gDPPipeSync removed,
  gDPSetPrimColor -> nd_val_prim, gDPSetEnvColor -> nd_val_env, gSPSegment -> nd_val_seg, gDPSetTileSize -> nd_val_tile_size).
  It keeps nd_rec_dl and gSPDisplayList (CONV2F is not merged) and the state calls. Each record runs the walker's own code.
- `python 3ds/tools/codemod.py --dry src/effects/big_smoke_puff.c` on main: `value 9 keep 0 state 0 dl 1`.
- The file has no TARGET_3DS line. Its last include is `#include "nd_gtab.h"`. The tool adds no #include.
- Scene: fxpic_0 (fxtest 0 = big_smoke_puff, pm_fxtest.c FXLIST), kind azahar_pic, shots 412 to 415, in bench_defs.json.
- Seed source: big_smoke_puff.c:169 `gDPSetEnvColor(gMainGfxPos++, 0, 0, 0, envAlpha)`. After edit 1 it is the native call with the same arguments.
Start: `sh 3ds/tools/wt_new.sh CODEMODFX7`. PM_ROW=CODEMODFX7 PM_LANE=CODEMODFX7 on every call.
## Touch only (places)
src/effects/big_smoke_puff.c only (the 9 value call lines and 2 include lines). No other file.
## Exact edits
1. `python 3ds/tools/codemod.py src/effects/big_smoke_puff.c` (rewrites in place).
2. After `#include "nd_gtab.h"` add `#include "nd_val.h"` and `#include "nd_xf.h"` (skip a line that is already there).
3. Commit: `CODEMODFX7: codemod run on big_smoke_puff.c (9 value calls to native records)`.
## Gate (expected values)
1. `python 3ds/tools/codemod.py --dry src/effects/big_smoke_puff.c` after the edit: `value 0 keep 0 state 0 dl 1`.
2. `python 3ds/tools/listcen.py --group src/effects` before edit 1 and after: the build count of big_smoke_puff.c falls by 9. Hand back
   both numbers.
3. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row CODEMODFX7 --why "CODEMODFX7"`: PASS.
4. Byte compare: `bench.py base fxpic_0` at the merge-base, then `bench.py run fxpic_0 --row CODEMODFX7 --why "CODEMODFX7 byte gate"` and
   `bench.py cmp fxpic_0 --row CODEMODFX7`: every shot byte-equal.
5. Seeded fault first (a value change, then undo): in its nd_val_env call, the first argument 0 to 64. cmp fxpic_0 must show a difference. If it shows none: stop and
   hand back.
6. Budget 8 Azahar starts. Counter: C1 -9, C2 -9.
## Stop and end
A cmp difference with the real edit, a seed with no difference, a base pair that differs (BASEKEEP line: tool fault), or a build
error: stop, commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
