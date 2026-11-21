# ws_DGB0 (Haiku): dgb_01 broken-bridge census pair and the console capture check (design: oracle DGB01, plan_dgb01.md)
Worktree build3ds/wt/DGB0, branch DGB0. PM_ROW=DGB0 PM_LANE=DGB0. Start: main 3a29e12504 or later. No source code changes.
Objective: count what the broken bridges add per frame on dgb_01. Story -28 shows group g240 (84 models), story -16 shows g340 (16 models) (bridges.c:189-194).
Counters: none move (census). Endgame C1-C12: none. The hand-back decides the next row by plan_dgb01.md section 4 rules D1 to D4.
## Files (one commit: `DGB0 1: scenario and bench pair`). Edit shape "a new row registry file": copy the form of 3ds/tools/azt.d/MTX0.json and 3ds/tools/bench.d/MTX0.json.
1. New 3ds/tools/azt.d/DGB0.json: `{"scenarios": [A, B]}`. Scenario A (write it as below). Scenario B is A with these 4 changes only:
   name `dgb01_whole_prof`, `story=-16` in debug, `sp-16` in the scene line regex, subject and reason say story -16 (STORY_CH3_DEFEATED_TUBBA_BLUBBA, bridges intact).
```
{"name": "dgb01_broken_prof", "status": "unverified",
 "reason": "DGB0 census (plan_dgb01.md): dgb_01 with the broken bridges. Pair with dgb01_whole_prof. Counts only; the console decides the ms.",
 "subject": "dgb_01 Great Hall, entry 1, Mario placed at pos 80 0 144, story -28 (STORY_CH3_TUBBA_SMASHED_THE_BRIDGES: g240 shown, g340 hidden), fps60, prof=30, det. Prof window 2.",
 "proves": "DGB0: model records, PICA draws, triangles, legacy records, model and texture misses per step, key walks of the broken state.",
 "debug": "warp=dgb_01:1 story=-28 pos=80:0:144 fps60 prof=30 det", "extra": ["-Turbo"],
 "until": "prof f([6-9]\\d\\d|[1-9]\\d{3,}) w\\d+ at dgb_01 ", "cap": 45, "deadline": 40,
 "first_marker": "prof=1 log=off", "expects": "none", "needs_picture": false,
 "expected_events": [{"name": "prof on", "regex": "prof=1 log=off"},
                     {"name": "scene line", "regex": "prof f\\d+ w\\d+ at dgb_01 15:1 m\\d+ sp-28 "}]}
```
2. New 3ds/tools/bench.d/DGB0.json: `{"benches": [...]}` with 2 benches, `dgb01_broken_prof` and `dgb01_whole_prof`. Each: kind `azahar_stream`, speed `turbo`, default false,
   scenario = its own name, cap 45, until = the scenario until, tags `["azahar", "prof", "dgb01"]`, gates `[]`, purpose one sentence. The same log_metrics in both (pick `last`, kc_walk `first`):
   ndc_model `prof f\d+ w\d+ ndc model ([0-9.]+) spr` | ndc_ent `prof f\d+ w\d+ ndc model [0-9.]+ spr [0-9.]+ ent ([0-9.]+) call` | pipe_draws `prof f\d+ w\d+ pipe fr \d+ draws (\d+) kick`
   nd_dr `prof f\d+ w\d+ nd [0-9.]+ lg [0-9.]+ dr (\d+) tc` | nd_tri `prof f\d+ w\d+ nd [0-9.]+ lg [0-9.]+ dr \d+ tc \d+ tw \d+ tri (\d+) leg` | nd_leg `prof f\d+ w\d+ nd .*? tri \d+ leg (\d+) gb`
   sx_m `prof f\d+ w\d+ stepx norm n \d+ g \d+ p \d+ d \d+ r \d+ s \d+ t \d+ m (\d+) a` | sx_t `prof f\d+ w\d+ stepx norm n \d+ g \d+ p \d+ d \d+ r \d+ s \d+ t (\d+) m`
   cls_rec `prof f\d+ w\d+ cls rec ([0-9.]+) ent` | cls_sub `prof f\d+ w\d+ cls rec .*? sub ([0-9.]+) kick` | c0r_hit `prof f\d+ w\d+ c0r key [0-9.]+ mtx [0-9.]+ hit ([0-9.]+)`
   c1d_rmdl `prof f\d+ w\d+ c1d rmdl ([0-9.]+) rent` | kc_walk `kcen f\d+ steps 300 kcopy \d+ kwalk (\d+)`. In JSON each `\` is written `\\`.
   Line sources (FACT): pm_phase.c:719 (ndc), :677 (pipe), :730 (nd), :688 (stepx), :644 (cls), :671 (c0r), :655 (c1d), :553 (scene line), pm_main.c:533 (kcen).
## Tests (output to build3ds/out/, read the last 20 lines). Budget: 1 build, 4 Azahar starts (8 max).
1. `PM_LANE=DGB0 sh 3ds/build.sh` (memcheck OK). `sh 3ds/tools/nd_host_test.sh` (timeout 600000): PASS. `python 3ds/tools/c0gate.py <elf>`: 0 / 0 / 0.
2. `python 3ds/tools/bench.py run dgb01_broken_prof --row DGB0 --why "DGB0 broken census"`, then the same for dgb01_whole_prof (`--why "DGB0 intact census"`).
   Pass: each run reaches the until line, the scene line has `sp-28` (or `sp-16`), and all 13 metrics parse.
3. `python 3ds/tools/capture_check.py --row DGB0 --proof "at dgb_01 15:1,sp-28,ndc model,stepx norm" --profile prof warp=dgb_01:1 story=-28 pos=80:0:144 profdisk=1`,
   then the same with `story=-16` and `sp-16`. Pass: `CAPTURE CHECK OK` twice.
Fix rights: a metric regex that does not match: copy the real line text from the run log into the regex (mechanical, note it in the commit). A JSON syntax error.
Stop on a tool fault (CLAUDE.md 4): an until line not reached in 45 s, an abnormal exit, a CAPTURE CHECK FAIL or TOOL FAULT. Do not raise the cap. Send the output path to the oracle.
Stop: the commit `DGB0 2: census` deletes this page. Hand back with ONE SubagentHandback:
commit hashes, host and c0gate lines, a table of the 13 metrics (broken, whole, broken minus whole), the 2 capture_check lines, the 2 run log paths.
