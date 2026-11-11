# Row BUILDLOCK: at most 3 Docker builds at once on this host (slot locks), and an engine check before a build

(Model: Haiku. Base: main. Tool only: 3ds/build.sh (around the build container run, :184-200), new 3ds/tools/build_slot.sh, new test
3ds/tests/buildlock/run.sh. Disjoint from every code row. Reads this file and CLAUDE.md.)

## Facts (oracle code read; user fact: the Docker engine died at 15:59 with five parallel builds, WSL capped at 10 GB, now 14 GB)
- build.sh runs one build container per lane (`docker run --rm --name "pm3ds-build-${PM_LANE:-main}"` with the volumes pm3ds-src-<lane>,
  pm3ds-build-<lane> and the shared pm3ds-ccache, :18-27, :194-200), after source syncs (:176-179, short containers) and with a watchdog
  that kills the container after PM_BUILD_CAP s (:186-191). Nothing limits how many lanes build at once.
- The base lock in bench.py (:1594-1620) is the pattern: an O_EXCL lock file with the holder pid and time, a dead holder removed at once,
  a wait line printed periodically.

## Start
`sh 3ds/tools/wt_new.sh BUILDLOCK`. PM_ROW=BUILDLOCK on every call.

## Exact edits
1. New 3ds/tools/build_slot.sh (POSIX sh, Git Bash): `build_slot_take` and `build_slot_release`. Lock directory: the main folder's
   build3ds/buildslots/ (find the main folder as wt_new.sh does: `git rev-parse --git-common-dir`/..). Three slots: slot<k>.lock made by
   `mkdir build3ds/buildslots/slot<k>.d` (atomic on Windows and POSIX), holder file `pid` inside with `$$ $(date +%s) $PM_LANE`.
   take: loop over k=1..3; mkdir succeeds -> write pid, export BUILD_SLOT=k, return. A slot whose pid is dead (`kill -0 <pid>` fails,
   and on Git Bash also `tasklist //FI "PID eq <winpid>"` when kill -0 cannot see it: use the Windows pid from `cat /proc/<pid>/winpid`
   when present) is removed at once (`rm -rf`) with one line `buildslot: slot <k> of dead pid <p> removed`. No free slot: sleep 2 s; every
   60 s print `buildslot: waiting (3 builds running: <lane list>) <n> s`. release: rm -rf the taken slot (only if its pid is ours).
2. build.sh: before the first docker call (:130) source build_slot.sh, check the engine: `timeout 20 docker info >/dev/null 2>&1 ||
   { echo "BUILD FAULT: docker engine not answering in 20 s (docker info). Restart Docker Desktop / WSL, then build again." >&2; exit 3; }`,
   then `build_slot_take`; `trap 'build_slot_release' EXIT INT TERM` (keep the existing traps: chain them). Print `buildslot: slot <k> taken`.
   The build output is unchanged.
3. 3ds/tests/buildlock/run.sh: a fake docker on PATH (a script that sleeps 3 s for `run`, answers `info` at once), five background runs
   of a small wrapper that sources build_slot.sh, takes a slot, records `start <k> $(date +%s%N)` / `end`, runs the fake docker run,
   releases; then checks from the records that at most 3 runs overlap and all 5 end; a sixth case: a slot dir with a dead pid is taken over.
   Prints `buildlock test: ok` or the failing check.

## Gate
- `sh 3ds/tests/buildlock/run.sh`: `buildlock test: ok`.
- One real build: `PM_ROW=BUILDLOCK PM_LANE=BUILDLOCK sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK, `buildslot: slot <k> taken`
  in the output, the slot dir gone after the build.

## Stop and end
A failed gate or a tool fault: commit, hand back the exact output (CLAUDE.md 4). Delete this row file in the completing commit. Hand back
the hash and the test output. Tell the orchestrator: coders see `BUILD FAULT: docker engine not answering` when the engine is down.
