# Row SEEDPROBE6: probe 8 seeds of row_SEEDS.md with seedprobe.py (2 groups, one run each)

Blocked by SEEDTOOL (seedprobe.py --write) and DRECSTEP (step windows with frame lines). Starts after the oracle read the SEEDPROBE1 pilot.
(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. ORACLE8 2026-11-13.)
## Facts (ORACLE8, main 3d272405c)
- fxtest runs `fxtest=a,b,...` one effect per 44 frames from frame 400 (pm_fxtest.c:179-180), so effect i draws at 412 + 44 i.
- Each seed: + 20.0f on the y argument of the first unguarded guTranslateF or guPositionF of <file>_appendGfx (row_SEEDS.md rule).
Start: `sh 3ds/tools/wt_new.sh SEEDPROBE6`. PM_ROW=SEEDPROBE6 PM_LANE=SEEDPROBE6 on every call.
## Touch only (places)
docs/handoff/row_SEEDS.md (the verdict field of these 8 lines, written by seedprobe.py --write). New azt.d/SEEDPROBE6.json and
bench.d/SEEDPROBE6.json. No source file stays changed (seedprobe.py restores it). New names: seedprobe6_a_drec, seedprobe6_b_drec. Host link: none.
Needs: 3ds/tools/seedprobe.py has --write
## Exact edits
1. New azt.d/SEEDPROBE6.json: copies of spot_mac01_drec (azt.d/HOSTCORE2.json) with these `debug` fields and untils, cap 40, deadline 35,
   needs_picture false. bench.d/SEEDPROBE6.json: the same names, kind azahar_pic, tags azahar det drec. Commit them.
   - seedprobe6_a_drec: `warp=mac_01:0 pos=0:0:200 fxtest=82,96,85,97,73,25 loglevel=debug det drec=412:6:44`, until
     `drec f632 draws [1-9]\d* bytes \d+`.
   - seedprobe6_b_drec: `warp=mac_01:0 pos=0:0:200 fxtest=112,63 loglevel=debug det drec=412:2:44`, until
     `drec f456 draws [1-9]\d* bytes \d+`.
2. For each group: `python 3ds/tools/seedprobe.py <scene> <the seeds of the group> --write docs/handoff/row_SEEDS.md`:
   Group A (seedprobe6_a_drec):
   `src/effects/thunderbolt_ring.c:115@412|data->pos.y|data->pos.y + 20.0f`
   `src/effects/tubba_heart_attack.c:318@456|data->centerY|data->centerY + 20.0f`
   `src/effects/waterfall.c:136@500|data->unk_08|data->unk_08 + 20.0f`
   `src/effects/whirlwind.c:179@544|sp20, 0.0f, 0.0f|sp20, 0.0f, 0.0f + 20.0f`
   `src/effects/radiating_energy_orb.c:150@588|part->unk_0C|part->unk_0C + 20.0f`
   `src/effects/damage_indicator.c:208@632|part->basePos.y|part->basePos.y + 20.0f`
   Group B (seedprobe6_b_drec):
   `src/effects/peach_star_beam.c:211@412|data->pos.y|data->pos.y + 20.0f`
   `src/effects/snowfall.c:214@456|data->unk_0C|data->unk_0C + 20.0f`
3. Commit: `SEEDPROBE6: verdicts of 8 seeds (seedprobe.py)`.
## Gate (expected values)
1. Each seedprobe call prints one verdict line per seed and no tool-fault, bad-spec or no-seed-line line; `git status` shows no
   src file changed after each call; row_SEEDS.md changed only in the probed lines.
2. The verdicts are the result: no expected value. A dead seed goes back to the oracle as its row_SEEDS.md line (the dead-seed
   fix rule of row_TEMPLATE.md is for code rows, not for probe rows).
3. Budget 8 Azahar starts (each group: a base pair and 1 run). Counter: none moves (probe row for the batch rows).
## Stop and end
A tool-fault line, a bad-spec line, or a src file left changed: stop, commit, hand back the exact output. Delete this row file at
the end.
