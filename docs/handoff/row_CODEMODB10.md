# Row CODEMODB10: codemod batch of 9 effect files (87 value calls to native records), drec gate over one fxtest window

(Written by the oracle in the mkrow.py form before the probes; it starts when rowlint shows no needs fault, i.e. every
seed below has the verdict live on main.)
(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md, row_TEMPLATE.md and what it names. Written by mkrow.py from
row_BATCHTEMPLATE.md and a seeds table; every seed has the verdict live from a probe row.)
## Facts (mkrow.py, main 351c8861e)
- 3ds/tools/codemod.py rewrites the value calls (gSPMatrix, gSPPopMatrix, gDPPipeSync, gDPSetPrimColor, gDPSetEnvColor, gSPSegment,
  gDPSetTileSize) to nd_xf_* and nd_val_*. The files (codemod --dry: value keep state dl | fxtest id | drec frame | live seed):
  - src/effects/firework.c: 11 1 1 1 | fxtest 61 | frame 412 | seed `src/effects/firework.c:203@412|part->unk_08|part->unk_08 + 20.0f`
  - src/effects/got_item_outline.c: 11 0 5 2 | fxtest 22 | frame 456 | seed `src/effects/got_item_outline.c:132@456|data->pos.y|data->pos.y + 20.0f`
  - src/effects/lightning.c: 13 0 0 2 | fxtest 48 | frame 500 | seed `src/effects/lightning.c:259@500|data->unk_08|data->unk_08 + 20.0f`
  - src/effects/lightning_bolt.c: 6 0 2 1 | fxtest 88 | frame 544 | seed `src/effects/lightning_bolt.c:212@544|sp10, 0.1f, 0.1f|sp10, 0.1f, 0.1f + 20.0f`
  - src/effects/misc_particles.c: 8 0 0 2 | fxtest 107 | frame 588 | seed `src/effects/misc_particles.c:318@588|particle->pos.y|particle->pos.y + 20.0f`
  - src/effects/recover.c: 10 0 4 10 | fxtest 58 | frame 632 | seed `src/effects/recover.c:180@632|part->pos.y|part->pos.y + 20.0f`
  - src/effects/rising_bubble.c: 9 0 0 4 | fxtest 30 | frame 676 | seed `src/effects/rising_bubble.c:132@676|data->pos.y|data->pos.y + 20.0f`
  - src/effects/shattering_stones.c: 7 0 0 3 | fxtest 17 | frame 720 | seed `src/effects/shattering_stones.c:154@720|part->unk_04|part->unk_04 + 20.0f`
  - src/effects/shockwave.c: 12 0 6 2 | fxtest 32 | frame 764 | seed `src/effects/shockwave.c:327@764|data->pos.y|data->pos.y + 20.0f`
- fxtest runs the ids in the order of the switch, one per 44 frames from frame 400 (pm_fxtest.c:179-180). File i draws at frame
  412 + 44 i. drec=412:9:44 records those frames (DRECSTEP frame lines).
