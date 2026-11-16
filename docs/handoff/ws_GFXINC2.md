# ws_GFXINC2 (Haiku): build.sh gen_gfx_tab uses the shared gconv object cache volume (counter T2)
Designer: the oracle (SendMessage). Worktree build3ds/wt/GFXINC2, branch GFXINC2, PM_ROW=GFXINC2 PM_LANE=GFXINC2.
Start after Merge VOLGEN and Merge GFXINC (VOLGEN owns build.sh; GFXINC adds 3ds/tools/gconv_objcache.py).
## Objective
- After VOLGEN, a gfx_tab cache miss in build.sh (any src edit: src is in its key) runs gen_gfx_tab.sh cold: FACT 339 s (BENCHSPEED).
- With the volume, only the changed objects compile. Pass: a src edit outside the gconv set gives `gconvobj: hit 80 miss 0`.
## File. Edit only 3ds/build.sh, one line (the gen_gfx_tab docker run, the line that ends `sh -c "$GT_CMD" || { echo "gen_gfx_tab failed" ...`).
## Cache key
No new key. The volume and its keys are the ones of GFXINC (pm3ds-gconvobj-<16 hex of the host image id>, gconv_objcache.py).
## Edit shape
Before: `... -v "$ab/ver/us/build:/ar/ver/us/build:ro" -w /work pm3ds-host:1 sh -c "$GT_CMD" || ...`
After:  `... -v "$ab/ver/us/build:/ar/ver/us/build:ro" -v "pm3ds-gconvobj-$(docker image inspect -f '{{.Id}}' pm3ds-host:1 | cut -c8-23):/gobj" -e GCONV_OBJ=/gobj -e PM_CACHE="${PM_CACHE:-on}" -w /work pm3ds-host:1 sh -c "$GT_CMD" || ...`
(The same volume name as nd_host_test.sh after GFXINC: `cut -c8-23` of the host image `.Id`. Do not use HIMGID: VOLGEN 1b makes it a layer hash.)
## Loop
1. Make the edit. `sh -n 3ds/build.sh`: no output. `sh 3ds/tools/nd_host_test.sh > build3ds/out/g2_host.txt 2>&1`: `ND HOST TESTS PASS`.
2. Commit: `GFXINC2 1: build.sh gen_gfx_tab mounts the shared gconv object cache volume`.
3. `printf '/* seed */\n' >> src/main_loop.c` (FACT: not in the gconv set, no gconv source includes it), then `PM_ROW=GFXINC2 PM_LANE=GFXINC2 sh 3ds/build.sh > build3ds/out/g2_A.txt 2>&1`,
   then `git checkout src/main_loop.c`. Pass: rc 0, `gconvobj: hit 80 miss 0`, and the `STAGE gen` value (note it).
4. `PM_CACHE=check PM_KEY_ONLY=1 sh 3ds/build.sh > /dev/null 2> build3ds/out/g2_check.txt`. Pass: `cachecheck gconvobj ok 80`, `CACHECHECK gens ok`.
5. Gate: `PM_ROW=GFXINC2 PM_LANE=GFXINC2 sh 3ds/build.sh` (rc 0), `python 3ds/tools/bench.py base spot_mac01_drec`,
   `python 3ds/tools/bench.py run spot_mac01_drec --row GFXINC2 --why "GFXINC2 gate"`,
   `python 3ds/tools/bench.py cmp spot_mac01_drec --row GFXINC2 --commit $(git merge-base HEAD main)`. Pass: 1/1 byte-equal.
Budget: 2 builds, 1 PM_KEY_ONLY run, 1 Azahar start plus the base.
## Fail
A failed pass line: stop. Send the output path to the oracle.
## Stop and hand-back
All steps pass. The last commit `GFXINC2 2: gate` deletes ws_GFXINC2.md. Hand back: commits, the step 3 and 4 lines, the STAGE gen value, the cmp line.
