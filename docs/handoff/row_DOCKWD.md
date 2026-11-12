# Row DOCKWD: docker health check per build and a scripted restart when no build runs (tool, no game code)

Model: Haiku (two shell functions and test cases in the existing BUILDLOCK host test; no design choice).

## Facts (oracle read)
- 3ds/build.sh:131 checks `timeout 20 docker info` once before build_slot_take, then exits 3 with a hand restart message.
- 3ds/tools/build_slot.sh: at most 3 slots (BUILD_SLOTS_N=3, build3ds/buildslots/slot<k>.d, holder pid file, build_slot_reap).
- Host test 3ds/tests/buildlock/run.sh uses a fake docker on PATH and BUILDSLOT_DIR.

## Start
`sh 3ds/tools/wt_new.sh DOCKWD`; PM_ROW=DOCKWD. No Azahar run, no game build.

## Files and places
3ds/tools/build_slot.sh (two new functions at the end), 3ds/build.sh (the line :131 only, and one call after build_slot_take),
3ds/tests/buildlock/run.sh (new cases at the end).

## Edits
1. build_slot.sh `build_docker_ok`: `timeout 20 docker info >/dev/null 2>&1`.
2. build_slot.sh `build_docker_restart`: only when no live slot exists (every slot<k>.d absent or reaped by build_slot_reap); else
   return 1 with "docker restart skipped: <n> builds running". Restart: `powershell.exe -NoProfile -Command "Stop-Process -Name
   'Docker Desktop' -Force -ErrorAction SilentlyContinue; Start-Process 'C:\Program Files\Docker\Docker\Docker Desktop.exe'"`
   (DOCKER_RESTART_CMD overrides it for the test), then build_docker_ok every 5 s up to 180 s; one line per try. 0 when it answers.
3. build.sh :131: `build_docker_ok || build_docker_restart || { the old BUILD FAULT line; exit 3; }` (source build_slot.sh before
   it). After build_slot_take: `build_docker_ok || { build_slot_release; echo "BUILD FAULT: docker stopped answering after the slot
   was taken" >&2; exit 3; }` (no restart while this build holds a slot).

## Host test cases (fake docker: `info` fails until a marker file exists; DOCKER_RESTART_CMD creates the marker)
(a) engine down, no slot held -> restart runs, build goes on; (b) engine down, one live slot held by a sleeping fake build -> no
restart, exit 3 with "builds running"; (c) engine up -> no restart call; (d) the old 5-wrapper case still passes. Expected:
the test prints PASS for every case and the old cases.

## Gate
`sh 3ds/tests/buildlock/run.sh`: all PASS. Then one real build in the worktree (`sh 3ds/build.sh`) succeeds with the engine up
(no restart line). Commit; delete this row file in the same commit.

## Stop
A case that fails twice: stop, commit, hand back the output.
