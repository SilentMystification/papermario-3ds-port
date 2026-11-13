# Row CODEMODFX9: codemod run on src/effects/embers.c (7 value calls to native records)

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. Recipe of CODEMODFX1, merged 177ac50be.)
## Facts (ORACLE8, main 0250d5221)
- 3ds/tools/codemod.py rewrites the value calls (gSPMatrix -> nd_xf_push, gSPPopMatrix -> nd_xf_pop, gDPPipeSync removed,
  gDPSetPrimColor -> nd_val_prim, gDPSetEnvColor -> nd_val_env, gSPSegment -> nd_val_seg, gDPSetTileSize -> nd_val_tile_size).
  It keeps nd_rec_dl and gSPDisplayList (CONV2F is not merged) and the state calls. Each record runs the walker's own code.
- `python 3ds/tools/codemod.py --dry src/effects/embers.c` on main: `value 7 keep 4 state 0 dl 2`. The 4 kept calls write into a local Gfx array (first argument is not gMainGfxPos++): they stay.
- The file has no TARGET_3DS line. Its last include is `#include "nd_api.h"`. The tool adds no #include.
- Scene: fxpic_105 (fxtest 105 = embers, pm_fxtest.c FXLIST), kind azahar_pic, shots 412 to 415, in bench_defs.json.
- Seed source: embers.c:209 `gDPSetEnvColor(gMainGfxPos++, part->unk_30, part->unk_34, part->unk_38, part->unk_3C)`. After edit 1 it is the native call with the same arguments.
Start: `sh 3ds/tools/wt_new.sh CODEMODFX9`. PM_ROW=CODEMODFX9 PM_LANE=CODEMODFX9 on every call.
## Touch only (places)
src/effects/embers.c only (the 7 value call lines and 2 include lines). No other file.
## Exact edits
1. `python 3ds/tools/codemod.py src/effects/embers.c` (rewrites in place).
2. After `#include "nd_api.h"` add `#include "nd_val.h"` and `#include "nd_xf.h"` (skip a line that is already there).
3. Commit: `CODEMODFX9: codemod run on embers.c (7 value calls to native records)`.
## Gate (expected values)
1. `python 3ds/tools/codemod.py --dry src/effects/embers.c` after the edit: `value 0 keep 4 state 0 dl 2`.
2. `python 3ds/tools/listcen.py --group src/effects` before edit 1 and after: the build count of embers.c falls by 7. Hand back
   both numbers.
3. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row CODEMODFX9 --why "CODEMODFX9"`: PASS.
4. Byte compare: `bench.py base fxpic_105` at the merge-base, then `bench.py run fxpic_105 --row CODEMODFX9 --why "CODEMODFX9 byte gate"` and
   `bench.py cmp fxpic_105 --row CODEMODFX9`: every shot byte-equal.
5. Seeded fault first (a value change, then undo): in its nd_val_env call, `part->unk_30` to `part->unk_30 + 1`. cmp fxpic_105 must show a difference. If it shows none: stop and
   hand back.
6. Budget 8 Azahar starts. Counter: C1 -7, C2 -7.
## Stop and end
A cmp difference with the real edit, a seed with no difference, a base pair that differs (BASEKEEP line: tool fault), or a build
error: stop, commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
