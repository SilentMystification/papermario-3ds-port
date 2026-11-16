# ws_GFXINC (Haiku): per-object cache of the gfx table inputs, shared by nd_host_test.sh and build.sh (counters T1, T2)
Designer: the oracle (SendMessage). Worktree build3ds/wt/GFXINC, branch GFXINC, PM_ROW=GFXINC PM_LANE=GFXINC. Start now
(BENCHSPEED is merged). Owns gen_gfx_tab.sh and nd_host_test.sh until merged; VOLHOST waits for it. build.sh: ws_GFXINC2.
## Objective and counters
- FACT (BENCHSPEED): gen_gfx_tab 339 s cold on every row. gen_gfx_tab.sh:17-20 compiles 1 shared file and 79 own-TU files (FACT gconv_src.d: 246 entries, 167 shared, 79 own) one at a time, on every run.
- FACT nd_host_test.sh:31-33: the gfxcache key misses include, src (outside effects and entity), 3ds/stub_include and most of
  3ds/tools, though gen_gfx_tab.sh:14 compiles with -I for all of them. Cache safety rule 1 fails. The patch fixes the key.
- Pass: the 8 outputs byte-equal to the old script; a second run has miss 0 of 80 objects (FACT oracle count); seeded faults miss.
## Files (all in docs/handoff/ws_GFXINC.patch). Edit only these 5.
3ds/tools/gen_gfx_tab.sh, 3ds/tools/nd_host_test.sh, 3ds/tools/gconv_objcache.py (new), 3ds/tests/host.d/gfxinc.sh (new), 3ds/tests/gfxinc/run_gen.sh (new).
## Cache key of one object (cache safety; review this list)
Primary: tool version, `gcc --version` and `as --version` lines, the flags, the TU path (/tmp/gct, the same in every caller), the
TU sha1. Checked on every use: sha1 of each file the compile read (gcc -MD: #include files; as --MD: .incbin files of
gconv_inc/include_asset.h, FACT oracle test in pm3ds-host) and obj.o against out.sha1. Store: docker volume
pm3ds-gconvobj-<16 hex of the host image id>. PM_CACHE=off: no cache. PM_CACHE=check: compile all, compare, CACHECHECK lines.
## Edit shape (the patch does it)
Before (gen_gfx_tab.sh:19): `for f in $TUS; do gcc -m32 -w ... -c -o "${f%.c}.o" "$f"; TUOBJ="$TUOBJ ${f%.c}.o"; done`.
After: `python3 3ds/tools/gconv_objcache.py --flags "-m32 -w ... $DEF -DND_GCONV_IN $INC" "$TD/gconv_in.c" $TUS`, then the same ld -r.
## Loop (write each output to build3ds/out/, read the last 20 lines)
1. `git show HEAD:3ds/tools/gen_gfx_tab.sh > build3ds/out/gen_old.sh`. Then `tr -d '\r' < docs/handoff/ws_GFXINC.patch | git apply -`.
2. `sh 3ds/tools/nd_host_test.sh > build3ds/out/gi_host.txt 2>&1`: `ND HOST TESTS PASS` and `gfxinc ok`. Commit the 5 files:
   `GFXINC 1: per-object cache of the gfx table inputs (gconv_objcache.py), shared volume; gfxcache key covers every -I folder`.
3. Reference: `PM_CACHE=off sh 3ds/tests/gfxinc/run_gen.sh build3ds/out/gen_old.sh build3ds/out/gi_ref none`.
4. Cold fill: `sh 3ds/tests/gfxinc/run_gen.sh 3ds/tools/gen_gfx_tab.sh build3ds/out/gi_b pm3ds-gconvobj-gfxinctest`.
5. Hit: the same line with build3ds/out/gi_c. Pass: gi_c/log.txt has `gconvobj: hit 80 miss 0`. Note the run_gen seconds of 3, 4, 5.
6. Equal: `for f in build3ds/out/gi_ref/*.[ct]*; do cmp "$f" build3ds/out/gi_b/${f##*/} && cmp "$f" build3ds/out/gi_c/${f##*/}; done`.
   Pass: no output (8 files, log.txt is not compared).
7. Seeded fault: `printf '/* seed */\n' >> src/battle/area/sam2/vtx.c` (an own-TU source), step 5 to build3ds/out/gi_d, then
   `git checkout src/battle/area/sam2/vtx.c`. Pass: gi_d/log.txt has `hit 79 miss 1`.
8. Cachecheck: `PM_CACHE=check sh 3ds/tests/gfxinc/run_gen.sh 3ds/tools/gen_gfx_tab.sh build3ds/out/gi_e pm3ds-gconvobj-gfxinctest`.
   Pass: `cachecheck gconvobj ok 80`. Then `docker volume rm pm3ds-gconvobj-gfxinctest`.
9. Host test twice more: `sh 3ds/tools/nd_host_test.sh` to gi_host2.txt, then `printf '/* seed */\n' >> include/common_structs.h`,
   gi_host3.txt, `git checkout include/common_structs.h`. Pass: both `ND HOST TESTS PASS`; gi_host3 has `gfxcache: miss`. No game file changes: no build, no Azahar run. Budget: 7 docker runs of gen_gfx_tab, 3 host tests.
## Fail
- `Killed` in a log (memory): run that step again with `GCONV_JOBS=2` in front. Never change the patch for it.
- A cmp line in step 6, or a pass line missing: stop. Send the output path to the oracle. A shell or python error that a step
  names in one of the 5 files: fix that line only, 2 tries, then send the output path.
## Stop and hand-back
All steps pass. The last commit `GFXINC 2: gate` deletes ws_GFXINC.md and ws_GFXINC.patch. Hand back (one SubagentHandback):
commits, the run_gen seconds of steps 3, 4, 5, the pass lines of steps 5-9, the STAGE gfx-table line of gi_host2.txt.
