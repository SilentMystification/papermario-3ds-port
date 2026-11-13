# Row CODEMODB4: codemod batch of 10 effect files (97 value calls to native records), drec gate over one fxtest window

(Written by the oracle in the mkrow.py form before the probes; it starts when rowlint shows no needs fault, i.e. every
seed below has the verdict live on main.)
(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md, row_TEMPLATE.md and what it names. Written by mkrow.py from
row_BATCHTEMPLATE.md and a seeds table; every seed has the verdict live from a probe row.)
## Facts (mkrow.py, main 351c8861e)
- 3ds/tools/codemod.py rewrites the value calls (gSPMatrix, gSPPopMatrix, gDPPipeSync, gDPSetPrimColor, gDPSetEnvColor, gSPSegment,
  gDPSetTileSize) to nd_xf_* and nd_val_*. The files (codemod --dry: value keep state dl | fxtest id | drec frame | live seed):
  - src/effects/flashing_box_shockwave.c: 12 0 0 4 | fxtest 68 | frame 412 | seed `src/effects/flashing_box_shockwave.c:154@412|data->pos.y|data->pos.y + 20.0f`
  - src/effects/floating_cloud_puff.c: 7 0 0 2 | fxtest 99 | frame 456 | seed `src/effects/floating_cloud_puff.c:128@456|data->unk_08|data->unk_08 + 20.0f`
  - src/effects/floating_flower.c: 5 0 0 1 | fxtest 8 | frame 500 | seed `src/effects/floating_flower.c:154@500|part->pos.y|part->pos.y + 20.0f`
  - src/effects/floating_rock.c: 6 0 2 2 | fxtest 70 | frame 544 | seed `src/effects/floating_rock.c:121@544|data->pos.y|data->pos.y + 20.0f`
  - src/effects/fright_jar.c: 14 0 11 1 | fxtest 91 | frame 588 | seed `src/effects/fright_jar.c:130@588|data->unk_08|data->unk_08 + 20.0f`
  - src/effects/gather_energy_pink.c: 11 0 0 2 | fxtest 14 | frame 632 | seed `src/effects/gather_energy_pink.c:179@632|part->posB.y|part->posB.y + 20.0f`
  - src/effects/gather_magic.c: 8 0 0 2 | fxtest 65 | frame 676 | seed `src/effects/gather_magic.c:168@676|part->unk_0C|part->unk_0C + 20.0f`
  - src/effects/green_impact.c: 12 0 4 2 | fxtest 44 | frame 720 | seed `src/effects/green_impact.c:168@720|part->unk_08|part->unk_08 + 20.0f`
  - src/effects/hieroglyphs.c: 14 1 1 1 | fxtest 106 | frame 764 | seed `src/effects/hieroglyphs.c:130@764|data->pos.y|data->pos.y + 20.0f`
  - src/effects/huff_puff_breath.c: 8 0 0 2 | fxtest 103 | frame 808 | seed `src/effects/huff_puff_breath.c:141@808|data->pos.y|data->pos.y + 20.0f`
- fxtest runs the ids in the order of the switch, one per 44 frames from frame 400 (pm_fxtest.c:179-180). File i draws at frame
  412 + 44 i. drec=412:10:44 records those frames (DRECSTEP frame lines).
