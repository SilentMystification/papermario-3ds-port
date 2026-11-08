# Row CLEANB: old test builds deleted, and a life cycle that keeps them few

## User facts
User (2026-11-08): "clean up all of the builds in the azahar directory. Why do we have so many? ... at most we should only have a few ... old outdated test builds should be deleted when they're no longer needed. A part of the total life cycle should be making sure we dont have tons of old builds."
Orchestrator count (2026-11-08):
- %APPDATA%\Azahar\sdmc\3ds\PaperMario: 187 .3dsx files (7.0 GB folder). Cause: run_azahar.ps1:70 copies each lane build as pm_3ds_<lane>.3dsx into that folder; nothing deletes it when the lane or worktree goes (wt_gc.sh removes worktree, lane folder and docker volumes only).
- build3ds/azslots/slot1/2/3/user/sdmc/3ds/PaperMario: 38 + 34 + 13 .3dsx (2.4 GB with the rest of the slots).
- 3ds/logs/builds: 121 files, 3.7 GB (push_3ds.sh:78-92 archives every pushed 3dsx plus .elf/.map; build.sh:103-104 copies elf/map per build). User rule (memory): no archived builds for history; old numbers come from a rebuild at that commit.
- build3ds/azt/shots: 1.2 GB.

## Goal
1. Clean now: delete every pm_3ds_<x>.3dsx in the Azahar sdmc folder and the slot sdmc folders whose lane has no live worktree (keep pm_3ds.3dsx, and the lane builds of the worktrees in build3ds/wt/* that exist now). Delete 3ds/logs/builds files except the last 3 pushes (keep pushed_builds.txt lines). Delete shot folders of scenarios older than 7 days, keep the newest run per scenario.
2. Life cycle (one place each, no second path):
   - wt_gc.sh, when it removes a worktree or lane, also deletes pm_3ds_<lane>.3dsx in %APPDATA%\Azahar\sdmc\3ds\PaperMario and in each build3ds/azslots/slot*/user/sdmc/3ds/PaperMario, and build3ds/azt/shots/*/<lane>.
   - push_3ds.sh keeps at most 3 archived pushes in 3ds/logs/builds (deletes the oldest). build.sh does not keep per-build elf/map copies beyond the last 3.
   - A dry run lists what it would delete (wt_gc.sh without --apply already does this; keep that pattern).
3. Report the disk use before and after.

## Start state
Worktree build3ds/wt/cleanb, branch cleanb from nds1 (current). Tools only. Live worktrees now: bt1, pond, prof2, bstale (and any build3ds/wt/* that exists when you run): never delete their lane builds.

## Touch only
3ds/tools/wt_gc.sh, 3ds/push_3ds.sh, 3ds/build.sh (only the elf/map copy lines 100-105), docs/handoff/orchestrator_rules.md (merge step 5 text). Files deleted: only the build copies, archives and shot folders named above. Never delete save.flash, options.ini, log files, pad scripts, tour lists or any file that is not a .3dsx/.elf/.map/shot.

## Done criteria
1. Dry run printed and saved to the status file before any delete.
2. After the clean: the Azahar sdmc folder holds pm_3ds.3dsx plus at most the live lanes; 3ds/logs/builds holds at most 3 pushes. Disk numbers before/after reported.
3. wt_gc.sh dry run on a test lane name shows the new delete lines; push_3ds.sh --dry shows the archive cap.
Affected scenarios: none (no Azahar start).

## Rules
CLAUDE.md rules 7, 9, 11. No git stash. No stubs. Do not stop Azahar if it runs; skip a locked file and report it.

## At the end
Delete this row file and .wt_keep in the last commit. SendMessage main "cleanb ready <hash>".
