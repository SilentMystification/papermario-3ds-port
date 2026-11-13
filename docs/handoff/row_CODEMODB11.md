# Row CODEMODB11: codemod batch of 9 effect files (75 value calls to native records), drec gate over one fxtest window

(Written by the oracle in the mkrow.py form before the probes; it starts when rowlint shows no needs fault, i.e. every
seed below has the verdict live on main.)
(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md, row_TEMPLATE.md and what it names. Written by mkrow.py from
row_BATCHTEMPLATE.md and a seeds table; every seed has the verdict live from a probe row.)
## Facts (mkrow.py, main 351c8861e)
- 3ds/tools/codemod.py rewrites the value calls (gSPMatrix, gSPPopMatrix, gDPPipeSync, gDPSetPrimColor, gDPSetEnvColor, gSPSegment,
  gDPSetTileSize) to nd_xf_* and nd_val_*. The files (codemod --dry: value keep state dl | fxtest id | drec frame | live seed):
  - src/effects/something_rotating.c: 10 0 0 6 | fxtest 117 | frame 412 | seed `src/effects/something_rotating.c:272@412|part->unk_1C|part->unk_1C + 20.0f`
  - src/effects/spiky_white_aura.c: 5 0 0 2 | fxtest 23 | frame 456 | seed `src/effects/spiky_white_aura.c:190@456|part->unk_08|part->unk_08 + 20.0f`
  - src/effects/spirit_card.c: 6 0 0 4 | fxtest 115 | frame 500 | seed `src/effects/spirit_card.c:143@500|data->pos.y|data->pos.y + 20.0f`
  - src/effects/star_spirits_energy.c: 12 0 6 3 | fxtest 123 | frame 544 | seed `src/effects/star_spirits_energy.c:361@544|sp20, 0.0f, 0.0f, 0.0f, data->unk_40[i], 0.0f, 0.0f|sp20, 0.0f, 0.0f, 0.0f, data->unk_40[i], 0.0f, 0.0f + 20.0f`
  - src/effects/stars_burst.c: 5 0 0 2 | fxtest 28 | frame 588 | seed `src/effects/stars_burst.c:158@588|part->unk_08|part->unk_08 + 20.0f`
  - src/effects/stars_orbiting.c: 7 0 0 2 | fxtest 41 | frame 632 | seed `src/effects/stars_orbiting.c:120@632|part->pos.y|part->pos.y + 20.0f`
  - src/effects/stat_change.c: 10 0 4 8 | fxtest 80 | frame 676 | seed `src/effects/stat_change.c:249@676|data->pos.y|data->pos.y + 20.0f`
  - src/effects/sun.c: 10 0 0 2 | fxtest 122 | frame 720 | seed `src/effects/sun.c:176@720|mtx, 0.0f, 0.0f|mtx, 0.0f, 0.0f + 20.0f`
  - src/effects/walking_dust.c: 10 0 0 2 | fxtest 2 | frame 764 | seed `src/effects/walking_dust.c:157@764|data->unk_0C|data->unk_0C + 20.0f`
- fxtest runs the ids in the order of the switch, one per 44 frames from frame 400 (pm_fxtest.c:179-180). File i draws at frame
  412 + 44 i. drec=412:9:44 records those frames (DRECSTEP frame lines).
