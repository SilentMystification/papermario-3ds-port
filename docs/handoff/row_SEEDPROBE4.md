# Row SEEDPROBE4: probe 12 seeds of row_SEEDS.md with seedprobe.py (2 groups, one run each)

Blocked by SEEDTOOL (seedprobe.py --write) and DRECSTEP (step windows with frame lines). Starts after the oracle read the SEEDPROBE1 pilot.
(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. ORACLE8 2026-11-13.)
## Facts (ORACLE8, main d13d0f663)
- fxtest runs `fxtest=a,b,...` one effect per 44 frames from frame 400 (pm_fxtest.c:179-180), so effect i draws at 412 + 44 i.
- Each seed: + 20.0f on the y argument of the first unguarded guTranslateF or guPositionF of <file>_appendGfx (row_SEEDS.md rule).
Start: `sh 3ds/tools/wt_new.sh SEEDPROBE4`. PM_ROW=SEEDPROBE4 PM_LANE=SEEDPROBE4 on every call.
## Touch only (places)
docs/handoff/row_SEEDS.md (the verdict field of these 12 lines, written by seedprobe.py --write). New azt.d/SEEDPROBE4.json and
bench.d/SEEDPROBE4.json. No source file stays changed (seedprobe.py restores it). New names: seedprobe4_a_drec, seedprobe4_b_drec. Host link: none.
Needs: 3ds/tools/seedprobe.py has --write
## Exact edits
1. New azt.d/SEEDPROBE4.json: copies of spot_mac01_drec (azt.d/HOSTCORE2.json) with these `debug` fields and untils, cap 40, deadline 35,
   needs_picture false. bench.d/SEEDPROBE4.json: the same names, kind azahar_pic, tags azahar det drec. Commit them.
   - seedprobe4_a_drec: `warp=mac_01:0 pos=0:0:200 fxtest=109,124,26,120,78,72 loglevel=debug det drec=412:6:44`, until
     `drec f632 draws [1-9]\d* bytes \d+`.
   - seedprobe4_b_drec: `warp=mac_01:0 pos=0:0:200 fxtest=98,31,52,102,67,34 loglevel=debug det drec=412:6:44`, until
     `drec f632 draws [1-9]\d* bytes \d+`.
2. For each group: `python 3ds/tools/seedprobe.py <scene> <the seeds of the group> --write docs/handoff/row_SEEDS.md`:
   Group A (seedprobe4_a_drec):
   `src/effects/moving_cloud.c:190@412|data->unk_08|data->unk_08 + 20.0f`
   `src/effects/pink_sparkles.c:251@456|part->unk_08|part->unk_08 + 20.0f`
   `src/effects/purple_ring.c:204@500|data->unk_08|data->unk_08 + 20.0f`
   `src/effects/quizmo_assistant.c:112@544|data->pos.y|data->pos.y + 20.0f`
   `src/effects/quizmo_audience.c:194@588|data->pos.y|data->pos.y + 20.0f`
   `src/effects/quizmo_stage.c:118@632|data->origin.y|data->origin.y + 20.0f`
   Group B (seedprobe4_b_drec):
   `src/effects/red_impact.c:175@412|part->unk_08|part->unk_08 + 20.0f`
   `src/effects/ring_blast.c:102@456|data->pos.y|data->pos.y + 20.0f`
   `src/effects/shimmer_wave.c:202@500|part->unk_08|part->unk_08 + 20.0f`
   `src/effects/shiny_flare.c:113@544|data->pos.y|data->pos.y + 20.0f`
   `src/effects/small_gold_sparkle.c:126@588|part->unk_0C|part->unk_0C + 20.0f`
   `src/effects/smoke_burst.c:117@632|data->pos.y|data->pos.y + 20.0f`
3. Commit: `SEEDPROBE4: verdicts of 12 seeds (seedprobe.py)`.
## Gate (expected values)
1. Each seedprobe call prints one verdict line per seed and no tool-fault, bad-spec or no-seed-line line; `git status` shows no
   src file changed after each call; row_SEEDS.md changed only in the probed lines.
2. The verdicts are the result: no expected value. A dead seed goes back to the oracle as its row_SEEDS.md line (the dead-seed
   fix rule of row_TEMPLATE.md is for code rows, not for probe rows).
3. Budget 8 Azahar starts (each group: a base pair and 1 run). Counter: none moves (probe row for the batch rows).
## Stop and end
A tool-fault line, a bad-spec line, or a src file left changed: stop, commit, hand back the exact output. Delete this row file at
the end.
