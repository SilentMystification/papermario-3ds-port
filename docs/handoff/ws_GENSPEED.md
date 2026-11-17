# ws_GENSPEED (Haiku): the gfx and pso generators read ext4 copies, not the C:\ bind mount (counter T2, STAGE gen)
Designer: the oracle (SendMessage). Worktree build3ds/wt/GENSPEED, branch GENSPEED, PM_ROW=GENSPEED PM_LANE=GENSPEED. Start now.
## Objective
- FACT (build3ds/out/main_build_nmdl0.txt): STAGE gen 260 s. FACT (oracle timing, main 6f61941c9f, cached gconv objects):
  gen_gfx_tab 71 s on the bind mount (gen_gfx_syms.py 52 s, object cache check 15.6 s), 2.4 s on ext4. gen_pso_tab 28 s, 2.1 s.
  gen_shapes 5 s. Outputs on ext4: byte-equal to the main folder files.
- Target: STAGE gen 40 s or less after a src edit (GUESS 17 s: tar 2.5 s, docker and asset check 3.0 s, run 2.4 s per generator).
- Not in this row: a change to a header that every gconv object reads (3ds/include/nd_*.h) compiles gconv_in.c again:
  96 s (FACT), one TU of 168 sources, 7.67 GB of preprocessed text (FACT). That needs the design GCONVSPLIT (oracle).
## Files (all in docs/handoff/ws_GENSPEED.patch). Edit only these 2.
3ds/build.sh (GEN_AV, gen_run, gen_gfx_tab, gen_pso_tab, the PM_KEY_ONLY line), 3ds/tests/genspeed/run.sh (new self-test).
## Cache keys
No new cache. The keys gfx_tab and pso_tab get v2 (their command text changes). The tar list is the key folder list
(GT_IN, PT_IN): the container sees only key inputs. Every use checks the asset volume against out.sha1 (exit 7 removes it).
## Edit shape (the patch does it)
Before (build.sh gen_gfx_tab): `MSYS_NO_PATHCONV=1 docker run --rm ... -v "$ab/assets:/ar/assets:ro" ... sh -c "$GT_CMD" || {...}`.
After: `gen_run gen_gfx_tab "$GT_CMD" $GT_IN`. GT_CMD starts with `set -e; '"$GEN_AV"'` in place of the 3 ln lines.
## Loop (write each output to build3ds/out/, read the last 20 lines)
1. Reference, before the patch: `PM_ROW=GENSPEED PM_LANE=GENSPEED PM_KEY_ONLY=1 sh 3ds/build.sh > build3ds/out/gs_ref.txt 2>&1`.
   Then `mkdir -p build3ds/out/gs_ref && cp 3ds/src/nd_gfx_tab.c 3ds/src/nd_ntab_descs.c 3ds/src/nd_pso_tab.c build3ds/out/gs_ref/`.
2. `tr -d '\r' < docs/handoff/ws_GENSPEED.patch | git apply -`. A reject: stop, send the output to the oracle.
3. `sh 3ds/tools/nd_host_test.sh > build3ds/out/gs_host.txt 2>&1`: `ND HOST TESTS PASS` (gtin.sh reads the GT_IN lines).
4. Commit the 2 files: `GENSPEED 1: gen_gfx_tab and gen_pso_tab read a tar copy of their key folders and the asset volume`.
5. Build: `PM_ROW=GENSPEED PM_LANE=GENSPEED sh 3ds/build.sh > build3ds/out/gs_build.txt 2>&1`. Pass: rc 0, `gconvobj: hit` with
   `miss 0`, no `gen_gfx_tab: cache hit` line, `STAGE gen` 40 s or less. Then `cmp` each of the 3 files of step 1 with
   3ds/src/<same name>: no output.
6. Cold: `PM_CACHE=off PM_ROW=GENSPEED PM_LANE=GENSPEED PM_KEY_ONLY=1 sh 3ds/build.sh > build3ds/out/gs_cold.txt 2>&1`. Pass: rc 0,
   `gconvobj: hit 0`, the 3 cmp of step 5 again with no output, and `docker volume ls -q | grep pm3ds-assets-cold` prints nothing.
7. Seeded fault: `sh 3ds/tests/genspeed/run.sh build3ds/lane-GENSPEED > build3ds/out/gs_seed.txt 2>&1`: `genspeed ok`.
8. `PM_LANE=GENSPEED python 3ds/tools/bench.py run spot_mac01_drec --row GENSPEED --why "GENSPEED gate"`, then
   `python 3ds/tools/bench.py cmp spot_mac01_drec --row GENSPEED --commit $(git merge-base HEAD main)`. Pass: 1/1 byte-equal.
Budget: 1 build, 2 generator-only runs (steps 1, 6), 1 Azahar start.
## Fail
- A cmp difference in step 5 or 6, or `genspeed FAIL`: stop. Send the output path to the oracle. Do not change the patch.
- `STAGE gen` over 40 s with all other passes: go on. Put the STAGE lines in the hand-back.
- A shell error in a line of the 2 files: fix that line only. 2 tries. Then send the output path to the oracle.
## Stop and hand-back
All steps pass. The last commit `GENSPEED 2: gate` deletes ws_GENSPEED.md and ws_GENSPEED.patch. Hand back (one SubagentHandback):
commits, STAGE gen of steps 5 and 6, the gconvobj lines, `genspeed ok`, the cmp line.
