# Row CODEMODB3: codemod batch of 10 effect files (76 value calls to native records), drec gate over one fxtest window

(Written by the oracle in the mkrow.py form before the probes; it starts when rowlint shows no needs fault, i.e. every
seed below has the verdict live on main.)
(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md, row_TEMPLATE.md and what it names. Written by mkrow.py from
row_BATCHTEMPLATE.md and a seeds table; every seed has the verdict live from a probe row.)
## Facts (mkrow.py, main 351c8861e)
- 3ds/tools/codemod.py rewrites the value calls (gSPMatrix, gSPPopMatrix, gDPPipeSync, gDPSetPrimColor, gDPSetEnvColor, gSPSegment,
  gDPSetTileSize) to nd_xf_* and nd_val_*. The files (codemod --dry: value keep state dl | fxtest id | drec frame | live seed):
  - src/effects/drop_leaves.c: 4 0 0 2 | fxtest 15 | frame 412 | seed `src/effects/drop_leaves.c:141@412|part->unk_08|part->unk_08 + 20.0f`
  - src/effects/effect_46.c: 7 0 0 2 | fxtest 64 | frame 456 | seed `src/effects/effect_46.c:219@456|part->pos.y|part->pos.y + 20.0f`
  - src/effects/effect_65.c: 6 0 2 1 | fxtest 95 | frame 500 | seed `src/effects/effect_65.c:254@500|mtx, 0.0f, 0.0f|mtx, 0.0f, 0.0f + 20.0f`
  - src/effects/effect_75.c: 9 0 0 2 | fxtest 110 | frame 544 | seed `src/effects/effect_75.c:207@544|data->pos.y|data->pos.y + 20.0f`
  - src/effects/effect_86.c: 7 0 0 2 | fxtest 126 | frame 588 | seed `src/effects/effect_86.c:122@588|part->unk_08|part->unk_08 + 20.0f`
  - src/effects/ending_decals.c: 9 0 4 2 | fxtest 46 | frame 632 | seed `src/effects/ending_decals.c:149@632|data->pos.y|data->pos.y + 20.0f`
  - src/effects/energy_in_out.c: 9 0 0 2 | fxtest 100 | frame 676 | seed `src/effects/energy_in_out.c:259@676|part->pos.y|part->pos.y + 20.0f`
  - src/effects/energy_shockwave.c: 9 0 0 2 | fxtest 51 | frame 720 | seed `src/effects/energy_shockwave.c:164@720|data->unk_08|data->unk_08 + 20.0f`
  - src/effects/falling_leaves.c: 4 0 0 2 | fxtest 38 | frame 764 | seed `src/effects/falling_leaves.c:136@764|part->unk_08|part->unk_08 + 20.0f`
  - src/effects/fire_flower.c: 12 0 2 4 | fxtest 57 | frame 808 | seed `src/effects/fire_flower.c:218@808|part->pos.y|part->pos.y + 20.0f`
- fxtest runs the ids in the order of the switch, one per 44 frames from frame 400 (pm_fxtest.c:179-180). File i draws at frame
  412 + 44 i. drec=412:10:44 records those frames (DRECSTEP frame lines).
