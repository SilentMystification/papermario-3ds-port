# Row CODEMODB7: codemod batch of 9 effect files (73 value calls to native records), drec gate over one fxtest window

(Written by the oracle in the mkrow.py form before the probes; it starts when rowlint shows no needs fault, i.e. every
seed below has the verdict live on main.)
(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md, row_TEMPLATE.md and what it names. Written by mkrow.py from
row_BATCHTEMPLATE.md and a seeds table; every seed has the verdict live from a probe row.)
## Facts (mkrow.py, main 351c8861e)
- 3ds/tools/codemod.py rewrites the value calls (gSPMatrix, gSPPopMatrix, gDPPipeSync, gDPSetPrimColor, gDPSetEnvColor, gSPSegment,
  gDPSetTileSize) to nd_xf_* and nd_val_*. The files (codemod --dry: value keep state dl | fxtest id | drec frame | live seed):
  - src/effects/stars_shimmer.c: 6 0 1 2 | fxtest 29 | frame 412 | seed `src/effects/stars_shimmer.c:299@412|data->pos.y|data->pos.y + 20.0f`
  - src/effects/stars_spread.c: 7 0 0 2 | fxtest 39 | frame 456 | seed `src/effects/stars_spread.c:134@456|part->unk_08|part->unk_08 + 20.0f`
  - src/effects/static_status.c: 8 0 0 2 | fxtest 108 | frame 500 | seed `src/effects/static_status.c:204@500|part->pos.y|part->pos.y + 20.0f`
  - src/effects/steam_burst.c: 11 0 0 2 | fxtest 40 | frame 544 | seed `src/effects/steam_burst.c:127@544|part->unk_08|part->unk_08 + 20.0f`
  - src/effects/stop_watch.c: 11 0 1 1 | fxtest 92 | frame 588 | seed `src/effects/stop_watch.c:196@588|data->unk_08|data->unk_08 + 20.0f`
  - src/effects/sweat.c: 4 0 0 2 | fxtest 35 | frame 632 | seed `src/effects/sweat.c:92@632|data->pos.y|data->pos.y + 20.0f`
  - src/effects/throw_spiny.c: 6 0 0 2 | fxtest 94 | frame 676 | seed `src/effects/throw_spiny.c:172@676|data->pos.y|data->pos.y + 20.0f`
  - src/effects/thunderbolt_ring.c: 11 0 0 2 | fxtest 82 | frame 720 | seed `src/effects/thunderbolt_ring.c:115@720|data->pos.y|data->pos.y + 20.0f`
  - src/effects/tubba_heart_attack.c: 9 0 0 2 | fxtest 96 | frame 764 | seed `src/effects/tubba_heart_attack.c:318@764|data->centerY|data->centerY + 20.0f`
- fxtest runs the ids in the order of the switch, one per 44 frames from frame 400 (pm_fxtest.c:179-180). File i draws at frame
  412 + 44 i. drec=412:9:44 records those frames (DRECSTEP frame lines).
