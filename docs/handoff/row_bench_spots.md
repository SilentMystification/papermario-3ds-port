# Row BENCH4: three test spots, build-at, optional archive (keep the tools small)

## Goal
Fixed test spots replace the old default spot. Any commit can be measured by command. A push never fails on archiving. Keep bench.py small: no new features beyond this list.

## Start state
Branch nds1. Python: /c/Python314/python. Tools and docs only. No game source. No console push. Never touch or commit 3ds/src/map_restore_table.c. Another session commits game source in this folder: stage and commit only your own files (git add <file>, git commit -- <file>), check git status first. Read first: docs/handoff/test_catalog.md (the process), 3ds/tools/bench_defs.json, 3ds/tools/bench.py, 3ds/push_3ds.sh, 3ds/build.sh (how it picks the source folder).

## Do
1. Spots (user list, entry 0): mac_01 pos 0:0:200, mac_00 pos -135:0:85, sbk_56 pos 20:0:440. For each spot add two benches to bench_defs.json: an Azahar picture bench (azahar_pic, kind per the existing seam_pic bench, warp=<map>:0 pos=x:y:z, cap and until-line from an existing scenario that prints the scene line) and a console prof bench (console_prof, recipe: `warp=<map>:0 pos=x:y:z prof=4`, no stat switches in the recipe; a bench may name extra switches only in its own definition). Add the Azahar scenarios to azt_scenarios.json (cap, until-line the build prints, no idle). Mark toad_standing and the other mac_01 pos -264 20 -326 recipe as `default: false`; the three spots are the defaults (`bench.py list` shows them first). Names: spot_mac01_pic, spot_mac01_prof, spot_mac00_pic, spot_mac00_prof, spot_sbk56_pic, spot_sbk56_prof.
2. Dirty flag: every run line gets `dirty` (true when `git status --porcelain` shows modified tracked files other than 3ds/src/map_restore_table.c and 3ds/logs/). Add it to the run table. Old runs: null.
3. `bench.py build-at <commit> <bench> --row R`: build that commit and run the bench on it WITHOUT changing the working tree: `git archive <commit>` into a scratch folder under build3ds/at_<short>/, copy the user's map_restore_table.c as the generated file the build needs (run gen_map_restore.py against the scratch copy), build there, run the bench, record the run with that commit. If 3ds/build.sh cannot build from another folder, say exactly why in the report and implement it with an env var or argument for the source root (a small edit to 3ds/build.sh). Do not use git checkout, git stash, git reset or worktrees in the main folder. Verify with md5sum 3ds/src/map_restore_table.c before and after (md5 437b90efe5f32b82394ce2ece094210e or the value before your start). Delete the scratch folder at the end.
4. push_3ds.sh: remove the refusal on ARCHIVE FAILED. Archiving to 3ds/logs/builds/ becomes best effort (a failed archive prints one warning line and the push goes on). Update the Console section of docs/handoff/orchestrator_rules.md to match (archive optional).
5. Update docs/handoff/test_catalog.md (situations D and E and the command reference) for the three spots, build-at and the dirty field. Keep it the same length.

## Done criteria (all must pass; handback only then)
1. `bench.py list` shows the six spot benches as defaults. `bench.py rebuild` clean.
2. The three picture benches each run once via `bench.py run` and end log_pass, with a shot in build3ds/azt/shots/<scenario>/ that shows the map. Report the three shot paths and say in one line what each shot shows (open the files).
3. `bench.py build-at <an older commit of 10 to 30 commits back> spot_mac01_pic --row BENCH4` records a run with that commit and dirty false, and `git status` plus the map_restore_table.c md5 are unchanged afterwards.
4. push_3ds.sh passes `sh -n 3ds/push_3ds.sh` and the archive failure path no longer exits (show the changed lines).
5. Commit only your files. No attribution. No push. Delete this row file in the last commit.

## Blockers (treat as work)
- A spot position puts Mario in a wall or void: the picture bench still records the scene line. Report the shot, do not change the spot.
- build.sh hard-codes the source folder: see item 3.
- Docker not running for the build: start the build the way 3ds/build.sh does and report the exact error; the picture benches (item 2) still count.

## At the end
Overwrite the BENCH3 STATE block at the top of docs/handoff/agent_bench1.md with a BENCH4 STATE block (10 lines or fewer). Do not append.
