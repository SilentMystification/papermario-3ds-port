# Row CODEMODFX4: codemod run on src/effects/lens_flare.c (10 value calls to native records)

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. Recipe of CODEMODFX1, merged 177ac50be.)
## Facts (ORACLE8, main 182ac5091)
- 3ds/tools/codemod.py rewrites the value calls (gSPMatrix -> nd_xf_push, gSPPopMatrix -> nd_xf_pop, gDPPipeSync removed,
  gDPSetPrimColor -> nd_val_prim, gDPSetEnvColor -> nd_val_env, gSPSegment -> nd_val_seg, gDPSetTileSize -> nd_val_tile_size).
  It keeps nd_rec_dl and gSPDisplayList (CONV2F is not merged) and the state calls. Each record runs the walker's own code.
- `python 3ds/tools/codemod.py --dry src/effects/lens_flare.c` on main: `value 10 keep 0 state 0 dl 3`.
- The file has no TARGET_3DS line. Its includes end with `#include "nd_gtab.h"` (line 3). The tool adds no #include.
- Scene: fxpic_21 (fxtest 21 = lens_flare, pm_fxtest.c FXLIST), kind azahar_pic, shots 412 to 415, in bench_defs.json.
- Seed source: lens_flare.c:158 `gDPSetPrimColor(gMainGfxPos++, 0, 0, 255, 255, 255, alpha)`. After edit 1 it is an nd_val_prim call with the same arguments.
Start: `sh 3ds/tools/wt_new.sh CODEMODFX4`. PM_ROW=CODEMODFX4 PM_LANE=CODEMODFX4 on every call.
## Touch only (places)
src/effects/lens_flare.c only (the 10 value call lines and 2 include lines). No other file.
## Exact edits
1. `python 3ds/tools/codemod.py src/effects/lens_flare.c` (rewrites in place).
2. After `#include "nd_gtab.h"` add `#include "nd_val.h"` and `#include "nd_xf.h"`.
3. Commit: `CODEMODFX4: codemod run on lens_flare.c (10 value calls to native records)`.
## Gate (expected values)
1. `python 3ds/tools/codemod.py --dry src/effects/lens_flare.c` after the edit: `value 0 keep 0 state 0 dl 3`.
2. `python 3ds/tools/listcen.py --group src/effects` before edit 1 and after: the build count of lens_flare.c falls by 10. Hand back
   both numbers.
3. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row CODEMODFX4 --why "CODEMODFX4"`: PASS.
4. Byte compare: `bench.py base fxpic_21` at the merge-base, then `bench.py run fxpic_21 --row CODEMODFX4 --why "CODEMODFX4 byte gate"` and
   `bench.py cmp fxpic_21 --row CODEMODFX4`: every shot byte-equal.
5. Seeded fault first (a value change, then undo): in the nd_val_prim call of the seed source change its third argument 255 to 254. cmp fxpic_21
   must show a difference. If it shows none: stop and hand back.
6. Budget 8 Azahar starts. Counter: C1 -10, C2 -10.
## Stop and end
A cmp difference with the real edit, a seed with no difference, a base pair that differs (BASEKEEP line: tool fault), or a build
error: stop, commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
