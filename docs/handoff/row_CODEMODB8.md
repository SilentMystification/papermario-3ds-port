# Row CODEMODB8: codemod batch of 9 effect files (90 value calls to native records), drec gate over one fxtest window

(Written by the oracle in the mkrow.py form before the probes; it starts when rowlint shows no needs fault, i.e. every
seed below has the verdict live on main.)
(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md, row_TEMPLATE.md and what it names. Written by mkrow.py from
row_BATCHTEMPLATE.md and a seeds table; every seed has the verdict live from a probe row.)
## Facts (mkrow.py, main 351c8861e)
- 3ds/tools/codemod.py rewrites the value calls (gSPMatrix, gSPPopMatrix, gDPPipeSync, gDPSetPrimColor, gDPSetEnvColor, gSPSegment,
  gDPSetTileSize) to nd_xf_* and nd_val_*. The files (codemod --dry: value keep state dl | fxtest id | drec frame | live seed):
  - src/effects/waterfall.c: 6 0 1 1 | fxtest 85 | frame 412 | seed `src/effects/waterfall.c:136@412|data->unk_08|data->unk_08 + 20.0f`
  - src/effects/energy_orb_wave.c: 10 0 5 5 | fxtest 76 | frame 456 | seed `src/effects/energy_orb_wave.c:269@456|data->pos.y|data->pos.y + 20.0f`
  - src/effects/fire_breath.c: 10 0 0 4 | fxtest 49 | frame 500 | seed `src/effects/fire_breath.c:214@500|data->pos.y|data->pos.y + 20.0f`
  - src/effects/flame.c: 8 0 4 4 | fxtest 27 | frame 544 | seed `src/effects/flame.c:203@544|data->pos.y|data->pos.y + 20.0f`
  - src/effects/radial_shimmer.c: 11 0 0 2 | fxtest 45 | frame 588 | seed `src/effects/radial_shimmer.c:328@588|data->unk_14|data->unk_14 + 20.0f`
  - src/effects/shape_spell.c: 10 0 1 1 | fxtest 13 | frame 632 | seed `src/effects/shape_spell.c:251@632|data->pos.y|data->pos.y + 20.0f`
  - src/effects/smoke_ring.c: 12 0 0 2 | fxtest 18 | frame 676 | seed `src/effects/smoke_ring.c:144@676|part->unk_08|part->unk_08 + 20.0f`
  - src/effects/star_outline.c: 10 0 6 4 | fxtest 125 | frame 720 | seed `src/effects/star_outline.c:234@720|data->pos.y|data->pos.y + 20.0f`
  - src/effects/water_fountain.c: 13 0 0 3 | fxtest 86 | frame 764 | seed `src/effects/water_fountain.c:241@764|data->pos.y|data->pos.y + 20.0f`
- fxtest runs the ids in the order of the switch, one per 44 frames from frame 400 (pm_fxtest.c:179-180). File i draws at frame
  412 + 44 i. drec=412:9:44 records those frames (DRECSTEP frame lines).
