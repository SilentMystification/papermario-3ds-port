# Row CODEMODFX18: codemod run on src/effects/whirlwind.c (14 value calls to native records), new scene fxpic_97

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. Recipe of CODEMODFX1, merged 177ac50be.)
## Facts (ORACLE8, main cff324713)
- 3ds/tools/codemod.py rewrites the value calls (gSPMatrix -> nd_xf_push, gSPPopMatrix -> nd_xf_pop, gDPPipeSync removed,
  gDPSetPrimColor -> nd_val_prim, gDPSetEnvColor -> nd_val_env, gSPSegment -> nd_val_seg, gDPSetTileSize -> nd_val_tile_size).
  It keeps nd_rec_dl and gSPDisplayList and the state calls. Each record runs the walker's own code.
- `python 3ds/tools/codemod.py --dry src/effects/whirlwind.c` on main: `value 14 keep 0 state 3 dl 1`.
- The file has no TARGET_3DS line. Its last include is `#include "nd_gtab.h"`. The tool adds no #include.
- fxtest 97 = whirlwind (pm_fxtest.c FXLIST). No stored scene draws it, so this row adds fxpic_97 (a copy of fxpic_89).
- Seed source (ORACLE8, 2026-11-13, seed rule of row_TEMPLATE.md): whirlwind.c:179 `guTranslateF(sp20, 0.0f, 0.0f, 0.0f)`, the first transform of the file (no
  guard before it). Every draw of the effect moves by 20 units.
Start: `sh 3ds/tools/wt_new.sh CODEMODFX18`. PM_ROW=CODEMODFX18 PM_LANE=CODEMODFX18 on every call.
## Touch only (places)
src/effects/whirlwind.c only (the 14 value call lines and 2 include lines). New 3ds/tools/azt.d/CODEMODFX18.json and
3ds/tools/bench.d/CODEMODFX18.json.
## Exact edits
1. Commit 1 holds only the scene. azt.d/CODEMODFX18.json: fxpic_97, a copy of the fxpic_89 scenario (azt_scenarios.json) with `debug`
   `warp=mac_01:0 pos=0:0:200 fxtest=97 shots shotwin=412:4 loglevel=debug det`, same until, cap 40, deadline 35.
   bench.d/CODEMODFX18.json: fxpic_97, kind azahar_pic, as fxpic_89 in bench_defs.json.
2. `python 3ds/tools/codemod.py src/effects/whirlwind.c` (rewrites in place).
3. After `#include "nd_gtab.h"` add `#include "nd_val.h"` and `#include "nd_xf.h"`.
4. Commit: `CODEMODFX18: codemod run on whirlwind.c (14 value calls to native records)`.
## Gate (expected values)
0. Draw proof first, from the log of the base run (no extra start): after `fxtest: spawn 97` the log has `fxtest: live 1` at
   shot 1, and `fxcenb 97 tri <T>` with T above the `fxcenb -1 tri` value of the base window (the file adds triangles).
   No: stop and hand back (the scene does not draw the file).
1. `python 3ds/tools/codemod.py --dry src/effects/whirlwind.c` after the edit: `value 0 keep 0 state 3 dl 1`.
2. `python 3ds/tools/listcen.py --group src/effects` before edit 2 and after: the build count of whirlwind.c falls by 14. Hand back
   both numbers.
3. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row CODEMODFX18 --why "CODEMODFX18"`: PASS.
4. At commit 1: `bench.py base fxpic_97`. After edit 4: `bench.py run fxpic_97 --row CODEMODFX18 --why "CODEMODFX18 byte gate"` and
   `bench.py cmp fxpic_97 --row CODEMODFX18`: every shot byte-equal.
5. Seeded fault first (a value change, then undo): at :179 the y argument `0.0f` -> `20.0f`. The ELF sha1 must differ from the real edit. cmp fxpic_97
   must show a difference. If it shows none: stop and hand back.
6. Budget 8 Azahar starts. Counter: C1 -14, C2 -14.
## Stop and end
A cmp difference with the real edit, a seed with no difference, a base pair that differs (BASEKEEP line: tool fault), or a build
error: stop, commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
