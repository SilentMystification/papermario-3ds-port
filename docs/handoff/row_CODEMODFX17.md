# Row CODEMODFX17: codemod run on src/effects/peach_star_beam.c (15 value calls to native records), new scene fxpic_112

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. Recipe of CODEMODFX1, merged 177ac50be.)
## Facts (ORACLE8, main cff324713)
- 3ds/tools/codemod.py rewrites the value calls (gSPMatrix -> nd_xf_push, gSPPopMatrix -> nd_xf_pop, gDPPipeSync removed,
  gDPSetPrimColor -> nd_val_prim, gDPSetEnvColor -> nd_val_env, gSPSegment -> nd_val_seg, gDPSetTileSize -> nd_val_tile_size).
  It keeps nd_rec_dl and gSPDisplayList and the state calls. Each record runs the walker's own code.
- `python 3ds/tools/codemod.py --dry src/effects/peach_star_beam.c` on main: `value 15 keep 0 state 0 dl 8`.
- The file has no TARGET_3DS line. Its last include is `#include "nd_gtab.h"`. The tool adds no #include.
- fxtest 112 = peach_star_beam (pm_fxtest.c FXLIST). No stored scene draws it, so this row adds fxpic_112 (a copy of fxpic_89).
- Seed source (ORACLE8 correction, 2026-11-13: a nd_val_seg seed is dead, see row_TEMPLATE.md): peach_star_beam.c:218 `gSPMatrix(gMainGfxPos++, camera->mtxBillboard, G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW)` (no guard).
Start: `sh 3ds/tools/wt_new.sh CODEMODFX17`. PM_ROW=CODEMODFX17 PM_LANE=CODEMODFX17 on every call.
## Touch only (places)
src/effects/peach_star_beam.c only (the 15 value call lines and 2 include lines). New 3ds/tools/azt.d/CODEMODFX17.json and
3ds/tools/bench.d/CODEMODFX17.json.
## Exact edits
1. Commit 1 holds only the scene. azt.d/CODEMODFX17.json: fxpic_112, a copy of the fxpic_89 scenario (azt_scenarios.json) with `debug`
   `warp=mac_01:0 pos=0:0:200 fxtest=112 shots shotwin=412:4 loglevel=debug det`, same until, cap 40, deadline 35.
   bench.d/CODEMODFX17.json: fxpic_112, kind azahar_pic, as fxpic_89 in bench_defs.json.
2. `python 3ds/tools/codemod.py src/effects/peach_star_beam.c` (rewrites in place).
3. After `#include "nd_gtab.h"` add `#include "nd_val.h"` and `#include "nd_xf.h"`.
4. Commit: `CODEMODFX17: codemod run on peach_star_beam.c (15 value calls to native records)`.
## Gate (expected values)
1. `python 3ds/tools/codemod.py --dry src/effects/peach_star_beam.c` after the edit: `value 0 keep 0 state 0 dl 8`.
2. `python 3ds/tools/listcen.py --group src/effects` before edit 2 and after: the build count of peach_star_beam.c falls by 15. Hand back
   both numbers.
3. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row CODEMODFX17 --why "CODEMODFX17"`: PASS.
4. At commit 1: `bench.py base fxpic_112`. After edit 4: `bench.py run fxpic_112 --row CODEMODFX17 --why "CODEMODFX17 byte gate"` and
   `bench.py cmp fxpic_112 --row CODEMODFX17`: every shot byte-equal.
5. Seeded fault first (a value change, then undo): in its nd_xf_push call, `G_MTX_MUL` -> `G_MTX_LOAD`. The ELF sha1 must differ from the real edit. cmp fxpic_112 must
   show a difference. If it shows none: stop and hand back.
6. Budget 8 Azahar starts. Counter: C1 -15, C2 -15.
## Stop and end
A cmp difference with the real edit, a seed with no difference, a base pair that differs (BASEKEEP line: tool fault), or a build
error: stop, commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
