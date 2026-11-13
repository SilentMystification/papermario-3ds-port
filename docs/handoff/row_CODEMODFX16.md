# Row CODEMODFX16: codemod run on src/effects/quizmo_stage.c (20 value calls to native records), new scene fxpic_72

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. Recipe of CODEMODFX1, merged 177ac50be.)
## Facts (ORACLE8, main cff324713)
- 3ds/tools/codemod.py rewrites the value calls (gSPMatrix -> nd_xf_push, gSPPopMatrix -> nd_xf_pop, gDPPipeSync removed,
  gDPSetPrimColor -> nd_val_prim, gDPSetEnvColor -> nd_val_env, gSPSegment -> nd_val_seg, gDPSetTileSize -> nd_val_tile_size).
  It keeps nd_rec_dl and gSPDisplayList and the state calls. Each record runs the walker's own code.
- `python 3ds/tools/codemod.py --dry src/effects/quizmo_stage.c` on main: `value 20 keep 0 state 1 dl 13`.
- The file has no TARGET_3DS line. Its last include is `#include "nd_gtab.h"`. The tool adds no #include.
- fxtest 72 = quizmo_stage (pm_fxtest.c FXLIST). No stored scene draws it, so this row adds fxpic_72 (a copy of fxpic_89).
- Seed source (ORACLE8 correction, 2026-11-13: a nd_val_seg seed is dead, see row_TEMPLATE.md): quizmo_stage.c:123 `gSPMatrix(gMainGfxPos++, ..., G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW)` (no guard).
Start: `sh 3ds/tools/wt_new.sh CODEMODFX16`. PM_ROW=CODEMODFX16 PM_LANE=CODEMODFX16 on every call.
## Touch only (places)
src/effects/quizmo_stage.c only (the 20 value call lines and 2 include lines). New 3ds/tools/azt.d/CODEMODFX16.json and
3ds/tools/bench.d/CODEMODFX16.json.
## Exact edits
1. Commit 1 holds only the scene. azt.d/CODEMODFX16.json: fxpic_72, a copy of the fxpic_89 scenario (azt_scenarios.json) with `debug`
   `warp=mac_01:0 pos=0:0:200 fxtest=72 shots shotwin=412:4 loglevel=debug det`, same until, cap 40, deadline 35.
   bench.d/CODEMODFX16.json: fxpic_72, kind azahar_pic, as fxpic_89 in bench_defs.json.
2. `python 3ds/tools/codemod.py src/effects/quizmo_stage.c` (rewrites in place).
3. After `#include "nd_gtab.h"` add `#include "nd_val.h"` and `#include "nd_xf.h"`.
4. Commit: `CODEMODFX16: codemod run on quizmo_stage.c (20 value calls to native records)`.
## Gate (expected values)
1. `python 3ds/tools/codemod.py --dry src/effects/quizmo_stage.c` after the edit: `value 0 keep 0 state 1 dl 13`.
2. `python 3ds/tools/listcen.py --group src/effects` before edit 2 and after: the build count of quizmo_stage.c falls by 20. Hand back
   both numbers.
3. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row CODEMODFX16 --why "CODEMODFX16"`: PASS.
4. At commit 1: `bench.py base fxpic_72`. After edit 4: `bench.py run fxpic_72 --row CODEMODFX16 --why "CODEMODFX16 byte gate"` and
   `bench.py cmp fxpic_72 --row CODEMODFX16`: every shot byte-equal.
5. Seeded fault first (a value change, then undo): in its nd_xf_push call, `G_MTX_LOAD` -> `G_MTX_MUL`. The ELF sha1 must differ from the real edit. cmp fxpic_72 must
   show a difference. If it shows none: stop and hand back.
6. Budget 8 Azahar starts. Counter: C1 -20, C2 -20.
## Stop and end
A cmp difference with the real edit, a seed with no difference, a base pair that differs (BASEKEEP line: tool fault), or a build
error: stop, commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
