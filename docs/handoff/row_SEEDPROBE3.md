# Row SEEDPROBE3: probe 12 seeds of row_SEEDS.md with seedprobe.py (2 groups, one run each)

Blocked by SEEDTOOL (seedprobe.py --write) and DRECSTEP (step windows with frame lines). Starts after the oracle read the SEEDPROBE1 pilot.
(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. ORACLE8 2026-11-13.)
## Facts (ORACLE8, main d13d0f663)
- fxtest runs `fxtest=a,b,...` one effect per 44 frames from frame 400 (pm_fxtest.c:179-180), so effect i draws at 412 + 44 i.
- Each seed: + 20.0f on the y argument of the first unguarded guTranslateF or guPositionF of <file>_appendGfx (row_SEEDS.md rule).
Start: `sh 3ds/tools/wt_new.sh SEEDPROBE3`. PM_ROW=SEEDPROBE3 PM_LANE=SEEDPROBE3 on every call.
## Touch only (places)
docs/handoff/row_SEEDS.md (the verdict field of these 12 lines, written by seedprobe.py --write). New azt.d/SEEDPROBE3.json and
bench.d/SEEDPROBE3.json. No source file stays changed (seedprobe.py restores it). New names: seedprobe3_a_drec, seedprobe3_b_drec. Host link: none.
Needs: 3ds/tools/seedprobe.py has --write
## Exact edits
1. New azt.d/SEEDPROBE3.json: copies of spot_mac01_drec (azt.d/HOSTCORE2.json) with these `debug` fields and untils, cap 40, deadline 35,
   needs_picture false. bench.d/SEEDPROBE3.json: the same names, kind azahar_pic, tags azahar det drec. Commit them.
   - seedprobe3_a_drec: `warp=mac_01:0 pos=0:0:200 fxtest=70,91,14,65,44,106 loglevel=debug det drec=412:6:44`, until
     `drec f632 draws [1-9]\d* bytes \d+`.
   - seedprobe3_b_drec: `warp=mac_01:0 pos=0:0:200 fxtest=103,121,114,1,47,77 loglevel=debug det drec=412:6:44`, until
     `drec f632 draws [1-9]\d* bytes \d+`.
2. For each group: `python 3ds/tools/seedprobe.py <scene> <the seeds of the group> --write docs/handoff/row_SEEDS.md`:
   Group A (seedprobe3_a_drec):
   `src/effects/floating_rock.c:121@412|data->pos.y|data->pos.y + 20.0f`
   `src/effects/fright_jar.c:130@456|data->unk_08|data->unk_08 + 20.0f`
   `src/effects/gather_energy_pink.c:179@500|part->posB.y|part->posB.y + 20.0f`
   `src/effects/gather_magic.c:168@544|part->unk_0C|part->unk_0C + 20.0f`
   `src/effects/green_impact.c:168@588|part->unk_08|part->unk_08 + 20.0f`
   `src/effects/hieroglyphs.c:130@632|data->pos.y|data->pos.y + 20.0f`
   Group B (seedprobe3_b_drec):
   `src/effects/huff_puff_breath.c:141@412|data->pos.y|data->pos.y + 20.0f`
   `src/effects/ice_pillar.c:165@456|data->pos.y|data->pos.y + 20.0f`
   `src/effects/ice_shard.c:147@500|data->pos.y|data->pos.y + 20.0f`
   `src/effects/landing_dust.c:290@544|part->y|part->y + 20.0f`
   `src/effects/light_rays.c:295@588|part->pos.y|part->pos.y + 20.0f`
   `src/effects/merlin_house_stars.c:144@632|data->unk_10|data->unk_10 + 20.0f`
3. Commit: `SEEDPROBE3: verdicts of 12 seeds (seedprobe.py)`.
## Gate (expected values)
1. Each seedprobe call prints one verdict line per seed and no tool-fault, bad-spec or no-seed-line line; `git status` shows no
   src file changed after each call; row_SEEDS.md changed only in the probed lines.
2. The verdicts are the result: no expected value. A dead seed goes back to the oracle as its row_SEEDS.md line (the dead-seed
   fix rule of row_TEMPLATE.md is for code rows, not for probe rows).
3. Budget 8 Azahar starts (each group: a base pair and 1 run). Counter: none moves (probe row for the batch rows).
## Stop and end
A tool-fault line, a bad-spec line, or a src file left changed: stop, commit, hand back the exact output. Delete this row file at
the end.
