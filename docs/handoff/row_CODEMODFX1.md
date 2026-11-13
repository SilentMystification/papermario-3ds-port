# Row CODEMODFX1: the first codemod run, value calls of src/effects/water_splash.c and src/effects/music_note.c

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names.)
## Facts (ORACLE8, main ac0e68cd5)
- 3ds/tools/codemod.py (CODEMOD, merged) rewrites the 7 value calls: gSPMatrix -> nd_xf_push, gSPPopMatrix -> nd_xf_pop (NXF1,
  merged), gDPPipeSync removed, gDPSetPrimColor -> nd_val_prim, gDPSetEnvColor -> nd_val_env, gSPSegment -> nd_val_seg,
  gDPSetTileSize -> nd_val_tile_size. It keeps nd_rec_dl and gSPDisplayList (nd_draw_list needs CONV2F, not merged) and the state
  calls (they need ND_PSO_DESC: NPSO3 to NPSO5). Each record runs the walker's own code (nd_walk_state), so the picture stays.
- `python 3ds/tools/codemod.py --dry src/effects/water_splash.c src/effects/music_note.c` on main: water_splash value 9 keep 0
  state 3 dl 2, music_note value 6 keep 0 state 0 dl 2. Total value 15.
- The tool adds no #include. The rewritten files need 3ds/include/nd_val.h and nd_xf.h (3ds/include is on the build path).
- Scenes: fxpic_89 (fxtest 89 = water_splash, pm_fxtest.c table) and fxpic_33 (fxtest 33 = music_note), shots at the effect,
  kind azahar_pic. Both stored equal bases at older commits.
Start: `sh 3ds/tools/wt_new.sh CODEMODFX1`. PM_ROW=CODEMODFX1 PM_LANE=CODEMODFX1 on every call.
## Touch only (places)
src/effects/water_splash.c and src/effects/music_note.c (the 15 value call lines and one include line each). No other file.
(N64DEL1 is merged: these files have no TARGET_3DS branch left. No open row edits src/effects.)
## Exact edits
1. `python 3ds/tools/codemod.py src/effects/water_splash.c src/effects/music_note.c` (rewrites in place).
2. In each of the 2 files, after its last `#include` line add `#include "nd_val.h"` and `#include "nd_xf.h"`.
3. Commit: `CODEMODFX1: codemod run on water_splash.c and music_note.c (15 value calls to native records)`.
## Gate (expected values)
1. `python 3ds/tools/codemod.py --dry src/effects/water_splash.c src/effects/music_note.c` after the edit: value 0 keep 0 for
   both files (nothing left to rewrite), state 3 and 0, dl 2 and 2 (unchanged).
2. `python 3ds/tools/listcen.py --group src/effects`: water_splash.c and music_note.c together 15 build calls less than before
   the edit (run it before edit 1 too and hand back both numbers).
3. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row CODEMODFX1 --why "CODEMODFX1"`: PASS.
4. Byte compare: `bench.py base fxpic_89` and `base fxpic_33` at the merge-base, then
   `bench.py run fxpic_89,fxpic_33 --row CODEMODFX1 --why "CODEMODFX1 byte gate"`, `bench.py cmp` of each: every shot byte-equal.
5. Seeded fault first (a value change, then undo): in water_splash.c change the alpha argument of its first nd_val_prim call by
   +1 (one value). cmp fxpic_89 must show a difference (the scene draws this file). If it shows none: stop and hand back.
6. Budget 8 Azahar starts. Counter: C1 -15, C2 -15 (the gMainGfxPos++ arguments go with the calls).
## Stop and end
A cmp difference with the real edit, a seed with no difference, a base pair that differs (BASEKEEP line: tool fault), or a build
error: stop, commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
