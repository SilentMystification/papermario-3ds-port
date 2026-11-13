# Row CODEMODB9: codemod batch of 9 effect files (74 value calls to native records), drec gate over one fxtest window

(Written by the oracle in the mkrow.py form before the probes; it starts when rowlint shows no needs fault, i.e. every
seed below has the verdict live on main.)
(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md, row_TEMPLATE.md and what it names. Written by mkrow.py from
row_BATCHTEMPLATE.md and a seeds table; every seed has the verdict live from a probe row.)
## Facts (mkrow.py, main 351c8861e)
- 3ds/tools/codemod.py rewrites the value calls (gSPMatrix, gSPPopMatrix, gDPPipeSync, gDPSetPrimColor, gDPSetEnvColor, gSPSegment,
  gDPSetTileSize) to nd_xf_* and nd_val_*. The files (codemod --dry: value keep state dl | fxtest id | drec frame | live seed):
  - src/effects/windy_leaves.c: 4 0 0 2 | fxtest 37 | frame 412 | seed `src/effects/windy_leaves.c:248@412|part->unk_04.y|part->unk_04.y + 20.0f`
  - src/effects/attack_result_text.c: 9 0 4 5 | fxtest 66 | frame 456 | seed `src/effects/attack_result_text.c:196@456|data->pos.y|data->pos.y + 20.0f`
  - src/effects/breaking_junk.c: 7 0 0 2 | fxtest 118 | frame 500 | seed `src/effects/breaking_junk.c:146@500|data->pos.y|data->pos.y + 20.0f`
  - src/effects/cloud_puff.c: 6 0 0 2 | fxtest 5 | frame 544 | seed `src/effects/cloud_puff.c:131@544|part->unk_1C|part->unk_1C + 20.0f`
  - src/effects/cloud_trail.c: 6 0 0 2 | fxtest 6 | frame 588 | seed `src/effects/cloud_trail.c:132@588|part->unk_20|part->unk_20 + 20.0f`
  - src/effects/damage_stars.c: 7 0 0 2 | fxtest 19 | frame 632 | seed `src/effects/damage_stars.c:257@632|part->pos.y|part->pos.y + 20.0f`
  - src/effects/disable_x.c: 13 0 4 11 | fxtest 59 | frame 676 | seed `src/effects/disable_x.c:219@676|data->pos.y|data->pos.y + 20.0f`
  - src/effects/effect_63.c: 9 0 0 2 | fxtest 93 | frame 720 | seed `src/effects/effect_63.c:250@720|part->unk_08|part->unk_08 + 20.0f`
  - src/effects/emote.c: 13 0 0 4 | fxtest 11 | frame 764 | seed `src/effects/emote.c:225@764|part->unk_08|part->unk_08 + 20.0f`
- fxtest runs the ids in the order of the switch, one per 44 frames from frame 400 (pm_fxtest.c:179-180). File i draws at frame
  412 + 44 i. drec=412:9:44 records those frames (DRECSTEP frame lines).
