# Row CODEMODFX11: codemod run on src/effects/snaking_static.c (6 value calls to native records)

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. Recipe of CODEMODFX1, merged 177ac50be.)
## Facts (ORACLE8, main cff324713)
- 3ds/tools/codemod.py rewrites the value calls (gSPMatrix -> nd_xf_push, gSPPopMatrix -> nd_xf_pop, gDPPipeSync removed,
  gDPSetPrimColor -> nd_val_prim, gDPSetEnvColor -> nd_val_env, gSPSegment -> nd_val_seg, gDPSetTileSize -> nd_val_tile_size).
  It keeps nd_rec_dl and gSPDisplayList and the state calls. Each record runs the walker's own code.
- `python 3ds/tools/codemod.py --dry src/effects/snaking_static.c` on main: `value 6 keep 5 state 1 dl 2`. The 5 kept calls write into a local Gfx array (first argument is not gMainGfxPos++): they stay.
- The file has no TARGET_3DS line. Its last include is `#include "nd_api.h"`. The tool adds no #include.
- Scene: fxpic_81 (fxtest 81 = snaking_static, pm_fxtest.c FXLIST), kind azahar_pic, shots 412 to 415, in bench_defs.json.
- Seed source (ORACLE8, 2026-11-13, seed rule of row_TEMPLATE.md): snaking_static.c:178 `guTranslateF(sp18, data->pos.x, data->pos.y, data->pos.z)`, the first transform of the file (no
  guard before it). Every draw of the effect moves by 20 units.
Start: `sh 3ds/tools/wt_new.sh CODEMODFX11`. PM_ROW=CODEMODFX11 PM_LANE=CODEMODFX11 on every call.
## Touch only (places)
src/effects/snaking_static.c only (the 6 value call lines and 2 include lines). No other file.
## Exact edits
1. `python 3ds/tools/codemod.py src/effects/snaking_static.c` (rewrites in place).
2. After `#include "nd_api.h"` add `#include "nd_val.h"` and `#include "nd_xf.h"` (skip a line that is already there).
3. Commit: `CODEMODFX11: codemod run on snaking_static.c (6 value calls to native records)`.
## Gate (expected values)
0. Draw proof first, from the log of the base run (no extra start): after `fxtest: spawn 81` the log has `fxtest: live 1` at
   shot 1, and `fxcenb 81 tri <T>` with T above the `fxcenb -1 tri` value of the base window (the file adds triangles).
   No: stop and hand back (the scene does not draw the file).
1. `python 3ds/tools/codemod.py --dry src/effects/snaking_static.c` after the edit: `value 0 keep 5 state 1 dl 2`.
2. `python 3ds/tools/listcen.py --group src/effects` before edit 1 and after: the build count of snaking_static.c falls by 6. Hand back
   both numbers.
3. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row CODEMODFX11 --why "CODEMODFX11"`: PASS.
4. Byte compare: `bench.py base fxpic_81` at the merge-base, then `bench.py run fxpic_81 --row CODEMODFX11 --why "CODEMODFX11 byte gate"` and
   `bench.py cmp fxpic_81 --row CODEMODFX11`: every shot byte-equal.
5. Seeded fault first (a value change, then undo): at :178 `data->pos.y` -> `data->pos.y + 20.0f`. The ELF sha1 must differ from the real edit. cmp fxpic_81
   must show a difference. If it shows none: stop and hand back.
6. Budget 8 Azahar starts. Counter: C1 -6, C2 -6.
## Stop and end
A cmp difference with the real edit, a seed with no difference, a base pair that differs (BASEKEEP line: tool fault), or a build
error: stop, commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