Start: `sh 3ds/tools/wt_new.sh CODEMODB4`. PM_ROW=CODEMODB4 PM_LANE=CODEMODB4 on every call.
## Touch only (places)
The value call lines and the include lines of: src/effects/flashing_box_shockwave.c, src/effects/floating_cloud_puff.c, src/effects/floating_flower.c, src/effects/floating_rock.c, src/effects/fright_jar.c, src/effects/gather_energy_pink.c, src/effects/gather_magic.c, src/effects/green_impact.c, src/effects/hieroglyphs.c, src/effects/huff_puff_breath.c. New azt.d/CODEMODB4.json and bench.d/CODEMODB4.json.
New names: codemodb4_drec. Host link: none.
Needs: docs/handoff/row_SEEDS.md has flashing_box_shockwave | 68 | 12 0 0 4 | 154 | `guTranslateF(mtxTransform, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has floating_cloud_puff | 99 | 7 0 0 2 | 128 | `guTranslateF(sp10, data->unk_04, data->unk_08, data->unk_0C);` | `data->unk_08` | live
Needs: docs/handoff/row_SEEDS.md has floating_flower | 8 | 5 0 0 1 | 154 | `guPositionF(mtxTransform, part->rot.x, part->rot.y, 0.0f, 1.0f, part->pos.x, part->pos.y, part->pos.z);` | `part->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has floating_rock | 70 | 6 0 2 2 | 121 | `guPositionF(mtxTransform, data->rot.x, data->rot.z, data->rot.y, data->scale, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has fright_jar | 91 | 14 0 11 1 | 130 | `guTranslateF(sp10, data->unk_04, data->unk_08, data->unk_0C);` | `data->unk_08` | live
Needs: docs/handoff/row_SEEDS.md has gather_energy_pink | 14 | 11 0 0 2 | 179 | `guTranslateF(sp20, part->posB.x, part->posB.y, part->posB.z);` | `part->posB.y` | live
Needs: docs/handoff/row_SEEDS.md has gather_magic | 65 | 8 0 0 2 | 168 | `guTranslateF(sp18, part->unk_08, part->unk_0C, part->unk_10);` | `part->unk_0C` | live
Needs: docs/handoff/row_SEEDS.md has green_impact | 44 | 12 0 4 2 | 168 | `guPositionF(sp20, 0.0f, -gCameras[gCurrentCameraID].curYaw, 0.0f, 1.0f, part->unk_04, part->unk_08, part->unk_0C);` | `part->unk_08` | live
Needs: docs/handoff/row_SEEDS.md has hieroglyphs | 106 | 14 1 1 1 | 130 | `guTranslateF(sp20, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has huff_puff_breath | 103 | 8 0 0 2 | 141 | `guTranslateF(sp18, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | live
## Exact edits
1. azt.d/CODEMODB4.json codemodb4_drec: a copy of the fxpic_89 scenario (azt_scenarios.json: first_marker and warp event `warp: mac_01 entry 0`; not the prof marker of spot_mac01_drec) with `debug`
   `warp=mac_01:0 pos=0:0:200 fxtest=68,99,8,70,91,14,65,44,106,103 loglevel=debug det drec=412:10:44`, until `drec f808 draws [1-9]\d* bytes \d+`,
   cap 45, deadline 40, needs_picture false. bench.d/CODEMODB4.json: codemodb4_drec, kind azahar_pic, tags azahar det drec. Commit them.
2. `python 3ds/tools/codemod.py src/effects/flashing_box_shockwave.c src/effects/floating_cloud_puff.c src/effects/floating_flower.c src/effects/floating_rock.c src/effects/fright_jar.c src/effects/gather_energy_pink.c src/effects/gather_magic.c src/effects/green_impact.c src/effects/hieroglyphs.c src/effects/huff_puff_breath.c`.
3. In each file that now calls nd_val_* or nd_xf_*, after its last #include line add `#include "nd_val.h"` and
   `#include "nd_xf.h"` (skip a line that is already there).
4. Commit: `CODEMODB4: codemod batch of 10 effect files (97 value calls to native records)`.
## Gate (expected values)
0. Draw proof, from the log of the base run of step 4: for each id, `fxtest: live 1` after its spawn and `fxcenb <id> tri <T>`
   with T above the `fxcenb -1 tri` value. A file without it moves out of the batch (row_TEMPLATE.md fix rules).
1. `python 3ds/tools/codemod.py --dry src/effects/flashing_box_shockwave.c src/effects/floating_cloud_puff.c src/effects/floating_flower.c src/effects/floating_rock.c src/effects/fright_jar.c src/effects/gather_energy_pink.c src/effects/gather_magic.c src/effects/green_impact.c src/effects/hieroglyphs.c src/effects/huff_puff_breath.c`: value 0 for each file, keep, state and dl as in the table.
2. `python 3ds/tools/listcen.py --group src/effects` before edit 2 and after: the build count of the batch falls by 97.
3. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row CODEMODB4 --why "CODEMODB4"`: PASS.
4. `bench.py base codemodb4_drec` (merge-base), `bench.py run codemodb4_drec --row CODEMODB4 --why "CODEMODB4 drec gate"`, `bench.py cmp codemodb4_drec
   --row CODEMODB4`: drec.txt byte-equal.
5. Seeded fault: the seed of each file has the verdict live in its seeds table line (the Needs lines above; a probe row measured
   it with the same fxtest window before this row was written). This row runs no probe.
6. Budget 8 Azahar starts. Counter: C1 -97, C2 -97.
## Stop and end
A drec difference of the real edit, a tool fault, or a counter that does not fall: stop, commit, hand back the exact output.
Delete this row file in the completing commit.
