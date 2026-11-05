# Handoff: build pipeline (3ds/build.sh)

## State
Branch dlc-compiler-c1c2. Not pushed. Not merged into main.
Uncommitted, not mine, leave alone: 3ds/src/map_restore_table.c (modified), 3ds/src/map_restore_table.c.bak (untracked).

## Commits (this agent)
- a587886ac build.sh: copy only build inputs. rsync scope 184.8 MB (25339 files) to 117.8 MB (25079 files). Exclude-list version, now replaced.
- 1bf6690f0 run_azahar.ps1, push_3ds.sh: gdb container named and killed on cleanup. Shots moved, not copied. push log read capped at PM_LOG_MAX (default 3600 s) when -t is 0.
- 434d25e9a build.sh, docker/Dockerfile.build: bind-mount inputs (read-only, /src layout), per-lane build volume pm3ds-build-<lane>, shared ccache volume pm3ds-ccache, watchdog kills only container pm3ds-build-<lane> after PM_BUILD_CAP (default 600), pm_build.h rewritten only on change, STAGE timings to stderr.

## Numbers so far (PM_LANE=pipe, bind-mount mode)
- Cold build, empty build and ccache volumes: configure 26 s. Reached 548 of 3333 steps after 956 s. Killed (exit 137) by the coordinator. Full cold time not known.
- Second build, ccache kept, PM_BUILD_CAP=900: configure 23 s, compile-link 119 s, copy-out 3 s, total 148 s. Exit 0. Output build3ds/lane-pipe/pm_3ds.3dsx, 13365576 bytes.
- CPU during compile: 228.6% (docker stats). Host 32 cores. Docker 32 CPUs, 31.3 GiB.
- Watchdog did not fire on the first run (exit 137 came from the kill).

## Open items
1. Low CPU (about 2.3 cores of 32). build.ninja has no compile pool; only edit_cache and rebuild_cache use console. cmake --build has no -j. ninja 1.11.1 default is nproc+2. Cause not found.
2. Add --parallel 32 to the cmake --build call in build.sh. Remeasure cold time and CPU.
3. Time one single-file compile, bind mount vs named volume (copy include-list dirs once into the volume). Keep the faster mode; delete the other.
4. SRC_DIRS list at top of build.sh, one list, no exclude lines. Current build.sh has INPUT_FILES and INPUT_DIRS lists.
5. Serial steps: header generation, gen_map_restore, link. Parallelize or cache if over 5 s.
6. Not measured: one-file rebuild time, cache-hit time, CPU after fix.
7. Write docs/handoff/build_pipeline.md with per-stage seconds, cold, one-file, cache hit, CPU before/after.
8. Commit, then git fetch . dlc-compiler-c1c2:main (no push).

## Commands
- Build (lane): PM_LANE=pipe PM_BUILD_CAP=900 sh 3ds/build.sh
- Cold reset (only for measurement): docker volume rm pm3ds-build-pipe pm3ds-ccache
- Watch CPU: docker stats pm3ds-build-pipe --no-stream
- Build log of ninja steps: ninja -t commands in container /work/build (volume pm3ds-build-pipe)
- Do not kill other containers. actual_server is not ours.
