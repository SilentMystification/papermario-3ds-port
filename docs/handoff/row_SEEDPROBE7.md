# Row SEEDPROBE7: probe 12 seeds of row_SEEDS.md with seedprobe.py (2 groups, one run each)

Blocked by SEEDTOOL (seedprobe.py --write) and DRECSTEP (step windows with frame lines). Starts after the oracle read the SEEDPROBE1 pilot and after SEEDPROBE6 merged (adjacent lines).
(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. ORACLE8 2026-11-13.)
## Facts (ORACLE8, main 8b274c6f5)
- fxtest runs `fxtest=a,b,...` one effect per 44 frames from frame 400 (pm_fxtest.c:179-180), so effect i draws at 412 + 44 i.
- Each seed: + 20.0f on the y argument of the first unguarded guTranslateF or guPositionF of <file>_appendGfx (row_SEEDS.md rule).
Start: `sh 3ds/tools/wt_new.sh SEEDPROBE7`. PM_ROW=SEEDPROBE7 PM_LANE=SEEDPROBE7 on every call.
## Touch only (places)
docs/handoff/row_SEEDS.md (the verdict field of these 12 lines, written by seedprobe.py --write). New azt.d/SEEDPROBE7.json and
bench.d/SEEDPROBE7.json. No source file stays changed (seedprobe.py restores it). New names: seedprobe7_a_drec, seedprobe7_b_drec. Host link: none.
Needs: 3ds/tools/seedprobe.py has --write
## Exact edits
1. New azt.d/SEEDPROBE7.json: copies of spot_mac01_drec (azt.d/HOSTCORE2.json) with these `debug` fields and untils, cap 40, deadline 35,
   needs_picture false. bench.d/SEEDPROBE7.json: the same names, kind azahar_pic, tags azahar det drec. Commit them.
   - seedprobe7_a_drec: `warp=mac_01:0 pos=0:0:200 fxtest=76,49,27,45,13,18 loglevel=debug det drec=412:6:44`, until
     `drec f632 draws [1-9]\d* bytes \d+`.
   - seedprobe7_b_drec: `warp=mac_01:0 pos=0:0:200 fxtest=125,86,37,66,118,5 loglevel=debug det drec=412:6:44`, until
     `drec f632 draws [1-9]\d* bytes \d+`.
2. For each group: `python 3ds/tools/seedprobe.py <scene> <the seeds of the group> --write docs/handoff/row_SEEDS.md`:
   Group A (seedprobe7_a_drec):
   `src/effects/energy_orb_wave.c:269@412|data->pos.y|data->pos.y + 20.0f`
   `src/effects/fire_breath.c:214@456|data->pos.y|data->pos.y + 20.0f`
   `src/effects/flame.c:203@500|data->pos.y|data->pos.y + 20.0f`
   `src/effects/radial_shimmer.c:328@544|data->unk_14|data->unk_14 + 20.0f`
   `src/effects/shape_spell.c:251@588|data->pos.y|data->pos.y + 20.0f`
   `src/effects/smoke_ring.c:144@632|part->unk_08|part->unk_08 + 20.0f`
   Group B (seedprobe7_b_drec):
   `src/effects/star_outline.c:234@412|data->pos.y|data->pos.y + 20.0f`
   `src/effects/water_fountain.c:241@456|data->pos.y|data->pos.y + 20.0f`
   `src/effects/windy_leaves.c:248@500|part->unk_04.y|part->unk_04.y + 20.0f`
   `src/effects/attack_result_text.c:196@544|data->pos.y|data->pos.y + 20.0f`
   `src/effects/breaking_junk.c:146@588|data->pos.y|data->pos.y + 20.0f`
   `src/effects/cloud_puff.c:131@632|part->unk_1C|part->unk_1C + 20.0f`
3. Commit: `SEEDPROBE7: verdicts of 12 seeds (seedprobe.py)`.
## Gate (expected values)
1. Each seedprobe call prints one verdict line per seed and no tool-fault, bad-spec or no-seed-line line; `git status` shows no
   src file changed after each call; row_SEEDS.md changed only in the probed lines.
2. The verdicts are the result: no expected value. A dead seed goes back to the oracle as its row_SEEDS.md line (the dead-seed
   fix rule of row_TEMPLATE.md is for code rows, not for probe rows).
3. Budget 8 Azahar starts (each group: a base pair and 1 run). Counter: none moves (probe row for the batch rows).
## Stop and end
A tool-fault line, a bad-spec line, or a src file left changed: stop, commit, hand back the exact output. Delete this row file at
the end.
