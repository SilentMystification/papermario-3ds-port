# Row SEEDPROBE9: probe 11 seeds of row_SEEDS.md with seedprobe.py (2 groups, one run each)

Blocked by SEEDTOOL (seedprobe.py --write) and DRECSTEP (step windows with frame lines). Starts after the oracle read the SEEDPROBE1 pilot and after SEEDPROBE6 merged (adjacent lines).
(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. ORACLE8 2026-11-13.)
## Facts (ORACLE8, main 8b274c6f5)
- fxtest runs `fxtest=a,b,...` one effect per 44 frames from frame 400 (pm_fxtest.c:179-180), so effect i draws at 412 + 44 i.
- Each seed: + 20.0f on the y argument of the first unguarded guTranslateF or guPositionF of <file>_appendGfx (row_SEEDS.md rule).
Start: `sh 3ds/tools/wt_new.sh SEEDPROBE9`. PM_ROW=SEEDPROBE9 PM_LANE=SEEDPROBE9 on every call.
## Touch only (places)
docs/handoff/row_SEEDS.md (the verdict field of these 11 lines, written by seedprobe.py --write). New azt.d/SEEDPROBE9.json and
bench.d/SEEDPROBE9.json. No source file stays changed (seedprobe.py restores it). New names: seedprobe9_a_drec, seedprobe9_b_drec. Host link: none.
Needs: 3ds/tools/seedprobe.py has --write
## Exact edits
1. New azt.d/SEEDPROBE9.json: copies of spot_mac01_drec (azt.d/HOSTCORE2.json) with these `debug` fields and untils, cap 40, deadline 35,
   needs_picture false. bench.d/SEEDPROBE9.json: the same names, kind azahar_pic, tags azahar det drec. Commit them.
   - seedprobe9_a_drec: `warp=mac_01:0 pos=0:0:200 fxtest=17,32,117,23,115,123 loglevel=debug det drec=412:6:44`, until
     `drec f632 draws [1-9]\d* bytes \d+`.
   - seedprobe9_b_drec: `warp=mac_01:0 pos=0:0:200 fxtest=28,41,80,122,2 loglevel=debug det drec=412:5:44`, until
     `drec f588 draws [1-9]\d* bytes \d+`.
2. For each group: `python 3ds/tools/seedprobe.py <scene> <the seeds of the group> --write docs/handoff/row_SEEDS.md`:
   Group A (seedprobe9_a_drec):
   `src/effects/shattering_stones.c:154@412|part->unk_04|part->unk_04 + 20.0f`
   `src/effects/shockwave.c:327@456|data->pos.y|data->pos.y + 20.0f`
   `src/effects/something_rotating.c:272@500|part->unk_1C|part->unk_1C + 20.0f`
   `src/effects/spiky_white_aura.c:190@544|part->unk_08|part->unk_08 + 20.0f`
   `src/effects/spirit_card.c:143@588|data->pos.y|data->pos.y + 20.0f`
   `src/effects/star_spirits_energy.c:361@632|sp20, 0.0f, 0.0f, 0.0f, data->unk_40[i], 0.0f, 0.0f|sp20, 0.0f, 0.0f, 0.0f, data->unk_40[i], 0.0f, 0.0f + 20.0f`
   Group B (seedprobe9_b_drec):
   `src/effects/stars_burst.c:158@412|part->unk_08|part->unk_08 + 20.0f`
   `src/effects/stars_orbiting.c:120@456|part->pos.y|part->pos.y + 20.0f`
   `src/effects/stat_change.c:249@500|data->pos.y|data->pos.y + 20.0f`
   `src/effects/sun.c:176@544|mtx, 0.0f, 0.0f|mtx, 0.0f, 0.0f + 20.0f`
   `src/effects/walking_dust.c:157@588|data->unk_0C|data->unk_0C + 20.0f`
3. Commit: `SEEDPROBE9: verdicts of 11 seeds (seedprobe.py)`.
## Gate (expected values)
1. Each seedprobe call prints one verdict line per seed and no tool-fault, bad-spec or no-seed-line line; `git status` shows no
   src file changed after each call; row_SEEDS.md changed only in the probed lines.
2. The verdicts are the result: no expected value. A dead seed goes back to the oracle as its row_SEEDS.md line (the dead-seed
   fix rule of row_TEMPLATE.md is for code rows, not for probe rows).
3. Budget 8 Azahar starts (each group: a base pair and 1 run). Counter: none moves (probe row for the batch rows).
## Stop and end
A tool-fault line, a bad-spec line, or a src file left changed: stop, commit, hand back the exact output. Delete this row file at
the end.
