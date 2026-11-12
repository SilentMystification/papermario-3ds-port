# Row S5MENU0: a det picture scene of the file select (gate scene for the S5MENU rows and S9GATE)

(Model: Haiku. Two json entries, no code. No blocker.)

## Facts (oracle 5, azt_scenarios.json on main 5c3865487)
- The file select scenes are census scenes only: fsel_fncov_idle (`fileselect fncov=400 loglevel=debug det`) and fsel_fncov_pick
  (`fileselect freshsave padscript=wait400,A*6 fncovrot=150 loglevel=debug det`). No file select scene stores shots, so no
  filemenu row (src/filemenu: filemenu_common.c 32 gbi calls, filemenu_selectlanguage.c 13, filemenu_msg.c 12) has a byte gate.
- A scene with `shotwin` takes shots only in its window (pm_gpu.c:1984-1990), so two runs of one build compare.

## Start
`sh 3ds/tools/wt_new.sh S5MENU0`. PM_ROW=S5MENU0 PM_LANE=S5MENU0 on every call.

## Touch only
3ds/tools/azt_scenarios.json and 3ds/tools/bench_defs.json: one entry `fsel_det_pic` each, right after `fsel_fncov_pick`.

## Exact edits
1. Scenario `fsel_det_pic`: a copy of fsel_fncov_idle with debug `fileselect freshsave shots shotwin=300:6 loglevel=debug det`,
   until `shot \d+ frame 305\b`, cap 30, deadline 25, expected_events: the entries of fsel_fncov_idle that are not the fncov
   line, plus `shot \d+ frame 305\b`. Subject: "File select at idle (fresh save), det, shots 300 to 305."
2. Bench `fsel_det_pic`: kind azahar_pic, scenario fsel_det_pic, cap 30, the same until, tags azahar, pic. Purpose: "S5MENU byte
   gate: the file select picture, det."
3. One commit for both files.

## Gate (expected values)
- `PM_ROW=S5MENU0 PM_LANE=S5MENU0 python 3ds/tools/bench.py base fsel_det_pic` (2 runs of the merge-base build; they must agree):
  stored, 6 shots. Two runs that differ: stop and hand back the differing shot names (the scene is then not det; the oracle
  changes the window). No code build is needed beyond the base build. Budget 3 Azahar starts.

## Stop
A base that is not stored: stop, commit, hand back the output. Delete this row file in the completing commit.
