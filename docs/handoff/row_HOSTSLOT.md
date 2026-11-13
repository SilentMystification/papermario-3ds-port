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
## Gate (expected values)
1. Seeded fault (a value change): in hostslot.sh set the held slots to 2 (one slot free): the check "no run for 5 s" fails:
   `hostslot test: FAIL waits`. Set 3 again: `hostslot test: ok`.
2. `sh 3ds/tests/buildlock/run.sh`: `buildlock test: ok` (unchanged).
3. `PM_ROW=HOSTSLOT PM_LANE=HOSTSLOT sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: build OK, memcheck OK. `python
   3ds/tools/bench.py run nd_host --row HOSTSLOT --why "HOSTSLOT"`: PASS. No Azahar.
## Stop and end
A failed gate, a docker error on `--memory` (hand back the message), or a tool fault: stop, commit, hand back the exact output
(CLAUDE.md 4-5). Delete this row file in the completing commit.
