# Row CODEMODFX7: codemod run on src/effects/big_smoke_puff.c (9 value calls to native records)

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. Recipe of CODEMODFX1, merged 177ac50be.)
## Facts (ORACLE8, main 0250d5221)
- 3ds/tools/codemod.py rewrites the value calls (gSPMatrix -> nd_xf_push, gSPPopMatrix -> nd_xf_pop, gDPPipeSync removed,
  gDPSetPrimColor -> nd_val_prim, gDPSetEnvColor -> nd_val_env, gSPSegment -> nd_val_seg, gDPSetTileSize -> nd_val_tile_size).
  It keeps nd_rec_dl and gSPDisplayList (CONV2F is not merged) and the state calls. Each record runs the walker's own code.
- `python 3ds/tools/codemod.py --dry src/effects/big_smoke_puff.c` on main: `value 9 keep 0 state 0 dl 1`.
- The file has no TARGET_3DS line. Its last include is `#include "nd_gtab.h"`. The tool adds no #include.
- Scene: fxpic_0 (fxtest 0 = big_smoke_puff, pm_fxtest.c FXLIST), kind azahar_pic, shots 412 to 415, in bench_defs.json.
- Seed source: big_smoke_puff.c:169 `gDPSetEnvColor(gMainGfxPos++, 0, 0, 0, envAlpha)`. After edit 1 it is the native call with the same arguments.
Start: `sh 3ds/tools/wt_new.sh CODEMODFX7`. PM_ROW=CODEMODFX7 PM_LANE=CODEMODFX7 on every call.
## Touch only (places)
src/effects/big_smoke_puff.c (the 9 value call lines and 2 include lines). New azt.d/CODEMODFX7.json and bench.d/CODEMODFX7.json.
## Exact edits
1. `python 3ds/tools/codemod.py src/effects/big_smoke_puff.c` (rewrites in place).
2. After `#include "nd_gtab.h"` add `#include "nd_val.h"` and `#include "nd_xf.h"` (skip a line that is already there).
3. Commit: `CODEMODFX7: codemod run on big_smoke_puff.c (9 value calls to native records)`.
## Gate (expected values)
1. `python 3ds/tools/codemod.py --dry src/effects/big_smoke_puff.c` after the edit: `value 0 keep 0 state 0 dl 1`.
2. `python 3ds/tools/listcen.py --group src/effects` before edit 1 and after: the build count of big_smoke_puff.c falls by 9. Hand back
   both numbers.
3. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row CODEMODFX7 --why "CODEMODFX7"`: PASS.
Correction 1 (ORACLE8, 2026-11-13). Resume after 9489d0c9b (gates 1-3 passed).
- The trace: in run 878721 and the run after it, all 6 shots differ by the same 84455 px in the same box. That includes shot 0 at
  frame 400 (fxtest `base`, live 0) and shot 5 at frame 432 (live 0), where no smoke draws. So the difference is not
  big_smoke_puff. fxpic_89 showed the same failure (same count and box) at db17115ac under load. BOOTSTATE (merge 1bef1b333)
  logged the game's global inputs in 3 runs: layout, interp, fps60, camera, Mario and seed were equal at every frame. The
  options.ini that run_azahar.ps1:231 copies is unchanged since 2026-11-08 (wide=1, fps60=0). No game input explains it. The
  shots read the framebuffer back from the emulator (pm_gpu.c dump_shot), and the failure occurs only with many Azahar slots at
  once. The producer is outside the game code (open tool fault, for the user). This row moves to the drec gate: drec records
  the draw inputs from game memory at nd_native_submit (HOSTCORE2, pm_gpu.c:574) and does not read the framebuffer.
4. Add azt.d/CODEMODFX7.json fxpic_0_drec: a copy of the fxpic_0 scenario with `debug`
   `warp=mac_01:0 pos=0:0:200 fxtest=0 loglevel=debug det drec=412:2`, until `drec f413 draws [1-9]\d* bytes \d+`,
   needs_picture false, cap 40, deadline 35. Add bench.d/CODEMODFX7.json: fxpic_0_drec, kind azahar_pic, tags azahar det drec,
   as spot_mac01_drec (bench.d/HOSTCORE2.json). Commit them. Then `bench.py base fxpic_0_drec` (merge-base build: drec is
   there since HOSTCORE2), `bench.py run fxpic_0_drec --row CODEMODFX7 --why "CODEMODFX7 drec gate"`, and
   `bench.py cmp fxpic_0_drec --row CODEMODFX7`: drec.txt byte-equal.
5. Seeded fault (a value change, then undo): the nd_val_seg call at :131 with base + 16. This moves the segment 9 texel
   and vertex addresses that drec records. Before the run, the sha1sum of build3ds/lane-CODEMODFX7/pm_3ds.elf must differ
   from its value at the real edit. cmp fxpic_0_drec must differ. Budget: one reset for this oracle correction (CLAUDE.md 4).
6. Budget 8 Azahar starts. Counter: C1 -9, C2 -9.
## Stop and end
A cmp difference with the real edit, a seed with no difference, a base pair that differs (BASEKEEP line: tool fault), or a build
error: stop, commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
