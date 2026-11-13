# Row CODEMODFX14: codemod run on src/effects/pink_sparkles.c (3 value calls to native records)

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. Recipe of CODEMODFX1, merged 177ac50be.)
## Facts (ORACLE8, main cff324713)
- 3ds/tools/codemod.py rewrites the value calls (gSPMatrix -> nd_xf_push, gSPPopMatrix -> nd_xf_pop, gDPPipeSync removed,
  gDPSetPrimColor -> nd_val_prim, gDPSetEnvColor -> nd_val_env, gSPSegment -> nd_val_seg, gDPSetTileSize -> nd_val_tile_size).
  It keeps nd_rec_dl and gSPDisplayList and the state calls. Each record runs the walker's own code.
- `python 3ds/tools/codemod.py --dry src/effects/pink_sparkles.c` on main: `value 3 keep 3 state 0 dl 2`. The 3 kept calls write into a local Gfx array (first argument is not gMainGfxPos++): they stay.
- The file has no TARGET_3DS line. Its last include is `#include "nd_api.h"`. The tool adds no #include.
- Scene: fxpic_124 (fxtest 124 = pink_sparkles, pm_fxtest.c FXLIST), kind azahar_pic, shots 412 to 415, in bench_defs.json.
- Gate form (ORACLE8, 2026-11-13, row_TEMPLATE.md gate rule): the drec gate. In CODEMODFX10 three seeds with three ELFs left
  all 6 fxpic_50 shots byte-equal, though the effect drew triangles in the window: the shots do not show the effect. drec
  records every draw of the window from game memory, seen or not.
- Seed source (ORACLE8, 2026-11-13, seed rule of row_TEMPLATE.md): pink_sparkles.c:251 `guTranslateF(sp98, part->unk_04, part->unk_08, part->unk_0C)`, the first transform of the file (no
  guard before it). Every draw of the effect moves by 20 units.
Start: `sh 3ds/tools/wt_new.sh CODEMODFX14`. PM_ROW=CODEMODFX14 PM_LANE=CODEMODFX14 on every call.
## Touch only (places)
src/effects/pink_sparkles.c (the 3 value call lines and 2 include lines). New azt.d/CODEMODFX14.json and bench.d/CODEMODFX14.json.
## Exact edits
1. `python 3ds/tools/codemod.py src/effects/pink_sparkles.c` (rewrites in place).
2. After `#include "nd_api.h"` add `#include "nd_val.h"` and `#include "nd_xf.h"` (skip a line that is already there).
3. Commit: `CODEMODFX14: codemod run on pink_sparkles.c (3 value calls to native records)`.
## Gate (expected values)
0. Draw proof first, from the log of the base run (no extra start): after `fxtest: spawn 124` the log has `fxtest: live 1` at
   shot 1, and `fxcenb 124 tri <T>` with T above the `fxcenb -1 tri` value of the base window (the file adds triangles).
   No: stop and hand back (the scene does not draw the file).
1. `python 3ds/tools/codemod.py --dry src/effects/pink_sparkles.c` after the edit: `value 0 keep 3 state 0 dl 2`.
2. `python 3ds/tools/listcen.py --group src/effects` before edit 1 and after: the build count of pink_sparkles.c falls by 3. Hand back
   both numbers.
3. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row CODEMODFX14 --why "CODEMODFX14"`: PASS.
4. Drec gate: add azt.d/CODEMODFX14.json fxpic_124_drec, a copy of the fxpic_124 scenario with `debug`
   `warp=mac_01:0 pos=0:0:200 fxtest=124 loglevel=debug det drec=412:2`, until `drec f413 draws [1-9]\d* bytes \d+`, needs_picture false,
   cap 40, deadline 35, and bench.d/CODEMODFX14.json (kind azahar_pic, tags azahar det drec, as spot_mac01_drec). Commit them.
   `bench.py base fxpic_124_drec` (merge-base build), `bench.py run fxpic_124_drec --row CODEMODFX14 --why "CODEMODFX14 drec gate"`, `bench.py cmp fxpic_124_drec
   --row CODEMODFX14`: drec.txt byte-equal.
5. Seeded fault first (a value change, then undo): at :251 `part->unk_08` -> `part->unk_08 + 20.0f`. The ELF sha1 must differ from the real edit. cmp fxpic_124_drec
   must show a difference. If it shows none: stop and hand back.
6. Budget 8 Azahar starts. Counter: C1 -3, C2 -3.
## Stop and end
A drec difference with the real edit, a seed with no difference, a base pair that differs (BASEKEEP line: tool fault), or a build
error: stop, commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
