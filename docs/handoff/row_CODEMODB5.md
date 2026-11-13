# Row CODEMODB5: codemod batch of 10 effect files (87 value calls to native records), drec gate over one fxtest window

(Written by the oracle in the mkrow.py form before the probes; it starts when rowlint shows no needs fault, i.e. every
seed below has the verdict live on main.)
(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md, row_TEMPLATE.md and what it names. Written by mkrow.py from
row_BATCHTEMPLATE.md and a seeds table; every seed has the verdict live from a probe row.)
## Facts (mkrow.py, main 351c8861e)
- 3ds/tools/codemod.py rewrites the value calls (gSPMatrix, gSPPopMatrix, gDPPipeSync, gDPSetPrimColor, gDPSetEnvColor, gSPSegment,
  gDPSetTileSize) to nd_xf_* and nd_val_*. The files (codemod --dry: value keep state dl | fxtest id | drec frame | live seed):
  - src/effects/ice_pillar.c: 7 0 0 2 | fxtest 121 | frame 412 | seed `src/effects/ice_pillar.c:165@412|data->pos.y|data->pos.y + 20.0f`
  - src/effects/ice_shard.c: 11 0 1 3 | fxtest 114 | frame 456 | seed `src/effects/ice_shard.c:147@456|data->pos.y|data->pos.y + 20.0f`
  - src/effects/light_rays.c: 9 0 0 2 | fxtest 47 | frame 500 | seed `src/effects/light_rays.c:295@500|part->pos.y|part->pos.y + 20.0f`
  - src/effects/merlin_house_stars.c: 9 0 1 2 | fxtest 77 | frame 544 | seed `src/effects/merlin_house_stars.c:144@544|data->unk_10|data->unk_10 + 20.0f`
  - src/effects/moving_cloud.c: 7 0 0 2 | fxtest 109 | frame 588 | seed `src/effects/moving_cloud.c:190@588|data->unk_08|data->unk_08 + 20.0f`
  - src/effects/purple_ring.c: 7 0 0 2 | fxtest 26 | frame 632 | seed `src/effects/purple_ring.c:204@632|data->unk_08|data->unk_08 + 20.0f`
  - src/effects/quizmo_assistant.c: 6 0 0 2 | fxtest 120 | frame 676 | seed `src/effects/quizmo_assistant.c:112@676|data->pos.y|data->pos.y + 20.0f`
  - src/effects/quizmo_audience.c: 8 0 0 2 | fxtest 78 | frame 720 | seed `src/effects/quizmo_audience.c:194@720|data->pos.y|data->pos.y + 20.0f`
  - src/effects/red_impact.c: 12 0 4 2 | fxtest 98 | frame 764 | seed `src/effects/red_impact.c:175@764|part->unk_08|part->unk_08 + 20.0f`
  - src/effects/ring_blast.c: 11 0 4 2 | fxtest 31 | frame 808 | seed `src/effects/ring_blast.c:102@808|data->pos.y|data->pos.y + 20.0f`
- fxtest runs the ids in the order of the switch, one per 44 frames from frame 400 (pm_fxtest.c:179-180). File i draws at frame
  412 + 44 i. drec=412:10:44 records those frames (DRECSTEP frame lines).