Start: `sh 3ds/tools/wt_new.sh CODEMODB11`. PM_ROW=CODEMODB11 PM_LANE=CODEMODB11 on every call.
## Touch only (places)
The value call lines and the include lines of: src/effects/something_rotating.c, src/effects/spiky_white_aura.c, src/effects/spirit_card.c, src/effects/star_spirits_energy.c, src/effects/stars_burst.c, src/effects/stars_orbiting.c, src/effects/stat_change.c, src/effects/sun.c, src/effects/walking_dust.c. New azt.d/CODEMODB11.json and bench.d/CODEMODB11.json.
New names: codemodb11_drec. Host link: none.
Needs: docs/handoff/row_SEEDS.md has something_rotating | 117 | 10 0 0 6 | 272 | `guRotateF(sp60, part->unk_1C, 0.0f, 0.0f, 1.0f);` | `part->unk_1C` | live
Needs: docs/handoff/row_SEEDS.md has spiky_white_aura | 23 | 5 0 0 2 | 190 | `guTranslateF(sp18, part->unk_04, part->unk_08, part->unk_0C);` | `part->unk_08` | live
Needs: docs/handoff/row_SEEDS.md has spirit_card | 115 | 6 0 0 4 | 143 | `guPositionF(sp20, 0.0f, data->yaw - temp, 0.0f, data->unk_18, data->pos.x + 2.0f, data->pos.y, data->pos.z + 2.0f);` | `data->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has star_spirits_energy | 123 | 12 0 6 3 | 361 | `guPositionF(sp20, 0.0f, 0.0f, 0.0f, data->unk_40[i], 0.0f, 0.0f, 0.0f);` | `sp20, 0.0f, 0.0f, 0.0f, data->unk_40[i], 0.0f, 0.0f` | live
Needs: docs/handoff/row_SEEDS.md has stars_burst | 28 | 5 0 0 2 | 158 | `guTranslateF(sp18, part->unk_04, part->unk_08, part->unk_0C);` | `part->unk_08` | live
Needs: docs/handoff/row_SEEDS.md has stars_orbiting | 41 | 7 0 0 2 | 120 | `guTranslateF(sp18, part->pos.x, part->pos.y, part->pos.z);` | `part->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has stat_change | 80 | 10 0 4 8 | 249 | `guPositionF(sp20, 0.0f, -gCameras[gCurrentCameraID].curYaw, 0.0f, data->scale, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has sun | 122 | 10 0 0 2 | 176 | `guTranslateF(mtx, 0.0f, 0.0f, 0.0f);` | `mtx, 0.0f, 0.0f` | live
Needs: docs/handoff/row_SEEDS.md has walking_dust | 2 | 10 0 0 2 | 157 | `guTranslateF(sp18, data->unk_08, data->unk_0C, data->unk_10);` | `data->unk_0C` | live
## Exact edits
1. azt.d/CODEMODB11.json codemodb11_drec: a copy of the fxpic_89 scenario (azt_scenarios.json: first_marker and warp event `warp: mac_01 entry 0`; not the prof marker of spot_mac01_drec) with `debug`
   `warp=mac_01:0 pos=0:0:200 fxtest=117,23,115,123,28,41,80,122,2 loglevel=debug det drec=412:9:44`, until `drec f764 draws [1-9]\d* bytes \d+`,
   cap 45, deadline 40, needs_picture false. bench.d/CODEMODB11.json: codemodb11_drec, kind azahar_pic, tags azahar det drec. Commit them.
2. `python 3ds/tools/codemod.py src/effects/something_rotating.c src/effects/spiky_white_aura.c src/effects/spirit_card.c src/effects/star_spirits_energy.c src/effects/stars_burst.c src/effects/stars_orbiting.c src/effects/stat_change.c src/effects/sun.c src/effects/walking_dust.c`.
3. In each file that now calls nd_val_* or nd_xf_*, after its last #include line add `#include "nd_val.h"` and
   `#include "nd_xf.h"` (skip a line that is already there).
4. Commit: `CODEMODB11: codemod batch of 9 effect files (75 value calls to native records)`.
## Gate (expected values)
0. Draw proof, from the log of the base run of step 4: for each id, `fxtest: live 1` after its spawn and `fxcenb <id> tri <T>`
   with T above the `fxcenb -1 tri` value. A file without it moves out of the batch (row_TEMPLATE.md fix rules).
1. `python 3ds/tools/codemod.py --dry src/effects/something_rotating.c src/effects/spiky_white_aura.c src/effects/spirit_card.c src/effects/star_spirits_energy.c src/effects/stars_burst.c src/effects/stars_orbiting.c src/effects/stat_change.c src/effects/sun.c src/effects/walking_dust.c`: value 0 for each file, keep, state and dl as in the table.
2. `python 3ds/tools/listcen.py --group src/effects` before edit 2 and after: the build count of the batch falls by 75.
3. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row CODEMODB11 --why "CODEMODB11"`: PASS.
4. `bench.py base codemodb11_drec` (merge-base), `bench.py run codemodb11_drec --row CODEMODB11 --why "CODEMODB11 drec gate"`, `bench.py cmp codemodb11_drec
   --row CODEMODB11`: drec.txt byte-equal.
5. Seeded fault: the seed of each file has the verdict live in its seeds table line (the Needs lines above; a probe row measured
   it with the same fxtest window before this row was written). This row runs no probe.
6. Budget 8 Azahar starts. Counter: C1 -75, C2 -75.
## Stop and end
A drec difference of the real edit, a tool fault, or a counter that does not fall: stop, commit, hand back the exact output.
Delete this row file in the completing commit.
