# Row SEEDPROBE5: probe 12 seeds of row_SEEDS.md with seedprobe.py (2 groups, one run each)

Blocked by SEEDTOOL (seedprobe.py --write) and DRECSTEP (step windows with frame lines). Starts after the oracle read the SEEDPROBE1 pilot.
(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. ORACLE8 2026-11-13.)
## Facts (ORACLE8, main d13d0f663)
- fxtest runs `fxtest=a,b,...` one effect per 44 frames from frame 400 (pm_fxtest.c:179-180), so effect i draws at 412 + 44 i.
- Each seed: + 20.0f on the y argument of the first unguarded guTranslateF or guPositionF of <file>_appendGfx (row_SEEDS.md rule).
Start: `sh 3ds/tools/wt_new.sh SEEDPROBE5`. PM_ROW=SEEDPROBE5 PM_LANE=SEEDPROBE5 on every call.
## Touch only (places)
docs/handoff/row_SEEDS.md (the verdict field of these 12 lines, written by seedprobe.py --write). New azt.d/SEEDPROBE5.json and
bench.d/SEEDPROBE5.json. No source file stays changed (seedprobe.py restores it). New names: seedprobe5_a_drec, seedprobe5_b_drec. Host link: none.
Needs: 3ds/tools/seedprobe.py has --write
## Exact edits
1. New azt.d/SEEDPROBE5.json: copies of spot_mac01_drec (azt.d/HOSTCORE2.json) with these `debug` fields and untils, cap 40, deadline 35,
   needs_picture false. bench.d/SEEDPROBE5.json: the same names, kind azahar_pic, tags azahar det drec. Commit them.
   - seedprobe5_a_drec: `warp=mac_01:0 pos=0:0:200 fxtest=24,9,90,12,83,29 loglevel=debug det drec=412:6:44`, until
     `drec f632 draws [1-9]\d* bytes \d+`.
   - seedprobe5_b_drec: `warp=mac_01:0 pos=0:0:200 fxtest=39,108,40,92,35,94 loglevel=debug det drec=412:6:44`, until
     `drec f632 draws [1-9]\d* bytes \d+`.
2. For each group: `python 3ds/tools/seedprobe.py <scene> <the seeds of the group> --write docs/handoff/row_SEEDS.md`:
   Group A (seedprobe5_a_drec):
   `src/effects/smoke_impact.c:138@412|part->unk_08|part->unk_08 + 20.0f`
   `src/effects/snowflake.c:130@456|data->pos.y|data->pos.y + 20.0f`
   `src/effects/snowman_doll.c:302@500|data->unk_08|data->unk_08 + 20.0f`
   `src/effects/sparkles.c:282@544|part->unk_08|part->unk_08 + 20.0f`
   `src/effects/squirt.c:179@588|0.0f|0.0f + 20.0f`
   `src/effects/stars_shimmer.c:299@632|data->pos.y|data->pos.y + 20.0f`
   Group B (seedprobe5_b_drec):
   `src/effects/stars_spread.c:134@412|part->unk_08|part->unk_08 + 20.0f`
   `src/effects/static_status.c:204@456|part->pos.y|part->pos.y + 20.0f`
   `src/effects/steam_burst.c:127@500|part->unk_08|part->unk_08 + 20.0f`
   `src/effects/stop_watch.c:196@544|data->unk_08|data->unk_08 + 20.0f`
   `src/effects/sweat.c:92@588|data->pos.y|data->pos.y + 20.0f`
   `src/effects/throw_spiny.c:172@632|data->pos.y|data->pos.y + 20.0f`
3. Commit: `SEEDPROBE5: verdicts of 12 seeds (seedprobe.py)`.
## Gate (expected values)
1. Each seedprobe call prints one verdict line per seed and no tool-fault, bad-spec or no-seed-line line; `git status` shows no
   src file changed after each call; row_SEEDS.md changed only in the probed lines.
2. The verdicts are the result: no expected value. A dead seed goes back to the oracle as its row_SEEDS.md line (the dead-seed
   fix rule of row_TEMPLATE.md is for code rows, not for probe rows).
3. Budget 8 Azahar starts (each group: a base pair and 1 run). Counter: none moves (probe row for the batch rows).
## Stop and end
A tool-fault line, a bad-spec line, or a src file left changed: stop, commit, hand back the exact output. Delete this row file at
the end.