Start: `sh 3ds/tools/wt_new.sh CODEMODB9`. PM_ROW=CODEMODB9 PM_LANE=CODEMODB9 on every call.
## Touch only (places)
The value call lines and the include lines of: src/effects/windy_leaves.c, src/effects/attack_result_text.c, src/effects/breaking_junk.c, src/effects/cloud_puff.c, src/effects/cloud_trail.c, src/effects/damage_stars.c, src/effects/disable_x.c, src/effects/effect_63.c, src/effects/emote.c. New azt.d/CODEMODB9.json and bench.d/CODEMODB9.json.
New names: codemodb9_drec. Host link: none.
Needs: docs/handoff/row_SEEDS.md has windy_leaves | 37 | 4 0 0 2 | 248 | `guTranslateF(sp18, part->unk_04.x, part->unk_04.y, part->unk_04.z);` | `part->unk_04.y` | live
Needs: docs/handoff/row_SEEDS.md has attack_result_text | 66 | 9 0 4 5 | 196 | `guTranslateF(mtxA, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has breaking_junk | 118 | 7 0 0 2 | 146 | `guPositionF(sp20, 0.0f, -gCameras[gCurrentCameraID].curYaw, 0.0f, data->scale * 0.5, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has cloud_puff | 5 | 6 0 0 2 | 131 | `guScaleF(sp60, part->unk_18, part->unk_1C, part->unk_20);` | `part->unk_1C` | live
Needs: docs/handoff/row_SEEDS.md has cloud_trail | 6 | 6 0 0 2 | 132 | `guScaleF(sp60, part->unk_1C, part->unk_20, part->unk_24);` | `part->unk_20` | live
Needs: docs/handoff/row_SEEDS.md has damage_stars | 19 | 7 0 0 2 | 257 | `guTranslateF(mtxTransform, part->pos.x, part->pos.y, part->pos.z);` | `part->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has disable_x | 59 | 13 0 4 11 | 219 | `guTranslateF(sp18, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has effect_63 | 93 | 9 0 0 2 | 250 | `guTranslateF(sp18, part->unk_04, part->unk_08, part->unk_0C);` | `part->unk_08` | live
Needs: docs/handoff/row_SEEDS.md has emote | 11 | 13 0 0 4 | 225 | `guTranslateF(sp18, part->unk_04, part->unk_08, part->unk_0C);` | `part->unk_08` | live
## Exact edits
1. azt.d/CODEMODB9.json codemodb9_drec: a copy of the fxpic_89 scenario (azt_scenarios.json: first_marker and warp event `warp: mac_01 entry 0`; not the prof marker of spot_mac01_drec) with `debug`
   `warp=mac_01:0 pos=0:0:200 fxtest=37,66,118,5,6,19,59,93,11 loglevel=debug det drec=412:9:44`, until `drec f764 draws [1-9]\d* bytes \d+`,
   cap 45, deadline 40, needs_picture false. bench.d/CODEMODB9.json: codemodb9_drec, kind azahar_pic, tags azahar det drec. Commit them.
2. `python 3ds/tools/codemod.py src/effects/windy_leaves.c src/effects/attack_result_text.c src/effects/breaking_junk.c src/effects/cloud_puff.c src/effects/cloud_trail.c src/effects/damage_stars.c src/effects/disable_x.c src/effects/effect_63.c src/effects/emote.c`.
3. In each file that now calls nd_val_* or nd_xf_*, after its last #include line add `#include "nd_val.h"` and
   `#include "nd_xf.h"` (skip a line that is already there).
4. Commit: `CODEMODB9: codemod batch of 9 effect files (74 value calls to native records)`.
## Gate (expected values)
0. Draw proof, from the log of the base run of step 4: for each id, `fxtest: live 1` after its spawn and `fxcenb <id> tri <T>`
   with T above the `fxcenb -1 tri` value. A file without it moves out of the batch (row_TEMPLATE.md fix rules).
1. `python 3ds/tools/codemod.py --dry src/effects/windy_leaves.c src/effects/attack_result_text.c src/effects/breaking_junk.c src/effects/cloud_puff.c src/effects/cloud_trail.c src/effects/damage_stars.c src/effects/disable_x.c src/effects/effect_63.c src/effects/emote.c`: value 0 for each file, keep, state and dl as in the table.
2. `python 3ds/tools/listcen.py --group src/effects` before edit 2 and after: the build count of the batch falls by 74.
3. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row CODEMODB9 --why "CODEMODB9"`: PASS.
4. `bench.py base codemodb9_drec` (merge-base), `bench.py run codemodb9_drec --row CODEMODB9 --why "CODEMODB9 drec gate"`, `bench.py cmp codemodb9_drec
   --row CODEMODB9`: drec.txt byte-equal.
5. Seeded fault: the seed of each file has the verdict live in its seeds table line (the Needs lines above; a probe row measured
   it with the same fxtest window before this row was written). This row runs no probe.
6. Budget 8 Azahar starts. Counter: C1 -74, C2 -74.
## Stop and end
A drec difference of the real edit, a tool fault, or a counter that does not fall: stop, commit, hand back the exact output.
Delete this row file in the completing commit.
