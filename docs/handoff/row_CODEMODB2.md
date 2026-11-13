# Row CODEMODB2: codemod batch of 10 effect files (77 value calls to native records), drec gate over one fxtest window

(Written by the oracle in the mkrow.py form before the probes; it starts when rowlint shows no needs fault, i.e. every
seed below has the verdict live on main.)
(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md, row_TEMPLATE.md and what it names. Written by mkrow.py from
row_BATCHTEMPLATE.md and a seeds table; every seed has the verdict live from a probe row.)
## Facts (mkrow.py, main 351c8861e)
- 3ds/tools/codemod.py rewrites the value calls (gSPMatrix, gSPPopMatrix, gDPPipeSync, gDPSetPrimColor, gDPSetEnvColor, gSPSegment,
  gDPSetTileSize) to nd_xf_* and nd_val_*. The files (codemod --dry: value keep state dl | fxtest id | drec frame | live seed):
  - src/effects/aura.c: 10 0 0 5 | fxtest 53 | frame 412 | seed `src/effects/aura.c:307@412|data->posB.y|data->posB.y + 20.0f`
  - src/effects/balloon.c: 5 0 0 2 | fxtest 69 | frame 456 | seed `src/effects/balloon.c:96@456|data->unk_08|data->unk_08 + 20.0f`
  - src/effects/big_snowflakes.c: 4 0 0 2 | fxtest 42 | frame 500 | seed `src/effects/big_snowflakes.c:123@500|data->unk_08|data->unk_08 + 20.0f`
  - src/effects/blast.c: 6 1 0 3 | fxtest 56 | frame 544 | seed `src/effects/blast.c:111@544|data->pos.y|data->pos.y + 20.0f`
  - src/effects/bombette_breaking.c: 9 0 0 3 | fxtest 60 | frame 588 | seed `src/effects/bombette_breaking.c:221@588|data->center.y|data->center.y + 20.0f`
  - src/effects/butterflies.c: 6 0 1 3 | fxtest 79 | frame 632 | seed `src/effects/butterflies.c:176@632|data->unk_10 + D_E00AA6EC[data->unk_2C] * 0.3f|data->unk_10 + D_E00AA6EC[data->unk_2C] * 0.3f + 20.0f`
  - src/effects/chomp_drop.c: 7 0 1 6 | fxtest 71 | frame 676 | seed `src/effects/chomp_drop.c:153@676|data->unk_08 * 100.0f|data->unk_08 * 100.0f + 20.0f`
  - src/effects/cold_breath.c: 12 0 2 2 | fxtest 104 | frame 720 | seed `src/effects/cold_breath.c:200@720|data->pos.y|data->pos.y + 20.0f`
  - src/effects/confetti.c: 9 0 3 2 | fxtest 62 | frame 764 | seed `src/effects/confetti.c:274@764|part->unk_08|part->unk_08 + 20.0f`
  - src/effects/debuff.c: 9 0 0 2 | fxtest 43 | frame 808 | seed `src/effects/debuff.c:148@808|data->pos.y|data->pos.y + 20.0f`
- fxtest runs the ids in the order of the switch, one per 44 frames from frame 400 (pm_fxtest.c:179-180). File i draws at frame
  412 + 44 i. drec=412:10:44 records those frames (DRECSTEP frame lines).