Start: `sh 3ds/tools/wt_new.sh CODEMODB7`. PM_ROW=CODEMODB7 PM_LANE=CODEMODB7 on every call.
## Touch only (places)
The value call lines and the include lines of: src/effects/stars_shimmer.c, src/effects/stars_spread.c, src/effects/static_status.c, src/effects/steam_burst.c, src/effects/stop_watch.c, src/effects/sweat.c, src/effects/throw_spiny.c, src/effects/thunderbolt_ring.c, src/effects/tubba_heart_attack.c. New azt.d/CODEMODB7.json and bench.d/CODEMODB7.json.
New names: codemodb7_drec. Host link: none.
Needs: docs/handoff/row_SEEDS.md has stars_shimmer | 29 | 6 0 1 2 | 299 | `guTranslateF(sp18, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has stars_spread | 39 | 7 0 0 2 | 134 | `guTranslateF(sp18, part->unk_04, part->unk_08, part->unk_0C);` | `part->unk_08` | live
Needs: docs/handoff/row_SEEDS.md has static_status | 108 | 8 0 0 2 | 204 | `guTranslateF(mtxTransform, part->pos.x, part->pos.y, part->pos.z);` | `part->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has steam_burst | 40 | 11 0 0 2 | 127 | `guTranslateF(sp18, part->unk_04, part->unk_08, part->unk_0C);` | `part->unk_08` | live
Needs: docs/handoff/row_SEEDS.md has stop_watch | 92 | 11 0 1 1 | 196 | `guTranslateF(sp20, data->unk_04, data->unk_08, data->unk_0C);` | `data->unk_08` | live
Needs: docs/handoff/row_SEEDS.md has sweat | 35 | 4 0 0 2 | 92 | `guTranslateF(sp18, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has throw_spiny | 94 | 6 0 0 2 | 172 | `guTranslateF(sp18, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has thunderbolt_ring | 82 | 11 0 0 2 | 115 | `guPositionF(sp20, 0.0f, -gCameras[gCurrentCameraID].curYaw, 0.0f, data->unk_28, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has tubba_heart_attack | 96 | 9 0 0 2 | 318 | `guTranslateF(sp18, data->centerX, data->centerY, data->centerZ);` | `data->centerY` | live
## Exact edits
1. azt.d/CODEMODB7.json codemodb7_drec: a copy of the fxpic_89 scenario (azt_scenarios.json: first_marker and warp event `warp: mac_01 entry 0`; not the prof marker of spot_mac01_drec) with `debug`
   `warp=mac_01:0 pos=0:0:200 fxtest=29,39,108,40,92,35,94,82,96 loglevel=debug det drec=412:9:44`, until `drec f764 draws [1-9]\d* bytes \d+`,
   cap 45, deadline 40, needs_picture false. bench.d/CODEMODB7.json: codemodb7_drec, kind azahar_pic, tags azahar det drec. Commit them.
2. `python 3ds/tools/codemod.py src/effects/stars_shimmer.c src/effects/stars_spread.c src/effects/static_status.c src/effects/steam_burst.c src/effects/stop_watch.c src/effects/sweat.c src/effects/throw_spiny.c src/effects/thunderbolt_ring.c src/effects/tubba_heart_attack.c`.
3. In each file that now calls nd_val_* or nd_xf_*, after its last #include line add `#include "nd_val.h"` and
   `#include "nd_xf.h"` (skip a line that is already there).
4. Commit: `CODEMODB7: codemod batch of 9 effect files (73 value calls to native records)`.
## Gate (expected values)
0. Draw proof, from the log of the base run of step 4: for each id, `fxtest: live 1` after its spawn and `fxcenb <id> tri <T>`
   with T above the `fxcenb -1 tri` value. A file without it moves out of the batch (row_TEMPLATE.md fix rules).
1. `python 3ds/tools/codemod.py --dry src/effects/stars_shimmer.c src/effects/stars_spread.c src/effects/static_status.c src/effects/steam_burst.c src/effects/stop_watch.c src/effects/sweat.c src/effects/throw_spiny.c src/effects/thunderbolt_ring.c src/effects/tubba_heart_attack.c`: value 0 for each file, keep, state and dl as in the table.
2. `python 3ds/tools/listcen.py --group src/effects` before edit 2 and after: the build count of the batch falls by 73.
3. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row CODEMODB7 --why "CODEMODB7"`: PASS.
4. `bench.py base codemodb7_drec` (merge-base), `bench.py run codemodb7_drec --row CODEMODB7 --why "CODEMODB7 drec gate"`, `bench.py cmp codemodb7_drec
   --row CODEMODB7`: drec.txt byte-equal.
5. Seeded fault: the seed of each file has the verdict live in its seeds table line (the Needs lines above; a probe row measured
   it with the same fxtest window before this row was written). This row runs no probe.
6. Budget 8 Azahar starts. Counter: C1 -73, C2 -73.
## Stop and end
A drec difference of the real edit, a tool fault, or a counter that does not fall: stop, commit, hand back the exact output.
Delete this row file in the completing commit.
