# Row SEEDPROBE8: probe 12 seeds of row_SEEDS.md with seedprobe.py (2 groups, one run each)

Blocked by SEEDTOOL (seedprobe.py --write) and DRECSTEP (step windows with frame lines). Starts after the oracle read the SEEDPROBE1 pilot and after SEEDPROBE6 merged (adjacent lines).
(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. ORACLE8 2026-11-13.)
## Facts (ORACLE8, main 8b274c6f5)
- fxtest runs `fxtest=a,b,...` one effect per 44 frames from frame 400 (pm_fxtest.c:179-180), so effect i draws at 412 + 44 i.
- Each seed: + 20.0f on the y argument of the first unguarded guTranslateF or guPositionF of <file>_appendGfx (row_SEEDS.md rule).
Start: `sh 3ds/tools/wt_new.sh SEEDPROBE8`. PM_ROW=SEEDPROBE8 PM_LANE=SEEDPROBE8 on every call.
## Touch only (places)
docs/handoff/row_SEEDS.md (the verdict field of these 12 lines, written by seedprobe.py --write). New azt.d/SEEDPROBE8.json and
bench.d/SEEDPROBE8.json. No source file stays changed (seedprobe.py restores it). New names: seedprobe8_a_drec, seedprobe8_b_drec. Host link: none.
Needs: 3ds/tools/seedprobe.py has --write
## Exact edits
1. New azt.d/SEEDPROBE8.json: copies of spot_mac01_drec (azt.d/HOSTCORE2.json) with these `debug` fields and untils, cap 40, deadline 35,
   needs_picture false. bench.d/SEEDPROBE8.json: the same names, kind azahar_pic, tags azahar det drec. Commit them.
   - seedprobe8_a_drec: `warp=mac_01:0 pos=0:0:200 fxtest=6,19,59,93,11,61 loglevel=debug det drec=412:6:44`, until
     `drec f632 draws [1-9]\d* bytes \d+`.
   - seedprobe8_b_drec: `warp=mac_01:0 pos=0:0:200 fxtest=22,48,88,107,58,30 loglevel=debug det drec=412:6:44`, until
     `drec f632 draws [1-9]\d* bytes \d+`.
2. For each group: `python 3ds/tools/seedprobe.py <scene> <the seeds of the group> --write docs/handoff/row_SEEDS.md`:
   Group A (seedprobe8_a_drec):
   `src/effects/cloud_trail.c:132@412|part->unk_20|part->unk_20 + 20.0f`
   `src/effects/damage_stars.c:257@456|part->pos.y|part->pos.y + 20.0f`
   `src/effects/disable_x.c:219@500|data->pos.y|data->pos.y + 20.0f`
   `src/effects/effect_63.c:250@544|part->unk_08|part->unk_08 + 20.0f`
   `src/effects/emote.c:225@588|part->unk_08|part->unk_08 + 20.0f`
   `src/effects/firework.c:203@632|part->unk_08|part->unk_08 + 20.0f`
   Group B (seedprobe8_b_drec):
   `src/effects/got_item_outline.c:132@412|data->pos.y|data->pos.y + 20.0f`
   `src/effects/lightning.c:259@456|data->unk_08|data->unk_08 + 20.0f`
   `src/effects/lightning_bolt.c:212@500|sp10, 0.1f, 0.1f|sp10, 0.1f, 0.1f + 20.0f`
   `src/effects/misc_particles.c:318@544|particle->pos.y|particle->pos.y + 20.0f`
   `src/effects/recover.c:180@588|part->pos.y|part->pos.y + 20.0f`
   `src/effects/rising_bubble.c:132@632|data->pos.y|data->pos.y + 20.0f`
3. Commit: `SEEDPROBE8: verdicts of 12 seeds (seedprobe.py)`.
## Gate (expected values)
1. Each seedprobe call prints one verdict line per seed and no tool-fault, bad-spec or no-seed-line line; `git status` shows no
   src file changed after each call; row_SEEDS.md changed only in the probed lines.
2. The verdicts are the result: no expected value. A dead seed goes back to the oracle as its row_SEEDS.md line (the dead-seed
   fix rule of row_TEMPLATE.md is for code rows, not for probe rows).
3. Budget 8 Azahar starts (each group: a base pair and 1 run). Counter: none moves (probe row for the batch rows).
## Stop and end
A tool-fault line, a bad-spec line, or a src file left changed: stop, commit, hand back the exact output. Delete this row file at
the end.
