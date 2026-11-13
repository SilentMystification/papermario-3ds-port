# Row CODEMODFX5: codemod run on src/effects/dust.c (6 value calls to native records)

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. Recipe of CODEMODFX1, merged 177ac50be.)
## Facts (ORACLE8, main 0250d5221)
- 3ds/tools/codemod.py rewrites the value calls (gSPMatrix -> nd_xf_push, gSPPopMatrix -> nd_xf_pop, gDPPipeSync removed,
  gDPSetPrimColor -> nd_val_prim, gDPSetEnvColor -> nd_val_env, gSPSegment -> nd_val_seg, gDPSetTileSize -> nd_val_tile_size).
  It keeps nd_rec_dl and gSPDisplayList (CONV2F is not merged) and the state calls. Each record runs the walker's own code.
- `python 3ds/tools/codemod.py --dry src/effects/dust.c` on main: `value 6 keep 0 state 0 dl 3`.
- The file has no TARGET_3DS line. Its last include is `#include "nd_gtab.h"`. The tool adds no #include.
- Scene: fxpic_16 (fxtest 16 = dust, pm_fxtest.c FXLIST), kind azahar_pic, shots 412 to 415, in bench_defs.json.
- Seed source: dust.c:140 `gDPSetPrimColor(gMainGfxPos++, 0, 0, part->unk_30, part->unk_34, part->unk_38, part->unk_18)`. After edit 1 it is the native call with the same arguments.
Start: `sh 3ds/tools/wt_new.sh CODEMODFX5`. PM_ROW=CODEMODFX5 PM_LANE=CODEMODFX5 on every call.
## Touch only (places)
src/effects/dust.c only (the 6 value call lines and 2 include lines). No other file.
## Exact edits
1. `python 3ds/tools/codemod.py src/effects/dust.c` (rewrites in place).
2. After `#include "nd_gtab.h"` add `#include "nd_val.h"` and `#include "nd_xf.h"` (skip a line that is already there).
3. Commit: `CODEMODFX5: codemod run on dust.c (6 value calls to native records)`.
## Gate (expected values)
1. `python 3ds/tools/codemod.py --dry src/effects/dust.c` after the edit: `value 0 keep 0 state 0 dl 3`.
2. `python 3ds/tools/listcen.py --group src/effects` before edit 1 and after: the build count of dust.c falls by 6. Hand back
   both numbers.
3. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row CODEMODFX5 --why "CODEMODFX5"`: PASS.
4. Byte compare: `bench.py base fxpic_16` at the merge-base, then `bench.py run fxpic_16 --row CODEMODFX5 --why "CODEMODFX5 byte gate"` and
   `bench.py cmp fxpic_16 --row CODEMODFX5`: every shot byte-equal.
5. Seeded fault first (a value change, then undo): in its nd_val_prim call, `part->unk_30` to `part->unk_30 + 1`. cmp fxpic_16 must show a difference. If it shows none: stop and
   hand back.
6. Budget 8 Azahar starts. Counter: C1 -6, C2 -6.
## Stop and end
A cmp difference with the real edit, a seed with no difference, a base pair that differs (BASEKEEP line: tool fault), or a build
error: stop, commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
