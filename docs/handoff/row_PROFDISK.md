# Row PROFDISK: console prof recipes write the prof lines to log.txt

(Model: Haiku. Base: main. Edits 3ds/tools/bench_defs.json recipes only. Reads this file and CLAUDE.md.)

## Fact (oracle code read)
- The full log.txt of the console push (prof profrot fxtest=all) has 0 pmx lines while the live stream had 19: prof lines go to the file
  only with the switch `profdisk=1` (3ds/src/pm_log.c:248, U2: "profiling to log.txt, 0 off (default), 1 on"). The Azahar harness adds
  it (the pmu_rot_az switches line shows `profdisk=1`); the console recipes do not. Not a port fault: a recipe gap.

## Start
`sh 3ds/tools/wt_new.sh PROFDISK`. PM_ROW=PROFDISK.

## Exact edits
1. bench_defs.json: in the recipe of every bench with kind console_prof whose name is pmu_rot_prof or fxcen_all_prof, append ` profdisk=1`.
2. `python 3ds/tools/bench.py rebuild`; `bench.py show pmu_rot_prof` and `bench.py show fxcen_all_prof` print the push command with profdisk=1.

## Gate
`git diff main --stat`: only bench_defs.json. No run.

## End
Delete this row file in the completing commit. Hand back the hash and the two printed push commands.
