# ws_LINKFAST (Haiku): a fast link and a fast 3dsx (counter T2)
Designer: the oracle (SendMessage). Worktree build3ds/wt/LINKFAST, PM_ROW=LINKFAST PM_LANE=LINKFAST. Start now, parallel with AZTFAST and LOOP1.
## Objective (plan_fastloop.md)
- FACT (oracle probe, lane GMERLEE): link 21.2-23.0 s. The 2143 per-file lines of 3ds/ld/map_restore_bss.ld cost 10.6 s.
  SORT_BY_NAME per map: link 12.4 s, ELF byte-equal. Objects without debug: link 4.4 s, 3dsx byte-equal.
- FACT: -g1 debug bytes are 4 % of -g (84 objects), .text equal. 3dsxtool 4.5 s, 0.6 s with the romfs in the volume.
- Target: link 8 s or less in .ninja_log, ELF under 60 MB (GUESS bound; FACT 19.5 MB without debug).
## Files (all in docs/handoff/ws_LINKFAST.patch). Edit only these 4.
3ds/tools/gen_map_restore.py, 3ds/CMakeLists.txt, 3ds/build.sh, 3ds/tests/host.d/romfscache.sh (new seeded self-test).
## Edit shape (the patch does it)
Before (3ds/ld/map_restore_bss.ld, generated): `    *area_mac/machi/entity.o(.bss .bss.* COMMON)` and 7 more lines.
After: `    SORT_BY_NAME(*area_mac/machi/*.o)(.bss .bss.* COMMON)`. A map keeps its per-file lines when the pattern can name another file.
Before (CMakeLists.txt:487): `ROMFS /out/romfs)`. After: `ROMFS /work/romfs)`. build.sh copies /out/romfs to /work/romfs
(key: shape content key, check: romfs.sha1 on every build, temp folder, one mv).
## Loop (write each output to build3ds/out/, read the last 20 lines)
1. `tr -d '\r' < docs/handoff/ws_LINKFAST.patch | git apply -`. A reject: stop. Send the output to the oracle.
2. `python 3ds/tools/gen_map_restore.py`, then `grep -c SORT_BY_NAME 3ds/ld/map_restore_bss.ld`: 420 (FACT main 341349fe3b).
3. `sh 3ds/tools/nd_host_test.sh > build3ds/out/lf_host.txt 2>&1`: `ND HOST TESTS PASS` and `romfscache ok (first, hit, byte, key, off)`.
4. Commit the 4 files: `LINKFAST 1: one sorted .bss line per map, -g1, romfs in the build volume, one volume inspect, docker build stamp`.
5. `PM_BUILD_CAP=1800 PM_ROW=LINKFAST PM_LANE=LINKFAST sh 3ds/build.sh > build3ds/out/lf_build.txt 2>&1`: rc 0, a `STAGE romfs` line.
   The first build compiles every TU (new flags, ccache miss): GUESS 5-10 min.
6. `MSYS_NO_PATHCONV=1 docker run --rm -v pm3ds-build-LINKFAST:/work:ro pm3ds-build:1 sh -c "grep -E 'pm_3ds[.](elf|3dsx)' /work/build/.ninja_log | tail -4" > build3ds/out/lf_ninja.txt`.
   Link ms = column 2 minus column 1 of the last pm_3ds.elf line: 8000 or less (FACT GMERLEE now: 21000-23000). `ls -l build3ds/lane-LINKFAST/pm_3ds.elf`: under 60 MB.
7. `PM_LANE=LINKFAST python 3ds/tools/bench.py run spot_mac01_drec --row LINKFAST --why "LINKFAST gate"`, then
   `python 3ds/tools/bench.py cmp spot_mac01_drec --row LINKFAST --commit $(git merge-base HEAD main)`: 1/1 byte-equal.
   A cmp line "cold start (warm 0)": run step 7 once more (budget 2 starts).
Budget: 1 host run, 1 build, 2 Azahar starts.
## Fail
- A link error that names a .bss symbol or `__ms_`: send build3ds/out/lf_build.txt to the oracle. Do not edit the generator.
- A cmp difference: stop. Send the cmp output path to the oracle (the probe says byte-equal; a difference is a new fact).
- A shell or python error in a patched line: fix that line only, note it in the commit. Any other fail: 2 tries, then the oracle.
## Stop and hand-back. Last commit `LINKFAST 2: gate` deletes ws_LINKFAST.md and ws_LINKFAST.patch. One SubagentHandback: commits,
the link ms of step 6, the ELF size, the STAGE lines of step 5, `romfscache ok`, the cmp line.
