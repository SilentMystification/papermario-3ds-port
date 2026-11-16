# ws_VOLGEN (Haiku): shared generator cache, shared asset volume, content-id code sync (counter T2 build time)
Designer: the oracle (SendMessage). Worktree build3ds/wt/VOLGEN, branch VOLGEN, PM_ROW=VOLGEN PM_LANE=VOLGEN. Start now. No shared file with BASEBIN (bench.py) or BENCHSPEED (nd_host_test.sh). Plan: docs/handoff/plan_buildspeed.md.
## Objective and counters
- FACT now: a new lane syncs 24668 files (STAGE sync-host 18-22 s) and runs all generators again (stamps per lane, build.sh:77, :96, :117).
- Pass on a new lane of an unchanged tree: STAGE sync-host 5 s or less, STAGE gen 1 s or less, 4 lines `cache hit`.
## Files (all in docs/handoff/ws_VOLGEN.patch). Edit only these 5.
3ds/build.sh, 3ds/tools/gencache.sh (new), 3ds/tools/src_sync.sh (new), 3ds/tests/host.d/volgen.sh (new), 3ds/tests/srcsync/run.sh (new).
## Cache keys (cache safety: content only, image id in each; review this list)
- texkey: texkey_bake.py, python version, ROM sha1, every asset file. shape: command text, host image id, ROM sha1, the 5 sources,
  3ds/include, 3ds/stub_include, include. gfx_tab: command text, host image id, assets, 3ds/tools, 3ds/include, 3ds/stub_include,
  include, src, nd_scomp.c, nd_texwin.c. pso_tab: as gfx_tab plus 3ds/src and the 2 gfx_tab outputs. Not in a key: pm_build.h (read
  only by pm_main.c, pm_options.c, pm_wdog.c). Asset volume: every asset file (git blob ids). Build skip: old key + asset ids +
  shape entry + both image ids, plus the 3dsx sha1 check. ccache volume and build dir: build image id.
## Edit shape (the patch does it)
Before (build.sh:117-120): `stamp=$OUT/pso_tab.stamp` ... `if [ -f "$stamp" ] && ... ; then echo "gen_pso_tab: up to date"; return 0; fi`.
After: `want="pso_tab v1 ..."` then `if gc_get pso_tab "$want" nd_pso_tab.c:3ds/src/nd_pso_tab.c; then echo "gen_pso_tab: cache hit"; return 0; fi`.
## Loop (write each output to build3ds/out/, read the last 20 lines)
1. `tr -d '\r' < docs/handoff/ws_VOLGEN.patch | git apply -`. A reject: stop, send the output to the oracle.
2. `sh 3ds/tests/srcsync/run.sh`: `srcsync ok`. `sh 3ds/tools/nd_host_test.sh > build3ds/out/vg_host.txt 2>&1`: `ND HOST TESTS PASS` and `volgen gencache ok`.
3. Commit the 5 files: `VOLGEN 1: shared generator cache, shared asset volume, content-id code sync`.
4. Build A: `PM_BUILD_CAP=1800 PM_ROW=VOLGEN PM_LANE=VOLGEN sh 3ds/build.sh > build3ds/out/vg_A.txt 2>&1`. Pass: rc 0, memcheck margin above 0.
   GUESS 5-10 min: the new ccache volume is empty. Note the line `asset volume pm3ds-assets-<16 hex> made`.
5. Build B (new lane, same tree): `PM_ROW=VOLGEN PM_LANE=VOLGENB sh 3ds/build.sh > build3ds/out/vg_B.txt 2>&1`. Pass: rc 0, the 4 lines
   `texkey_bake: cache hit`, `gen_shapes: cache hit`, `gen_gfx_tab: cache hit`, `gen_pso_tab: cache hit`, `STAGE gen` 1 s or less, `STAGE sync-host` 5 s or less.
6. Seeded fault, generator key: `printf '/* seed */\n' >> 3ds/src/pm_main.c`, then
   `PM_LANE=VOLGENB PM_KEY_ONLY=1 sh 3ds/build.sh > /dev/null 2> build3ds/out/vg_seed.txt`, then `git checkout 3ds/src/pm_main.c`.
   Pass: `gen_gfx_tab: cache hit` is in the file, `gen_pso_tab: cache hit` is not.
7. Cachecheck: `PM_CACHE=check PM_KEY_ONLY=1 sh 3ds/build.sh > /dev/null 2> build3ds/out/vg_check.txt`. Pass: `CACHECHECK gens ok`.
8. Seeded fault, asset volume: `MSYS_NO_PATHCONV=1 docker run --rm -v <volume of step 4>:/a pm3ds-build:1 sh -c 'printf x >> /a/c/assets/us/mapfs/geom/mac_01_shape.bin'`.
   Build C: `PM_ROW=VOLGEN PM_LANE=VOLGENC sh 3ds/build.sh > build3ds/out/vg_C.txt 2>&1`. Pass: rc 7 and `BUILD FAULT: the asset volume`.
   Build D: the same line to vg_D.txt. Pass: rc 0 and `asset volume ... made`.
9. Gate: `python 3ds/tools/bench.py base spot_mac01_drec`, `python 3ds/tools/bench.py run spot_mac01_drec --row VOLGEN --why "VOLGEN gate"`,
   `python 3ds/tools/bench.py cmp spot_mac01_drec --row VOLGEN --commit $(git merge-base HEAD main)`. Pass: 1/1 byte-equal.
Budget: 4 builds (the cache proof needs a cold, a hot and a fault build), 2 PM_KEY_ONLY runs, 1 Azahar start plus the base.
## Fail
- A shell error that a step names in one of the 5 files: fix that line only. 2 tries. Never change a key input list: send the output path to the oracle.
- Any other failed pass line: stop. Send the output path to the oracle.
## Stop and hand-back
All steps pass. The last commit `VOLGEN 2: gate` deletes ws_VOLGEN.md and ws_VOLGEN.patch. Hand back (one SubagentHandback):
commits, the STAGE lines of vg_B.txt (asset-hash, gen, sync-host, compile-link), the step 6, 7, 8 pass lines, the cmp line.
