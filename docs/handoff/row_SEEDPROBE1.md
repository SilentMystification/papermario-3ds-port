# Row SEEDPROBE1: probe 12 seeds of row_SEEDS.md with seedprobe.py (2 groups, one run each) - pilot

Blocked by SEEDTOOL (seedprobe.py --write) and DRECSTEP (step windows with frame lines). This is the pilot of the probe rows: the oracle
reads its result before SEEDPROBE2 and later start.
(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. ORACLE8 2026-11-13.)
## Facts (ORACLE8, main d13d0f663)
- fxtest runs `fxtest=a,b,...` one effect per 44 frames from frame 400 (pm_fxtest.c:179-180), so effect i draws at 412 + 44 i.
- Each seed: + 20.0f on the y argument of the first unguarded guTranslateF or guPositionF of <file>_appendGfx (row_SEEDS.md rule).
Start: `sh 3ds/tools/wt_new.sh SEEDPROBE1`. PM_ROW=SEEDPROBE1 PM_LANE=SEEDPROBE1 on every call.
## Touch only (places)
docs/handoff/row_SEEDS.md (the verdict field of these 12 lines, written by seedprobe.py --write). New azt.d/SEEDPROBE1.json and
bench.d/SEEDPROBE1.json. No source file stays changed (seedprobe.py restores it). New names: seedprobe1_a_drec, seedprobe1_b_drec. Host link: none.
Needs: 3ds/tools/seedprobe.py has --write
## Exact edits
1. New azt.d/SEEDPROBE1.json: copies of the fxpic_89 scenario (azt_scenarios.json: first_marker and warp event `warp: mac_01 entry 0`; not the prof marker of spot_mac01_drec) with these `debug` fields and untils, cap 40, deadline 35,
   needs_picture false. bench.d/SEEDPROBE1.json: the same names, kind azahar_pic, tags azahar det drec. Commit them.
   - seedprobe1_a_drec: `warp=mac_01:0 pos=0:0:200 fxtest=53,69,42,56,60,79 loglevel=debug det drec=412:6:44`, until
     `drec f632 draws [1-9]\d* bytes \d+`.
   - seedprobe1_b_drec: `warp=mac_01:0 pos=0:0:200 fxtest=71,104,62,43,15,55 loglevel=debug det drec=412:6:44`, until
     `drec f632 draws [1-9]\d* bytes \d+`.
2. For each group: `python 3ds/tools/seedprobe.py <scene> <the seeds of the group> --write docs/handoff/row_SEEDS.md`:
   Group A (seedprobe1_a_drec):
   `src/effects/aura.c:307@412|data->posB.y|data->posB.y + 20.0f`
   `src/effects/balloon.c:96@456|data->unk_08|data->unk_08 + 20.0f`
   `src/effects/big_snowflakes.c:123@500|data->unk_08|data->unk_08 + 20.0f`
   `src/effects/blast.c:111@544|data->pos.y|data->pos.y + 20.0f`
   `src/effects/bombette_breaking.c:221@588|data->center.y|data->center.y + 20.0f`
   `src/effects/butterflies.c:176@632|data->unk_10 + D_E00AA6EC[data->unk_2C] * 0.3f|data->unk_10 + D_E00AA6EC[data->unk_2C] * 0.3f + 20.0f`
   Group B (seedprobe1_b_drec):
   `src/effects/chomp_drop.c:153@412|data->unk_08 * 100.0f|data->unk_08 * 100.0f + 20.0f`
   `src/effects/cold_breath.c:200@456|data->pos.y|data->pos.y + 20.0f`
   `src/effects/confetti.c:274@500|part->unk_08|part->unk_08 + 20.0f`
   `src/effects/debuff.c:148@544|data->pos.y|data->pos.y + 20.0f`
   `src/effects/drop_leaves.c:141@588|part->unk_08|part->unk_08 + 20.0f`
   `src/effects/effect_3D.c:229@632|part->pos.y|part->pos.y + 20.0f`
3. Commit: `SEEDPROBE1: verdicts of 12 seeds (seedprobe.py)`.
## Gate (expected values)
1. Each seedprobe call prints one verdict line per seed and no tool-fault, bad-spec or no-seed-line line; `git status` shows no
   src file changed after each call; row_SEEDS.md changed only in the probed lines.
2. The verdicts are the result: no expected value. A dead seed goes back to the oracle as its row_SEEDS.md line (the dead-seed
   fix rule of row_TEMPLATE.md is for code rows, not for probe rows).
3. Budget 8 Azahar starts (each group: a base pair and 1 run). Counter: none moves (probe row for the batch rows).
## Stop and end
A tool-fault line, a bad-spec line, or a src file left changed: stop, commit, hand back the exact output. Delete this row file at
the end.