Start: `sh 3ds/tools/wt_new.sh CODEMODB10`. PM_ROW=CODEMODB10 PM_LANE=CODEMODB10 on every call.
## Touch only (places)
The value call lines and the include lines of: src/effects/firework.c, src/effects/got_item_outline.c, src/effects/lightning.c, src/effects/lightning_bolt.c, src/effects/misc_particles.c, src/effects/recover.c, src/effects/rising_bubble.c, src/effects/shattering_stones.c, src/effects/shockwave.c. New azt.d/CODEMODB10.json and bench.d/CODEMODB10.json.
New names: codemodb10_drec. Host link: none.
Needs: docs/handoff/row_SEEDS.md has firework | 61 | 11 1 1 1 | 203 | `guPositionF(sp20, 0.0f, -gCameras[gCurrentCameraID].curYaw, 0.0f, unk_28, part->unk_04, part->unk_08, part->unk_0C);` | `part->unk_08` | live
Needs: docs/handoff/row_SEEDS.md has got_item_outline | 22 | 11 0 5 2 | 132 | `guTranslateF(mtxTransform, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has lightning | 48 | 13 0 0 2 | 259 | `guTranslateF(sp20, data->unk_04, data->unk_08, -500.0f);` | `data->unk_08` | live
Needs: docs/handoff/row_SEEDS.md has lightning_bolt | 88 | 6 0 2 1 | 212 | `guScaleF(sp10, 0.1f, 0.1f, 0.1f);` | `sp10, 0.1f, 0.1f` | live
Needs: docs/handoff/row_SEEDS.md has misc_particles | 107 | 8 0 0 2 | 318 | `guPositionF(mtxTransform, 0.0f, -gCameras[gCurrentCameraID].curYaw, 0.0f, particle->scale * alphaScale, particle->pos.x, particle->pos.y, particle->pos.z);` | `particle->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has recover | 58 | 10 0 4 10 | 180 | `guTranslateF(sp18, part->pos.x, part->pos.y, part->pos.z);` | `part->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has rising_bubble | 30 | 9 0 0 4 | 132 | `guTranslateF(sp20, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has shattering_stones | 17 | 7 0 0 3 | 154 | `guTranslateF(sp20, part->unk_00, part->unk_04, part->unk_08);` | `part->unk_04` | live
Needs: docs/handoff/row_SEEDS.md has shockwave | 32 | 12 0 6 2 | 327 | `guTranslateF(sp20, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | live
## Exact edits
1. azt.d/CODEMODB10.json codemodb10_drec: a copy of the fxpic_89 scenario (azt_scenarios.json: first_marker and warp event `warp: mac_01 entry 0`; not the prof marker of spot_mac01_drec) with `debug`
   `warp=mac_01:0 pos=0:0:200 fxtest=61,22,48,88,107,58,30,17,32 loglevel=debug det drec=412:9:44`, until `drec f764 draws [1-9]\d* bytes \d+`,
   cap 45, deadline 40, needs_picture false. bench.d/CODEMODB10.json: codemodb10_drec, kind azahar_pic, tags azahar det drec. Commit them.
2. `python 3ds/tools/codemod.py src/effects/firework.c src/effects/got_item_outline.c src/effects/lightning.c src/effects/lightning_bolt.c src/effects/misc_particles.c src/effects/recover.c src/effects/rising_bubble.c src/effects/shattering_stones.c src/effects/shockwave.c`.
3. In each file that now calls nd_val_* or nd_xf_*, after its last #include line add `#include "nd_val.h"` and
   `#include "nd_xf.h"` (skip a line that is already there).
4. Commit: `CODEMODB10: codemod batch of 9 effect files (87 value calls to native records)`.
## Gate (expected values)
0. Draw proof, from the log of the base run of step 4: for each id, `fxtest: live 1` after its spawn and `fxcenb <id> tri <T>`
   with T above the `fxcenb -1 tri` value. A file without it moves out of the batch (row_TEMPLATE.md fix rules).
1. `python 3ds/tools/codemod.py --dry src/effects/firework.c src/effects/got_item_outline.c src/effects/lightning.c src/effects/lightning_bolt.c src/effects/misc_particles.c src/effects/recover.c src/effects/rising_bubble.c src/effects/shattering_stones.c src/effects/shockwave.c`: value 0 for each file, keep, state and dl as in the table.
2. `python 3ds/tools/listcen.py --group src/effects` before edit 2 and after: the build count of the batch falls by 87.
3. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row CODEMODB10 --why "CODEMODB10"`: PASS.
4. `bench.py base codemodb10_drec` (merge-base), `bench.py run codemodb10_drec --row CODEMODB10 --why "CODEMODB10 drec gate"`, `bench.py cmp codemodb10_drec
   --row CODEMODB10`: drec.txt byte-equal.
5. Seeded fault: the seed of each file has the verdict live in its seeds table line (the Needs lines above; a probe row measured
   it with the same fxtest window before this row was written). This row runs no probe.
6. Budget 8 Azahar starts. Counter: C1 -87, C2 -87.
## Stop and end
A drec difference of the real edit, a tool fault, or a counter that does not fall: stop, commit, hand back the exact output.
Delete this row file in the completing commit.
