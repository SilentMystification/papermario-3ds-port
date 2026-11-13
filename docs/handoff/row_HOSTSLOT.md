# Row HOSTSLOT: nd_host and the build generators take a build slot, and every heavy container has a memory limit

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names.)
## Tool fault (orchestrator fact) and code read (ORACLE8)
- NPSO3 run 849869: the nd_host docker block ended `gcc: fatal error: Killed signal terminated program cc1` at the gconv compile
  (gen_gfx_tab.sh in nd_host_test.sh), while up to 6 lanes ran builds, nd_host and generators. The docker VM has 14.65 GB. A cc1
  kill under load is the out-of-memory killer (evidence: the message, the load, the earlier run 849043 passed the same step).
- 3ds/tools/build_slot.sh (BUILDLOCK) allows 3 builds at once (BUILD_SLOTS_N=3, slot dirs in the main build3ds/buildslots). It is
  taken only at build.sh:153-159, AFTER the generators (gen_shapes :75 docker, gen_gfx_tab :92 docker, called :79-101). The nd_host
  docker blocks (nd_host_test.sh:15 and :81) take no slot. No docker run has a memory limit.
- Open, not proven: whether host load changes Azahar frame timing (the unstable base pairs). BOOTSTATE gathers that evidence.
  This row does not change the Azahar slots.
Start: `sh 3ds/tools/wt_new.sh HOSTSLOT`. PM_ROW=HOSTSLOT PM_LANE=HOSTSLOT on every call.
## Touch only (places)
3ds/build.sh: the slot block (:153-159) and the line `gen_map_restore` (:47); the docker run lines :75, :92, :224 (one option each).
3ds/tools/nd_host_test.sh: the top (after `cd ...`, :4) and the docker run lines :15 and :81. New 3ds/tests/buildlock/hostslot.sh.
## Exact edits
1. build.sh: move the 7 lines of the slot block (from `. 3ds/tools/build_slot.sh` to `trap 'build_slot_release; exit 130' INT
   TERM`) to right before `gen_map_restore` (:47), inside `if [ -z "$PM_KEY_ONLY" ]; then ... fi` (a key-only call takes no slot:
   its generators are stamp-skipped). Nothing else moves.
2. build.sh docker runs :75, :92, :224: add `--memory 4g` after `--rm`.
3. nd_host_test.sh after `cd "$(dirname "$0")/../.." || exit 2`: `. 3ds/tools/build_slot.sh`, `build_slot_take`, `trap
   'build_slot_release' EXIT`, `trap 'build_slot_release; exit 130' INT TERM`. Its docker runs :15 and :81: add `--memory 4g`.
4. hostslot.sh (as 3ds/tests/buildlock/run.sh, fake docker on PATH, BUILDSLOT_DIR in build3ds/out): hold 3 slots with dummy
   holders (live pids), start nd_host_test.sh with the fake docker: it must not run docker for 5 s (it waits: the fake docker
   records no `run`), release one slot: it runs. Print `hostslot test: ok` or `hostslot test: FAIL <check>`.
5. Commit: `HOSTSLOT: nd_host and the build generators take a build slot, docker runs limited to 4 GB`.
## Correction 1 (ORACLE8, after 5f84185e5): the build runs 32 compilers at once
- build.sh:242 runs `cmake --build /work/build --parallel "$(nproc)"` inside the container: 32 cc1 on the 32 CPUs of the VM.
  Under `--memory 4g` that build was OOM-killed (docker inspect OOMKilled=true). The memory of one build is the sum of its
  parallel cc1. VM 14.65 GB: 3 slots x 4 GB = 12 GB leaves 2.6 GB for docker itself. So the slot count (3) and the limit (4 GB)
  stay, and the jobs per build drop: 8 parallel compilers (about 250 MB each, one large unit such as nd_gfx_tab.c or pm_gpu.c
  up to about 1 GB) fit in 4 GB. A clean full build takes longer (ccache keeps the normal rebuilds short).
- Extra edits (in the places of this row, build.sh):
  a. :242 `--parallel "$(nproc)"` -> `--parallel "${PM_BUILD_JOBS:-8}"`.
  b. Every `--memory 4g` of this row (build.sh :75, :92, :224, nd_host_test.sh :15, :81) -> `--memory "${PM_BUILD_MEM:-4g}"`.
  c. The line `if [ "$rc" -ne 0 ]; then echo "BUILD FAILED exit $rc (container $NAME; stage timings above)" >&2; exit "$rc"; fi`:
     add to its text ` (memory limit ${PM_BUILD_MEM:-4g}, ${PM_BUILD_JOBS:-8} jobs: a "Killed signal" above is an out-of-memory kill)`.
## Gate (expected values, corrected)
1. Done: hostslot test seeded FAIL waits, then ok. buildlock test ok.
2. Seeded fault (a value change, no edit): `PM_BUILD_MEM=1g PM_ROW=HOSTSLOT PM_LANE=HOSTSLOT sh 3ds/build.sh > build3ds/out/build_1g.txt
   2>&1` after `docker volume rm pm3ds-build-HOSTSLOT` (a clean build): it fails, the last line is the BUILD FAILED text with
   "memory limit 1g".
3. `docker volume rm pm3ds-build-HOSTSLOT`, then `PM_ROW=HOSTSLOT PM_LANE=HOSTSLOT sh 3ds/build.sh > build3ds/out/build.txt 2>&1`:
   full build OK (all objects) under 4g and 8 jobs, memcheck OK. Hand back its build time.
4. `python 3ds/tools/bench.py run nd_host --row HOSTSLOT --why "HOSTSLOT"`: PASS (the nd_host limits). No Azahar.
## Stop and end
A failed gate, a docker error on `--memory` (hand back the message), or a tool fault: stop, commit, hand back the exact output
(CLAUDE.md 4-5). Delete this row file in the completing commit.