Start: `sh 3ds/tools/wt_new.sh CODEMODB5`. PM_ROW=CODEMODB5 PM_LANE=CODEMODB5 on every call.
## Touch only (places)
The value call lines and the include lines of: src/effects/ice_pillar.c, src/effects/ice_shard.c, src/effects/light_rays.c, src/effects/merlin_house_stars.c, src/effects/moving_cloud.c, src/effects/purple_ring.c, src/effects/quizmo_assistant.c, src/effects/quizmo_audience.c, src/effects/red_impact.c, src/effects/ring_blast.c. New azt.d/CODEMODB5.json and bench.d/CODEMODB5.json.
New names: codemodb5_drec. Host link: none.
Needs: docs/handoff/row_SEEDS.md has ice_pillar | 121 | 7 0 0 2 | 165 | `guTranslateF(sp10, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has ice_shard | 114 | 11 0 1 3 | 147 | `guPositionF(sp20, 0.0f, -gCameras[gCurrentCameraID].curYaw, 0.0f, data->scale, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has light_rays | 47 | 9 0 0 2 | 295 | `guTranslateF(mtxTranslate, part->pos.x, part->pos.y, part->pos.z);` | `part->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has merlin_house_stars | 77 | 9 0 1 2 | 144 | `guTranslateF(sp10, data->unk_0C, data->unk_10, data->unk_14);` | `data->unk_10` | live
Needs: docs/handoff/row_SEEDS.md has moving_cloud | 109 | 7 0 0 2 | 190 | `guTranslateF(sp10, data->unk_04, data->unk_08, data->unk_0C);` | `data->unk_08` | live
Needs: docs/handoff/row_SEEDS.md has purple_ring | 26 | 7 0 0 2 | 204 | `guTranslateF(sp10, data->unk_04, data->unk_08, data->unk_0C);` | `data->unk_08` | live
Needs: docs/handoff/row_SEEDS.md has quizmo_assistant | 120 | 6 0 0 2 | 112 | `guTranslateF(sp18, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has quizmo_audience | 78 | 8 0 0 2 | 194 | `guTranslateF(sp18, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has red_impact | 98 | 12 0 4 2 | 175 | `guPositionF(sp20, 0.0f, -gCameras[gCurrentCameraID].curYaw, 0.0f, 1.0f, part->unk_04, part->unk_08, part->unk_0C);` | `part->unk_08` | live
Needs: docs/handoff/row_SEEDS.md has ring_blast | 31 | 11 0 4 2 | 102 | `guPositionF(sp20, 0.0f, -gCameras[gCurrentCameraID].curYaw, 0.0f, data->unk_10, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | live
## Exact edits
1. azt.d/CODEMODB5.json codemodb5_drec: a copy of the fxpic_89 scenario (azt_scenarios.json: first_marker and warp event `warp: mac_01 entry 0`; not the prof marker of spot_mac01_drec) with `debug`
   `warp=mac_01:0 pos=0:0:200 fxtest=121,114,47,77,109,26,120,78,98,31 loglevel=debug det drec=412:10:44`, until `drec f808 draws [1-9]\d* bytes \d+`,
   cap 45, deadline 40, needs_picture false. bench.d/CODEMODB5.json: codemodb5_drec, kind azahar_pic, tags azahar det drec. Commit them.
2. `python 3ds/tools/codemod.py src/effects/ice_pillar.c src/effects/ice_shard.c src/effects/light_rays.c src/effects/merlin_house_stars.c src/effects/moving_cloud.c src/effects/purple_ring.c src/effects/quizmo_assistant.c src/effects/quizmo_audience.c src/effects/red_impact.c src/effects/ring_blast.c`.
3. In each file that now calls nd_val_* or nd_xf_*, after its last #include line add `#include "nd_val.h"` and
   `#include "nd_xf.h"` (skip a line that is already there).
4. Commit: `CODEMODB5: codemod batch of 10 effect files (87 value calls to native records)`.
## Gate (expected values)
0. Draw proof, from the log of the base run of step 4: for each id, `fxtest: live 1` after its spawn and `fxcenb <id> tri <T>`
   with T above the `fxcenb -1 tri` value. A file without it moves out of the batch (row_TEMPLATE.md fix rules).
1. `python 3ds/tools/codemod.py --dry src/effects/ice_pillar.c src/effects/ice_shard.c src/effects/light_rays.c src/effects/merlin_house_stars.c src/effects/moving_cloud.c src/effects/purple_ring.c src/effects/quizmo_assistant.c src/effects/quizmo_audience.c src/effects/red_impact.c src/effects/ring_blast.c`: value 0 for each file, keep, state and dl as in the table.
2. `python 3ds/tools/listcen.py --group src/effects` before edit 2 and after: the build count of the batch falls by 87.
3. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row CODEMODB5 --why "CODEMODB5"`: PASS.
4. `bench.py base codemodb5_drec` (merge-base), `bench.py run codemodb5_drec --row CODEMODB5 --why "CODEMODB5 drec gate"`, `bench.py cmp codemodb5_drec
   --row CODEMODB5`: drec.txt byte-equal.
5. Seeded fault: the seed of each file has the verdict live in its seeds table line (the Needs lines above; a probe row measured
   it with the same fxtest window before this row was written). This row runs no probe.
6. Budget 8 Azahar starts. Counter: C1 -87, C2 -87.
## Stop and end
A drec difference of the real edit, a tool fault, or a counter that does not fall: stop, commit, hand back the exact output.
Delete this row file in the completing commit.
