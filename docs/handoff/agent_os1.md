# agent_os1: row OS1 (pm_gbi.c, pm_gpu.c, pm_tex.c at -Os) - result: PASS (map and picture gates)

## State
- 3ds/CMakeLists.txt: one set_property(SOURCE ... APPEND PROPERTY COMPILE_OPTIONS -Os) for src/pm_gbi.c, src/pm_gpu.c, src/pm_tex.c, after the -fno-short-enums line. Other files unchanged.
- Flag path: add_compile_options(-O2 ...) (CMakeLists line 9) applies to all files. The source option is appended after it. In build.ninja the pm_gbi.o FLAGS end with -O2 ... -Os (the last -O wins).
- DS2 (-Os on pm_gpu.c alone, agent_drawcost.md) was dropped for a +8 % dcsum close sum (24 to 26 us, one run). OS1 applies -Os to three files. The speed effect is not measured on the console.

## Map gate (sum of plan section 1 function bodies, scratchpad os1sum.py on build3ds lanes)
- HEAD (lane os1h, bfb63c063): 58340 B over 45 symbols.
- OS1 (lane os1m): 48720 B. Three symbols appear only in OS1 (dual_bind 1340, ensure_tile 1076, sh_clear 68 = 2484 B). Net drop 9620 B. Gate 8 KB: PASS.
- Warnings: 3363 lines in both builds (ld enum warnings only). No new warning.

## Picture gate (Azahar, direction only)
- mac01t_cap1 (unverified, -Try): HEAD run 1, HEAD run 2 (top and bottom byte-equal). OS1: top 0000_1_2 and bottom 0001_1_2 byte-equal to HEAD. Log checks 6 of 6 in all three runs.
- yard: HEAD and OS1 top shot 0000_0_11 byte-equal. Log checks 3 of 3 in both. Bottom shots not compared (noise per agent_bat1.md).
- A person must open the shots and confirm the subject (not read by the agent).
- Azahar starts used for OS1: 5 of 8.

## Open items
- Console judges speed. Check the close and dcsum stage on the console before the epic is pushed. If the close sum rises as in DS2, revert the three-file line with Edit.
- Map numbers are from the lane maps (build3ds/lane-os1h, build3ds/lane-os1m), not build3ds/pm_3ds.map.

## Commands
- Build: `python 3ds/tools/gen_map_restore.py`; `PM_LANE=os1m PM_ROW=OS1 sh 3ds/build.sh`; restore 3ds/src/map_restore_table.c from a copy (never commit it).
- Test: `PM_ROW=OS1 powershell -File 3ds/tools/azt.ps1 mac01t_cap1 -Row OS1 -Build os1m -Try`; `... yard -Row OS1 -Build os1m`.
- Flags check: `docker run --rm -v pm3ds-build-os1m:/work pm3ds-build:1 sed -n 319p /work/build/build.ninja`.