Start: `sh 3ds/tools/wt_new.sh CODEMODB3`. PM_ROW=CODEMODB3 PM_LANE=CODEMODB3 on every call.
## Touch only (places)
The value call lines and the include lines of: src/effects/drop_leaves.c, src/effects/effect_46.c, src/effects/effect_65.c, src/effects/effect_75.c, src/effects/effect_86.c, src/effects/ending_decals.c, src/effects/energy_in_out.c, src/effects/energy_shockwave.c, src/effects/falling_leaves.c, src/effects/fire_flower.c. New azt.d/CODEMODB3.json and bench.d/CODEMODB3.json.
New names: codemodb3_drec. Host link: none.
Needs: docs/handoff/row_SEEDS.md has drop_leaves | 15 | 4 0 0 2 | 141 | `guTranslateF(sp18, part->unk_04, part->unk_08, part->unk_0C);` | `part->unk_08` | live
Needs: docs/handoff/row_SEEDS.md has effect_46 | 64 | 7 0 0 2 | 219 | `guTranslateF(mtxTransform, part->pos.x, part->pos.y, part->pos.z);` | `part->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has effect_65 | 95 | 6 0 2 1 | 254 | `guTranslateF(mtx, 0.0f, 0.0f, 0.0f);` | `mtx, 0.0f, 0.0f` | live
Needs: docs/handoff/row_SEEDS.md has effect_75 | 110 | 9 0 0 2 | 207 | `guTranslateF(mtxTransfrom, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has effect_86 | 126 | 7 0 0 2 | 122 | `guTranslateF(sp10, part->unk_04, part->unk_08, part->unk_0C);` | `part->unk_08` | live
Needs: docs/handoff/row_SEEDS.md has ending_decals | 46 | 9 0 4 2 | 149 | `guPositionF(sp20, 0.0f, -gCameras[gCurrentCameraID].curYaw, 0.0f, data->scale, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has energy_in_out | 100 | 9 0 0 2 | 259 | `guPositionF(sp20, 0.0f, -gCameras[gCurrentCameraID].curYaw, 0.0f, scale, part->pos.x, part->pos.y, part->pos.z);` | `part->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has energy_shockwave | 51 | 9 0 0 2 | 164 | `guTranslateF(sp10, data->unk_04, data->unk_08, data->unk_0C);` | `data->unk_08` | live
Needs: docs/handoff/row_SEEDS.md has falling_leaves | 38 | 4 0 0 2 | 136 | `guTranslateF(sp18, part->unk_04, part->unk_08, part->unk_0C);` | `part->unk_08` | live
Needs: docs/handoff/row_SEEDS.md has fire_flower | 57 | 12 0 2 4 | 218 | `guTranslateF(sp18, part->pos.x, part->pos.y, part->pos.z);` | `part->pos.y` | live
## Exact edits
1. azt.d/CODEMODB3.json codemodb3_drec: a copy of the fxpic_89 scenario (azt_scenarios.json: first_marker and warp event `warp: mac_01 entry 0`; not the prof marker of spot_mac01_drec) with `debug`
   `warp=mac_01:0 pos=0:0:200 fxtest=15,64,95,110,126,46,100,51,38,57 loglevel=debug det drec=412:10:44`, until `drec f808 draws [1-9]\d* bytes \d+`,
   cap 45, deadline 40, needs_picture false. bench.d/CODEMODB3.json: codemodb3_drec, kind azahar_pic, tags azahar det drec. Commit them.
2. `python 3ds/tools/codemod.py src/effects/drop_leaves.c src/effects/effect_46.c src/effects/effect_65.c src/effects/effect_75.c src/effects/effect_86.c src/effects/ending_decals.c src/effects/energy_in_out.c src/effects/energy_shockwave.c src/effects/falling_leaves.c src/effects/fire_flower.c`.
3. In each file that now calls nd_val_* or nd_xf_*, after its last #include line add `#include "nd_val.h"` and
   `#include "nd_xf.h"` (skip a line that is already there).
4. Commit: `CODEMODB3: codemod batch of 10 effect files (76 value calls to native records)`.
## Gate (expected values)
0. Draw proof, from the log of the base run of step 4: for each id, `fxtest: live 1` after its spawn and `fxcenb <id> tri <T>`
   with T above the `fxcenb -1 tri` value. A file without it moves out of the batch (row_TEMPLATE.md fix rules).
1. `python 3ds/tools/codemod.py --dry src/effects/drop_leaves.c src/effects/effect_46.c src/effects/effect_65.c src/effects/effect_75.c src/effects/effect_86.c src/effects/ending_decals.c src/effects/energy_in_out.c src/effects/energy_shockwave.c src/effects/falling_leaves.c src/effects/fire_flower.c`: value 0 for each file, keep, state and dl as in the table.
2. `python 3ds/tools/listcen.py --group src/effects` before edit 2 and after: the build count of the batch falls by 76.
3. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row CODEMODB3 --why "CODEMODB3"`: PASS.
4. `bench.py base codemodb3_drec` (merge-base), `bench.py run codemodb3_drec --row CODEMODB3 --why "CODEMODB3 drec gate"`, `bench.py cmp codemodb3_drec
   --row CODEMODB3`: drec.txt byte-equal.
5. Seeded fault: the seed of each file has the verdict live in its seeds table line (the Needs lines above; a probe row measured
   it with the same fxtest window before this row was written). This row runs no probe.
6. Budget 8 Azahar starts. Counter: C1 -76, C2 -76.
## Stop and end
A drec difference of the real edit, a tool fault, or a counter that does not fall: stop, commit, hand back the exact output.
Delete this row file in the completing commit.
