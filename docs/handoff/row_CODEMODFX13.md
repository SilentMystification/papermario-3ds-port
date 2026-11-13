# Row CODEMODFX13: codemod run on src/effects/snowfall.c (4 value calls to native records)

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. Recipe of CODEMODFX1, merged 177ac50be.)
## Facts (ORACLE8, main cff324713)
- 3ds/tools/codemod.py rewrites the value calls (gSPMatrix -> nd_xf_push, gSPPopMatrix -> nd_xf_pop, gDPPipeSync removed,
  gDPSetPrimColor -> nd_val_prim, gDPSetEnvColor -> nd_val_env, gSPSegment -> nd_val_seg, gDPSetTileSize -> nd_val_tile_size).
  It keeps nd_rec_dl and gSPDisplayList and the state calls. Each record runs the walker's own code.
- `python 3ds/tools/codemod.py --dry src/effects/snowfall.c` on main: `value 4 keep 3 state 0 dl 2`. The 3 kept calls write into a local Gfx array (first argument is not gMainGfxPos++): they stay.
- The file has no TARGET_3DS line. Its last include is `#include "nd_api.h"`. The tool adds no #include.
- Scene: fxpic_63 (fxtest 63 = snowfall, pm_fxtest.c FXLIST), kind azahar_pic, shots 412 to 415, in bench_defs.json.
- Gate form (ORACLE8, 2026-11-13, row_TEMPLATE.md gate rule): the drec gate. In CODEMODFX10 three seeds with three ELFs left
  all 6 fxpic_50 shots byte-equal, though the effect drew triangles in the window: the shots do not show the effect. drec
  records every draw of the window from game memory, seen or not.
- Seed source (ORACLE8, 2026-11-13, seed rule of row_TEMPLATE.md): snowfall.c:214 `guTranslateF(sp18, data->unk_08, data->unk_0C, data->unk_10)`, the first transform of the file (no
  guard before it). Every draw of the effect moves by 20 units.
Start: `sh 3ds/tools/wt_new.sh CODEMODFX13`. PM_ROW=CODEMODFX13 PM_LANE=CODEMODFX13 on every call.
## Touch only (places)
src/effects/snowfall.c (the 4 value call lines and 2 include lines). New azt.d/CODEMODFX13.json and bench.d/CODEMODFX13.json.
## Exact edits
1. `python 3ds/tools/codemod.py src/effects/snowfall.c` (rewrites in place).
2. After `#include "nd_api.h"` add `#include "nd_val.h"` and `#include "nd_xf.h"` (skip a line that is already there).
3. Commit: `CODEMODFX13: codemod run on snowfall.c (4 value calls to native records)`.
## Gate (expected values)
0. Draw proof first, from the log of the base run (no extra start): after `fxtest: spawn 63` the log has `fxtest: live 1` at
   shot 1, and `fxcenb 63 tri <T>` with T above the `fxcenb -1 tri` value of the base window (the file adds triangles).
   No: stop and hand back (the scene does not draw the file).
1. `python 3ds/tools/codemod.py --dry src/effects/snowfall.c` after the edit: `value 0 keep 3 state 0 dl 2`.
2. `python 3ds/tools/listcen.py --group src/effects` before edit 1 and after: the build count of snowfall.c falls by 4. Hand back
   both numbers.
3. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row CODEMODFX13 --why "CODEMODFX13"`: PASS.
4. Drec gate: add azt.d/CODEMODFX13.json fxpic_63_drec, a copy of the fxpic_63 scenario with `debug`
   `warp=mac_01:0 pos=0:0:200 fxtest=63 loglevel=debug det drec=412:2`, until `drec f413 draws [1-9]\d* bytes \d+`, needs_picture false,
   cap 40, deadline 35, and bench.d/CODEMODFX13.json (kind azahar_pic, tags azahar det drec, as spot_mac01_drec). Commit them.
   `bench.py base fxpic_63_drec` (merge-base build), `bench.py run fxpic_63_drec --row CODEMODFX13 --why "CODEMODFX13 drec gate"`, `bench.py cmp fxpic_63_drec
   --row CODEMODFX13`: drec.txt byte-equal.
5. Seeded fault first (a value change, then undo): at :214 `data->unk_0C` -> `data->unk_0C + 20.0f`. The ELF sha1 must differ from the real edit. cmp fxpic_63_drec
   must show a difference. If it shows none: stop and hand back.
6. Budget 8 Azahar starts. Counter: C1 -4, C2 -4.
## Stop and end
A drec difference with the real edit, a seed with no difference, a base pair that differs (BASEKEEP line: tool fault), or a build
error: stop, commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