Start: `sh 3ds/tools/wt_new.sh CODEMODB8`. PM_ROW=CODEMODB8 PM_LANE=CODEMODB8 on every call.
## Touch only (places)
The value call lines and the include lines of: src/effects/waterfall.c, src/effects/energy_orb_wave.c, src/effects/fire_breath.c, src/effects/flame.c, src/effects/radial_shimmer.c, src/effects/shape_spell.c, src/effects/smoke_ring.c, src/effects/star_outline.c, src/effects/water_fountain.c. New azt.d/CODEMODB8.json and bench.d/CODEMODB8.json.
New names: codemodb8_drec. Host link: none.
Needs: docs/handoff/row_SEEDS.md has waterfall | 85 | 6 0 1 1 | 136 | `guTranslateF(sp10, data->unk_04, data->unk_08, data->unk_0C);` | `data->unk_08` | live
Needs: docs/handoff/row_SEEDS.md has energy_orb_wave | 76 | 10 0 5 5 | 269 | `guTranslateF(sp18, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has fire_breath | 49 | 10 0 0 4 | 214 | `guTranslateF(transformMtx, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has flame | 27 | 8 0 4 4 | 203 | `guTranslateF(sp18, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has radial_shimmer | 45 | 11 0 0 2 | 328 | `guTranslateF(sp20, data->unk_10, data->unk_14, data->unk_18);` | `data->unk_14` | live
Needs: docs/handoff/row_SEEDS.md has shape_spell | 13 | 10 0 1 1 | 251 | `guPositionF(mL, 0.0f, yawOut, 0.0f, 1.0f, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has smoke_ring | 18 | 12 0 0 2 | 144 | `guTranslateF(sp18, part->unk_04, part->unk_08, part->unk_0C);` | `part->unk_08` | live
Needs: docs/handoff/row_SEEDS.md has star_outline | 125 | 10 0 6 4 | 234 | `guPositionF(sp20, 0.0f, -gCameras[gCurrentCameraID].curYaw, 0.0f, data->unk_38 * 0.4, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has water_fountain | 86 | 13 0 0 3 | 241 | `guTranslateF(sp18, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | live
## Exact edits
1. azt.d/CODEMODB8.json codemodb8_drec: a copy of the fxpic_89 scenario (azt_scenarios.json: first_marker and warp event `warp: mac_01 entry 0`; not the prof marker of spot_mac01_drec) with `debug`
   `warp=mac_01:0 pos=0:0:200 fxtest=85,76,49,27,45,13,18,125,86 loglevel=debug det drec=412:9:44`, until `drec f764 draws [1-9]\d* bytes \d+`,
   cap 45, deadline 40, needs_picture false. bench.d/CODEMODB8.json: codemodb8_drec, kind azahar_pic, tags azahar det drec. Commit them.
2. `python 3ds/tools/codemod.py src/effects/waterfall.c src/effects/energy_orb_wave.c src/effects/fire_breath.c src/effects/flame.c src/effects/radial_shimmer.c src/effects/shape_spell.c src/effects/smoke_ring.c src/effects/star_outline.c src/effects/water_fountain.c`.
3. In each file that now calls nd_val_* or nd_xf_*, after its last #include line add `#include "nd_val.h"` and
   `#include "nd_xf.h"` (skip a line that is already there).
4. Commit: `CODEMODB8: codemod batch of 9 effect files (90 value calls to native records)`.
## Gate (expected values)
0. Draw proof, from the log of the base run of step 4: for each id, `fxtest: live 1` after its spawn and `fxcenb <id> tri <T>`
   with T above the `fxcenb -1 tri` value. A file without it moves out of the batch (row_TEMPLATE.md fix rules).
1. `python 3ds/tools/codemod.py --dry src/effects/waterfall.c src/effects/energy_orb_wave.c src/effects/fire_breath.c src/effects/flame.c src/effects/radial_shimmer.c src/effects/shape_spell.c src/effects/smoke_ring.c src/effects/star_outline.c src/effects/water_fountain.c`: value 0 for each file, keep, state and dl as in the table.
2. `python 3ds/tools/listcen.py --group src/effects` before edit 2 and after: the build count of the batch falls by 90.
3. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row CODEMODB8 --why "CODEMODB8"`: PASS.
4. `bench.py base codemodb8_drec` (merge-base), `bench.py run codemodb8_drec --row CODEMODB8 --why "CODEMODB8 drec gate"`, `bench.py cmp codemodb8_drec
   --row CODEMODB8`: drec.txt byte-equal.
5. Seeded fault: the seed of each file has the verdict live in its seeds table line (the Needs lines above; a probe row measured
   it with the same fxtest window before this row was written). This row runs no probe.
6. Budget 8 Azahar starts. Counter: C1 -90, C2 -90.
## Stop and end
A drec difference of the real edit, a tool fault, or a counter that does not fall: stop, commit, hand back the exact output.
Delete this row file in the completing commit.
