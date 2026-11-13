# Row CODEMODB6: codemod batch of 9 effect files (76 value calls to native records), drec gate over one fxtest window

(Written by the oracle in the mkrow.py form before the probes; it starts when rowlint shows no needs fault, i.e. every
seed below has the verdict live on main.)
(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md, row_TEMPLATE.md and what it names. Written by mkrow.py from
row_BATCHTEMPLATE.md and a seeds table; every seed has the verdict live from a probe row.)
## Facts (mkrow.py, main 351c8861e)
- 3ds/tools/codemod.py rewrites the value calls (gSPMatrix, gSPPopMatrix, gDPPipeSync, gDPSetPrimColor, gDPSetEnvColor, gSPSegment,
  gDPSetTileSize) to nd_xf_* and nd_val_*. The files (codemod --dry: value keep state dl | fxtest id | drec frame | live seed):
  - src/effects/shimmer_wave.c: 12 0 0 2 | fxtest 52 | frame 412 | seed `src/effects/shimmer_wave.c:202@412|part->unk_08|part->unk_08 + 20.0f`
  - src/effects/shiny_flare.c: 7 0 0 2 | fxtest 102 | frame 456 | seed `src/effects/shiny_flare.c:113@456|data->pos.y|data->pos.y + 20.0f`
  - src/effects/small_gold_sparkle.c: 9 0 0 3 | fxtest 67 | frame 500 | seed `src/effects/small_gold_sparkle.c:126@500|part->unk_0C|part->unk_0C + 20.0f`
  - src/effects/smoke_burst.c: 9 0 2 2 | fxtest 34 | frame 544 | seed `src/effects/smoke_burst.c:117@544|data->pos.y|data->pos.y + 20.0f`
  - src/effects/smoke_impact.c: 13 0 2 2 | fxtest 24 | frame 588 | seed `src/effects/smoke_impact.c:138@588|part->unk_08|part->unk_08 + 20.0f`
  - src/effects/snowflake.c: 7 0 0 2 | fxtest 9 | frame 632 | seed `src/effects/snowflake.c:130@632|data->pos.y|data->pos.y + 20.0f`
  - src/effects/snowman_doll.c: 7 0 0 2 | fxtest 90 | frame 676 | seed `src/effects/snowman_doll.c:302@676|data->unk_08|data->unk_08 + 20.0f`
  - src/effects/sparkles.c: 5 0 2 1 | fxtest 12 | frame 720 | seed `src/effects/sparkles.c:282@720|part->unk_08|part->unk_08 + 20.0f`
  - src/effects/squirt.c: 7 0 1 1 | fxtest 83 | frame 764 | seed `src/effects/squirt.c:179@764|sp10, 0.0f, 0.0f|sp10, 0.0f, 0.0f + 20.0f`
- fxtest runs the ids in the order of the switch, one per 44 frames from frame 400 (pm_fxtest.c:179-180). File i draws at frame
  412 + 44 i. drec=412:9:44 records those frames (DRECSTEP frame lines).
