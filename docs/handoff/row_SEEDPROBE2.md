# Row SEEDPROBE2: probe 12 seeds of row_SEEDS.md with seedprobe.py (2 groups, one run each)

Blocked by SEEDTOOL (seedprobe.py --write) and DRECSTEP (step windows with frame lines). Starts after the oracle read the SEEDPROBE1 pilot.
(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. ORACLE8 2026-11-13.)
## Facts (ORACLE8, main d13d0f663)
- fxtest runs `fxtest=a,b,...` one effect per 44 frames from frame 400 (pm_fxtest.c:179-180), so effect i draws at 412 + 44 i.
- Each seed: + 20.0f on the y argument of the first unguarded guTranslateF or guPositionF of <file>_appendGfx (row_SEEDS.md rule).
Start: `sh 3ds/tools/wt_new.sh SEEDPROBE2`. PM_ROW=SEEDPROBE2 PM_LANE=SEEDPROBE2 on every call.
## Touch only (places)
docs/handoff/row_SEEDS.md (the verdict field of these 12 lines, written by seedprobe.py --write). New azt.d/SEEDPROBE2.json and
bench.d/SEEDPROBE2.json. No source file stays changed (seedprobe.py restores it). New names: seedprobe2_a_drec, seedprobe2_b_drec. Host link: none.
Needs: 3ds/tools/seedprobe.py has --write
## Exact edits
1. New azt.d/SEEDPROBE2.json: copies of the fxpic_89 scenario (azt_scenarios.json: first_marker and warp event `warp: mac_01 entry 0`; not the prof marker of spot_mac01_drec) with these `debug` fields and untils, cap 40, deadline 35,
   needs_picture false. bench.d/SEEDPROBE2.json: the same names, kind azahar_pic, tags azahar det drec. Commit them.
   - seedprobe2_a_drec: `warp=mac_01:0 pos=0:0:200 fxtest=64,95,110,126,46,100 loglevel=debug det drec=412:6:44`, until
     `drec f632 draws [1-9]\d* bytes \d+`.
   - seedprobe2_b_drec: `warp=mac_01:0 pos=0:0:200 fxtest=51,38,57,68,99,8 loglevel=debug det drec=412:6:44`, until
     `drec f632 draws [1-9]\d* bytes \d+`.
2. For each group: `python 3ds/tools/seedprobe.py <scene> <the seeds of the group> --write docs/handoff/row_SEEDS.md`:
   Group A (seedprobe2_a_drec):
   `src/effects/effect_46.c:219@412|part->pos.y|part->pos.y + 20.0f`
   `src/effects/effect_65.c:254@456|0.0f|0.0f + 20.0f`
   `src/effects/effect_75.c:207@500|data->pos.y|data->pos.y + 20.0f`
   `src/effects/effect_86.c:122@544|part->unk_08|part->unk_08 + 20.0f`
   `src/effects/ending_decals.c:149@588|data->pos.y|data->pos.y + 20.0f`
   `src/effects/energy_in_out.c:259@632|part->pos.y|part->pos.y + 20.0f`
   Group B (seedprobe2_b_drec):
   `src/effects/energy_shockwave.c:164@412|data->unk_08|data->unk_08 + 20.0f`
   `src/effects/falling_leaves.c:136@456|part->unk_08|part->unk_08 + 20.0f`
   `src/effects/fire_flower.c:218@500|part->pos.y|part->pos.y + 20.0f`
   `src/effects/flashing_box_shockwave.c:154@544|data->pos.y|data->pos.y + 20.0f`
   `src/effects/floating_cloud_puff.c:128@588|data->unk_08|data->unk_08 + 20.0f`
   `src/effects/floating_flower.c:154@632|part->pos.y|part->pos.y + 20.0f`
3. Commit: `SEEDPROBE2: verdicts of 12 seeds (seedprobe.py)`.
## Gate (expected values)
1. Each seedprobe call prints one verdict line per seed and no tool-fault, bad-spec or no-seed-line line; `git status` shows no
   src file changed after each call; row_SEEDS.md changed only in the probed lines.
2. The verdicts are the result: no expected value. A dead seed goes back to the oracle as its row_SEEDS.md line (the dead-seed
   fix rule of row_TEMPLATE.md is for code rows, not for probe rows).
3. Budget 8 Azahar starts (each group: a base pair and 1 run). Counter: none moves (probe row for the batch rows).
## Stop and end
A tool-fault line, a bad-spec line, or a src file left changed: stop, commit, hand back the exact output. Delete this row file at
the end.
