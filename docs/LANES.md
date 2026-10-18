# Lanes (parallel build lanes)

A lane is a git worktree with its own branch, build folder and Docker volume. Lane B exists now. Builds run in parallel. Azahar runs one at a time (mutex `Global\pm_azahar_run` in run_azahar.ps1).

## Lane B facts
- Folder: `C:\Users\Andrew\Documents\GitHub\papermario-3ds-port-laneB`. Branch `lane-b` (from main 019453ec0).
- `PM_LANE=laneB` selects: volume `pm3ds-work-laneB`, output folder `build3ds/lane-laneB/`, 3dsx name `pm_3ds_laneB.3dsx` on the Azahar SD card. build.sh does not copy to the real SD card in a lane.
- Empty `PM_LANE` = main behaviour, unchanged.
- The script changes (build.sh, run_azahar.ps1, go.ps1, tools/golden.py) are committed on `lane-b`. They reach main with the merge.
- Git-ignored inputs were copied into the lane folder: `assets/`, `ver/us/build/`, `ver/us/baserom.z64`, `ver/us/papermario.ld`, `ver/us/asm/data/`, `ver/us/.splat_cache/`, `3ds/ld/map_restore.ld` (about 141 MB). Copy them again for a new lane.

## Start an agent in lane B
1. Work only in the lane B folder. Never edit the main folder.
2. Build: `cd` to the lane folder, then `PM_LANE=laneB sh 3ds/build.sh`.
3. Run: `$env:PM_LANE="laneB"; powershell -File 3ds/run_azahar.ps1 -Turbo -Smoke -FaultStop -Debug "fast preset=yard"`.
4. If another run holds the mutex, the run waits (`-LockWait`, exit 3 on timeout).
5. Commit on `lane-b`. Append to PROGRESS.log only. Never push.

## Merge lane-b into main
1. In the main folder, commit or stash nothing of other agents. Use no `git checkout` on tracked files.
2. In the lane folder: `git rebase main` (or in main: `git cherry-pick <hashes>`).
3. In main: `git merge --ff-only lane-b` when main has no local edits in the touched files. Else cherry-pick.
4. Commit only own files. Never push. Rebuild main once and run the smoke check on `yard` and `toad`.
5. Shared docs (HANDOFF.md, PROGRESS.log, SLICES.md): append only. Fix conflicts by keeping both sides. Only one lane may regenerate maps.cmake, map_syms.lds and stub_include (S30b).

## Remove the lane
1. `git worktree remove --force ..\papermario-3ds-port-laneB` (main folder).
2. `git branch -d lane-b` (after the merge).
3. `docker volume rm pm3ds-work-laneB`.
4. Delete `pm_3ds_laneB.3dsx` from `%APPDATA%\Azahar\sdmc\3ds\PaperMario`.

## Cost
- Disk: about 215 MB for the folder (141 MB of copied inputs), about 0.7 GB for the volume, 22 MB for the build output.
- Time: first full build 233 s (Docker, 32 threads). A parallel build slows both builds. Later builds are incremental.
- Smoke run through the lane: 17 s, exit 0.

## Rules
- Only one Azahar run at a time. The mutex enforces it. Builds may run in parallel.
- Never touch `build3ds/` or volume `pm3ds-work` from lane B. Never touch `pm3ds-work-laneB` from main.
- Azahar settings (qt-config.ini, SD card, log) are still shared. Phase 3 of docs/PARALLEL_PLAN.md (own Azahar folder per lane) is not done.