Start: `sh 3ds/tools/wt_new.sh CODEMODB6`. PM_ROW=CODEMODB6 PM_LANE=CODEMODB6 on every call.
## Touch only (places)
The value call lines and the include lines of: src/effects/shimmer_wave.c, src/effects/shiny_flare.c, src/effects/small_gold_sparkle.c, src/effects/smoke_burst.c, src/effects/smoke_impact.c, src/effects/snowflake.c, src/effects/snowman_doll.c, src/effects/sparkles.c, src/effects/squirt.c. New azt.d/CODEMODB6.json and bench.d/CODEMODB6.json.
New names: codemodb6_drec. Host link: none.
Needs: docs/handoff/row_SEEDS.md has shimmer_wave | 52 | 12 0 0 2 | 202 | `guTranslateF(sp20, part->unk_04, part->unk_08, part->unk_0C);` | `part->unk_08` | live
Needs: docs/handoff/row_SEEDS.md has shiny_flare | 102 | 7 0 0 2 | 113 | `guTranslateF(sp10, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has small_gold_sparkle | 67 | 9 0 0 3 | 126 | `guTranslateF(sp18, part->unk_08, part->unk_0C, part->unk_10);` | `part->unk_0C` | live
Needs: docs/handoff/row_SEEDS.md has smoke_burst | 34 | 9 0 2 2 | 117 | `guPositionF(sp20, 0.0f, -gCameras[gCurrentCameraID].curYaw, 0.0f, data->unk_10, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has smoke_impact | 24 | 13 0 2 2 | 138 | `guTranslateF(sp18, part->unk_04, part->unk_08, part->unk_0C);` | `part->unk_08` | live
Needs: docs/handoff/row_SEEDS.md has snowflake | 9 | 7 0 0 2 | 130 | `guTranslateF(sp18, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has snowman_doll | 90 | 7 0 0 2 | 302 | `guTranslateF(sp18, data->unk_04, data->unk_08, data->unk_0C);` | `data->unk_08` | live
Needs: docs/handoff/row_SEEDS.md has sparkles | 12 | 5 0 2 1 | 282 | `guTranslateF(sp18, part->unk_04, part->unk_08, part->unk_0C);` | `part->unk_08` | live
Needs: docs/handoff/row_SEEDS.md has squirt | 83 | 7 0 1 1 | 179 | `guTranslateF(sp10, 0.0f, 0.0f, 0.0f);` | `sp10, 0.0f, 0.0f` | live
## Exact edits
1. azt.d/CODEMODB6.json codemodb6_drec: a copy of the fxpic_89 scenario (azt_scenarios.json: first_marker and warp event `warp: mac_01 entry 0`; not the prof marker of spot_mac01_drec) with `debug`
   `warp=mac_01:0 pos=0:0:200 fxtest=52,102,67,34,24,9,90,12,83 loglevel=debug det drec=412:9:44`, until `drec f764 draws [1-9]\d* bytes \d+`,
   cap 45, deadline 40, needs_picture false. bench.d/CODEMODB6.json: codemodb6_drec, kind azahar_pic, tags azahar det drec. Commit them.
2. `python 3ds/tools/codemod.py src/effects/shimmer_wave.c src/effects/shiny_flare.c src/effects/small_gold_sparkle.c src/effects/smoke_burst.c src/effects/smoke_impact.c src/effects/snowflake.c src/effects/snowman_doll.c src/effects/sparkles.c src/effects/squirt.c`.
3. In each file that now calls nd_val_* or nd_xf_*, after its last #include line add `#include "nd_val.h"` and
   `#include "nd_xf.h"` (skip a line that is already there).
4. Commit: `CODEMODB6: codemod batch of 9 effect files (76 value calls to native records)`.
## Gate (expected values)
0. Draw proof, from the log of the base run of step 4: for each id, `fxtest: live 1` after its spawn and `fxcenb <id> tri <T>`
   with T above the `fxcenb -1 tri` value. A file without it moves out of the batch (row_TEMPLATE.md fix rules).
1. `python 3ds/tools/codemod.py --dry src/effects/shimmer_wave.c src/effects/shiny_flare.c src/effects/small_gold_sparkle.c src/effects/smoke_burst.c src/effects/smoke_impact.c src/effects/snowflake.c src/effects/snowman_doll.c src/effects/sparkles.c src/effects/squirt.c`: value 0 for each file, keep, state and dl as in the table.
2. `python 3ds/tools/listcen.py --group src/effects` before edit 2 and after: the build count of the batch falls by 76.
3. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row CODEMODB6 --why "CODEMODB6"`: PASS.
4. `bench.py base codemodb6_drec` (merge-base), `bench.py run codemodb6_drec --row CODEMODB6 --why "CODEMODB6 drec gate"`, `bench.py cmp codemodb6_drec
   --row CODEMODB6`: drec.txt byte-equal.
5. Seeded fault: the seed of each file has the verdict live in its seeds table line (the Needs lines above; a probe row measured
   it with the same fxtest window before this row was written). This row runs no probe.
6. Budget 8 Azahar starts. Counter: C1 -76, C2 -76.
## Stop and end
A drec difference of the real edit, a tool fault, or a counter that does not fall: stop, commit, hand back the exact output.
Delete this row file in the completing commit.
