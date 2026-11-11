# Row TOOLB10: base --fill stores only warm byte scenes, mdl5_iwa10_det first

(Model: Haiku. The agent reads only this file, CLAUDE.md and what this file names. A tool row: no game source edit, no memcheck.)
## Facts (oracle code read, main 012ab5762)
- 3ds/tools/bench.py:1376 `BASE_FILL = ("spot_mac01_det", "battle_start_f182_cold", "mdl5_iwa10_det")`; base_fill (:1734-1758) runs
  them as one list through base_run on one kept slot (warm-up plus warm pairs, TOOLB9).
- battle_start_f182_cold is a cold scenario by design (-NoWarm): base_run can never pair two warm runs of it ("not both warm"), and two
  cold runs of one build differ (the cold-boot fault, row LOADMTX note). No base can exist for it, and it is not a byte gate
  (row_ORACLE_NEXT.md decisions). Its mentions as a gate in old rows (CORE2C, FFOREST, HC8, INTERIOR, PAUSEFLK) are void.
- mdl5_iwa10_det: every row's own base (mdl5 first, then spot_mac01_det) stored "two runs equal"; the fill, where mdl5 follows the cold
  battle start on the same slot, gave "two runs differ" (shot 0000_8_5) at 4bbd73043 and at 012ab5762. Game state does not survive a
  warm restart (it reboots the emulated system; the game writes the SD card only at an options close, pm_options.c:331-336, or a game
  save, and these scenarios do neither), so the difference comes from the slot after a cold start; the code read names no game path.
  The fill takes the order that the rows use.
## Start
`sh 3ds/tools/wt_new.sh TOOLB10`; PM_ROW=TOOLB10 PM_LANE=TOOLB10.
## Touch only
3ds/tools/bench.py: BASE_FILL (:1376) and the docstring line (:33). No other line.
## Exact edits
1. :1376 -> `BASE_FILL = ("mdl5_iwa10_det", "spot_mac01_det")  # TOOLB10: warm byte scenes, in the order the rows run them; battle_start_f182_cold is cold by design (no base)`.
2. :33: the list in the docstring -> `(mdl5_iwa10_det, spot_mac01_det)`.
## Gate
- `python -c "import ast,sys; ast.parse(open('3ds/tools/bench.py').read())"`: rc 0. `python 3ds/tools/bench.py run nd_host --row TOOLB10 --why "bench.py edit"`: PASS.
- The fill itself runs in the main folder after the merge (the orchestrator): `python 3ds/tools/bench.py base --fill`: both stored
  ("two runs equal"). If mdl5_iwa10_det still differs there: the orchestrator hands the line to the oracle.
## Stop and end
A failed gate or a tool fault: commit and hand back the exact output. Delete this row file in the completing commit.