Start: `sh 3ds/tools/wt_new.sh CODEMODB2`. PM_ROW=CODEMODB2 PM_LANE=CODEMODB2 on every call.
## Touch only (places)
The value call lines and the include lines of: src/effects/aura.c, src/effects/balloon.c, src/effects/big_snowflakes.c, src/effects/blast.c, src/effects/bombette_breaking.c, src/effects/butterflies.c, src/effects/chomp_drop.c, src/effects/cold_breath.c, src/effects/confetti.c, src/effects/debuff.c. New azt.d/CODEMODB2.json and bench.d/CODEMODB2.json.
New names: codemodb2_drec. Host link: none.
Needs: docs/handoff/row_SEEDS.md has aura | 53 | 10 0 0 5 | 307 | `guTranslateF(translateMtx, data->posB.x, data->posB.y, data->posB.z);` | `data->posB.y` | live
Needs: docs/handoff/row_SEEDS.md has balloon | 69 | 5 0 0 2 | 96 | `guTranslateF(sp18, data->unk_04, data->unk_08, data->unk_0C);` | `data->unk_08` | live
Needs: docs/handoff/row_SEEDS.md has big_snowflakes | 42 | 4 0 0 2 | 123 | `guTranslateF(sp18, data->unk_04, data->unk_08, data->unk_0C);` | `data->unk_08` | live
Needs: docs/handoff/row_SEEDS.md has blast | 56 | 6 1 0 3 | 111 | `guTranslateF(sp18, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has bombette_breaking | 60 | 9 0 0 3 | 221 | `guTranslateF(sp20, data->center.x, data->center.y, data->center.z);` | `data->center.y` | live
Needs: docs/handoff/row_SEEDS.md has butterflies | 79 | 6 0 1 3 | 176 | `guTranslateF(sp18, data->unk_0C, data->unk_10 + D_E00AA6EC[data->unk_2C] * 0.3f, data->unk_14);` | `data->unk_10 + D_E00AA6EC[data->unk_2C] * 0.3f` | live
Needs: docs/handoff/row_SEEDS.md has chomp_drop | 71 | 7 0 1 6 | 153 | `guPositionF(sp60, 0.0f, 0.0f, 0.0f, data->unk_24, data->unk_04 * 100.0f, data->unk_08 * 100.0f, data->unk_0C * 100.0f);` | `data->unk_08 * 100.0f` | live
Needs: docs/handoff/row_SEEDS.md has cold_breath | 104 | 12 0 2 2 | 200 | `guTranslateF(sp20, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | live
Needs: docs/handoff/row_SEEDS.md has confetti | 62 | 9 0 3 2 | 274 | `guTranslateF(sp18, part->unk_04, part->unk_08, part->unk_0C);` | `part->unk_08` | live
Needs: docs/handoff/row_SEEDS.md has debuff | 43 | 9 0 0 2 | 148 | `guTranslateF(mtxTranslate, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | live
## Exact edits
1. azt.d/CODEMODB2.json codemodb2_drec: a copy of the fxpic_89 scenario (azt_scenarios.json: first_marker and warp event `warp: mac_01 entry 0`; not the prof marker of spot_mac01_drec) with `debug`
   `warp=mac_01:0 pos=0:0:200 fxtest=53,69,42,56,60,79,71,104,62,43 loglevel=debug det drec=412:10:44`, until `drec f808 draws [1-9]\d* bytes \d+`,
   cap 45, deadline 40, needs_picture false. bench.d/CODEMODB2.json: codemodb2_drec, kind azahar_pic, tags azahar det drec. Commit them.
2. `python 3ds/tools/codemod.py src/effects/aura.c src/effects/balloon.c src/effects/big_snowflakes.c src/effects/blast.c src/effects/bombette_breaking.c src/effects/butterflies.c src/effects/chomp_drop.c src/effects/cold_breath.c src/effects/confetti.c src/effects/debuff.c`.
3. In each file that now calls nd_val_* or nd_xf_*, after its last #include line add `#include "nd_val.h"` and
   `#include "nd_xf.h"` (skip a line that is already there).
4. Commit: `CODEMODB2: codemod batch of 10 effect files (77 value calls to native records)`.
## Gate (expected values)
0. Draw proof, from the log of the base run of step 4: for each id, `fxtest: live 1` after its spawn and `fxcenb <id> tri <T>`
   with T above the `fxcenb -1 tri` value. A file without it moves out of the batch (row_TEMPLATE.md fix rules).
1. `python 3ds/tools/codemod.py --dry src/effects/aura.c src/effects/balloon.c src/effects/big_snowflakes.c src/effects/blast.c src/effects/bombette_breaking.c src/effects/butterflies.c src/effects/chomp_drop.c src/effects/cold_breath.c src/effects/confetti.c src/effects/debuff.c`: value 0 for each file, keep, state and dl as in the table.
2. `python 3ds/tools/listcen.py --group src/effects` before edit 2 and after: the build count of the batch falls by 77.
3. Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row CODEMODB2 --why "CODEMODB2"`: PASS.
4. `bench.py base codemodb2_drec` (merge-base), `bench.py run codemodb2_drec --row CODEMODB2 --why "CODEMODB2 drec gate"`, `bench.py cmp codemodb2_drec
   --row CODEMODB2`: drec.txt byte-equal.
5. Seeded fault: the seed of each file has the verdict live in its seeds table line (the Needs lines above; a probe row measured
   it with the same fxtest window before this row was written). This row runs no probe.
6. Budget 8 Azahar starts. Counter: C1 -77, C2 -77.
## Stop and end
A drec difference of the real edit, a tool fault, or a counter that does not fall: stop, commit, hand back the exact output.
Delete this row file in the completing commit.
