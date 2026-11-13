# Row CODEMODB1: codemod batch of 9 effect files (109 value calls to native records), drec gate over one fxtest window

(Written by the oracle in the mkrow.py form before the probes; it starts when rowlint shows no needs fault, i.e. every
seed below has the verdict live on main.)
(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md, row_TEMPLATE.md and what it names. Written by mkrow.py from
row_BATCHTEMPLATE.md and a seeds table; every seed has the verdict live from a probe row.)
## Facts (mkrow.py, main 9fbb741e3)
- 3ds/tools/codemod.py rewrites the value calls (gSPMatrix, gSPPopMatrix, gDPPipeSync, gDPSetPrimColor, gDPSetEnvColor, gSPSegment,
  gDPSetTileSize) to nd_xf_* and nd_val_*. The files (codemod --dry: value keep state dl | fxtest id | drec frame | live seed):
  - src/effects/effect_3D.c: 5 3 0 2 | fxtest 55 | frame 412 | seed `src/effects/effect_3D.c:229@412|part->pos.y|part->pos.y + 20.0f`
  - src/effects/snowfall.c: 4 3 0 2 | fxtest 63 | frame 456 | seed `src/effects/snowfall.c:214@456|data->unk_0C|data->unk_0C + 20.0f`
  - src/effects/pink_sparkles.c: 3 3 0 2 | fxtest 124 | frame 500 | seed `src/effects/pink_sparkles.c:251@500|part->unk_08|part->unk_08 + 20.0f`
  - src/effects/radiating_energy_orb.c: 20 0 2 7 | fxtest 73 | frame 544 | seed `src/effects/radiating_energy_orb.c:150@544|part->unk_0C|part->unk_0C + 20.0f`
  - src/effects/quizmo_stage.c: 20 0 1 13 | fxtest 72 | frame 588 | seed `src/effects/quizmo_stage.c:118@588|data->origin.y|data->origin.y + 20.0f`
  - src/effects/peach_star_beam.c: 15 0 0 8 | fxtest 112 | frame 632 | seed `src/effects/peach_star_beam.c:211@632|data->pos.y|data->pos.y + 20.0f`
  - src/effects/whirlwind.c: 14 0 3 1 | fxtest 97 | frame 676 | seed `src/effects/whirlwind.c:179@676|sp20, 0.0f, 0.0f|sp20, 0.0f, 0.0f + 20.0f`
  - src/effects/landing_dust.c: 14 0 0 3 | fxtest 1 | frame 720 | seed `src/effects/landing_dust.c:290@720|part->y|part->y + 20.0f`
  - src/effects/damage_indicator.c: 14 0 4 15 | fxtest 25 | frame 764 | seed `src/effects/damage_indicator.c:208@764|part->basePos.y|part->basePos.y + 20.0f`
- fxtest runs the ids in the order of the switch, one per 44 frames from frame 400 (pm_fxtest.c:179-180). File i draws at frame
  412 + 44 i. drec=412:9:44 records those frames (DRECSTEP frame lines).
Start: `sh 3ds/tools/wt_new.sh CODEMODB1`. PM_ROW=CODEMODB1 PM_LANE=CODEMODB1 on every call.
## Touch only (places)
The value call lines and the include lines of: src/effects/effect_3D.c, src/effects/snowfall.c, src/effects/pink_sparkles.c, src/effects/radiating_energy_orb.c, src/effects/quizmo_stage.c, src/effects/peach_star_beam.c, src/effects/whirlwind.c, src/effects/landing_dust.c, src/effects/damage_indicator.c. New azt.d/CODEMODB1.json and bench.d/CODEMODB1.json.
New names: codemodb1_drec. Host link: none.
Needs: docs/handoff/row_SEEDS.md has effect_3D | 55 | 5 3 0 2 | 229 | `guTranslateF(sp18, part->pos.x, part->pos.y, part->pos.z);` | `part->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has snowfall | 63 | 4 3 0 2 | 214 | `guTranslateF(sp18, data->unk_08, data->unk_0C, data->unk_10);` | `data->unk_0C` | live
Needs: docs/handoff/row_SEEDS.md has pink_sparkles | 124 | 3 3 0 2 | 251 | `guTranslateF(sp98, part->unk_04, part->unk_08, part->unk_0C);` | `part->unk_08` | live
Needs: docs/handoff/row_SEEDS.md has radiating_energy_orb | 73 | 20 0 2 7 | 150 | `guTranslateF(sp18, part->unk_08, part->unk_0C, part->unk_10);` | `part->unk_0C` | live
Needs: docs/handoff/row_SEEDS.md has quizmo_stage | 72 | 20 0 1 13 | 118 | `guTranslateF(sp18, data->origin.x, data->origin.y, data->origin.z);` | `data->origin.y` | live
Needs: docs/handoff/row_SEEDS.md has peach_star_beam | 112 | 15 0 0 8 | 211 | `guTranslateF(sp20, data->pos.x, data->pos.y - (((f32) (255 - data->unk_3C) * 400.0) / 255.0), data->pos.z);` | `data->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has whirlwind | 97 | 14 0 3 1 | 179 | `guTranslateF(sp20, 0.0f, 0.0f, 0.0f);` | `sp20, 0.0f, 0.0f` | live
Needs: docs/handoff/row_SEEDS.md has landing_dust | 1 | 14 0 0 3 | 290 | `guTranslateF(mtx1, part->x, part->y, part->z);` | `part->y` | live
Needs: docs/handoff/row_SEEDS.md has damage_indicator | 25 | 14 0 4 15 | 208 | `guTranslateF(mtxTransform, part->basePos.x, part->basePos.y, part->basePos.z);` | `part->basePos.y` | live
## Exact edits
1. azt.d/CODEMODB1.json codemodb1_drec: a copy of the fxpic_89 scenario (azt_scenarios.json: first_marker and warp event `warp: mac_01 entry 0`; not the prof marker of spot_mac01_drec) with `debug`
   `warp=mac_01:0 pos=0:0:200 fxtest=55,63,124,73,72,112,97,1,25 loglevel=debug det drec=412:9:44`, until `drec f764 draws [1-9]\d* bytes \d+`,
   cap 45, deadline 40, needs_picture false. bench.d/CODEMODB1.json: codemodb1_drec, kind azahar_pic, tags azahar det drec. Commit them.
2. `python 3ds/tools/codemod.py src/effects/effect_3D.c src/effects/snowfall.c src/effects/pink_sparkles.c src/effects/radiating_energy_orb.c src/effects/quizmo_stage.c src/effects/peach_star_beam.c src/effects/whirlwind.c src/effects/landing_dust.c src/effects/damage_indicator.c`.
3. In each file that now calls nd_val_* or nd_xf_*, after its last #include line add `#include "nd_val.h"` and
   `#include "nd_xf.h"` (skip a line that is already there).
4. Commit: `CODEMODB1: codemod batch of 9 effect files (109 value calls to native records)`.
## Gate (expected values)
0. Draw proof, from the log of the base run of step 4: for each id, `fxtest: live 1` after its spawn and `fxcenb <id> tri <T>`
   with T above the `fxcenb -1 tri` value. A file without it moves out of the batch (row_TEMPLATE.md fix rules).
1. `python 3ds/tools/codemod.py --dry src/effects/effect_3D.c src/effects/snowfall.c src/effects/pink_sparkles.c src/effects/radiating_energy_orb.c src/effects/quizmo_stage.c src/effects/peach_star_beam.c src/effects/whirlwind.c src/effects/landing_dust.c src/effects/damage_indicator.c`: value 0 for each file, keep, state and dl as in the table.
2. `python 3ds/tools/listcen.py --group src/effects` before edit 2 and after: the build count of the batch falls by 109.
3. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row CODEMODB1 --why "CODEMODB1"`: PASS.
4. `bench.py base codemodb1_drec` (merge-base), `bench.py run codemodb1_drec --row CODEMODB1 --why "CODEMODB1 drec gate"`, `bench.py cmp codemodb1_drec
   --row CODEMODB1`: drec.txt byte-equal.
5. Seeded fault: the seed of each file has the verdict live in its seeds table line (the Needs lines above; a probe row measured
   it with the same fxtest window before this row was written). This row runs no probe.
6. Budget 8 Azahar starts. Counter: C1 -109, C2 -109.
## Stop and end
A drec difference of the real edit, a tool fault, or a counter that does not fall: stop, commit, hand back the exact output.
Delete this row file in the completing commit.
