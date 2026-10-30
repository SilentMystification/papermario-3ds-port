# agent_cfd: epic CFD state (CACHE-FIT, data side)

Branch: main. Rules: CLAUDE.md 2, 5, 7; docs/handoff/hw_guidelines.md; plan_dl_finish.md section 6.

## Commits
- 8a1c19231 CF5 (hot op-loop scalars in one .data.pmhot block).
- afb82557c CF7 (per-frame VRAM upload and stage passes visit set bits).
- CF6: no commit (stopped at step 1, see below).

## State
| Row | State | Quantity (before -> after) | Azahar mac01t_fsplit gbi f900 | Shots |
|---|---|---|---|---|
| CF5 | DONE | hot scalars (nm, pm_3ds.elf): 17 lines, 9 pages -> 5 lines, 2 pages (29 symbols; block 0x70 B at 0x45e640) | HEAD 6.7 / 6.7, new 6.8 | 2 of 2 byte-equal; HEAD reproduces 2 of 2 |
| CF6 | STOPPED (valid gate) | full slot scans per frame on HEAD: 0.32 and 0.24 per frame (97 and 71 per 300 frames, run 1 102 and 73) | n/a | n/a |
| CF7 | DONE | per-frame walk visits 2 x slot_hi (70 at f300, 114 at f600) -> 2 x 16 words | HEAD 6.7 / 6.7, new 6.7 | fsplit 2 of 2, mac01t 2 of 2 byte-equal to HEAD |

## Notes per row
- CF5: the plan asked for one aligned struct. The code keeps the 28 scalars as separate statics with the section attribute `PM_HOTD` (include/pm_port.h). About 110 use sites would have needed a rename. The linker (ld/3dsx.ld.in) puts `.data.pmhot` first in `.data`, after `ALIGN(32)`. pm_gbi.c line 3608 now writes vert_mid only under po3a_on(); po3a_mid is 0 when po3a is off, so the array values do not change. pm_gpu.c `prim` (second static) is not moved.
- CF6: step 1 counted full slot scans in load_impl on HEAD (mac01t, texstat window 300 frames). Scans run at 0.24 to 0.34 per frame. The gate says stop below 1 per frame. The 12-entry recent list catches most lidx misses, so a 4-way index saves almost no scans. The counter was removed again (pm_tex.c back to HEAD before CF7). Plan section 2 estimate of 30 % index collisions is not supported by these counts.
- CF7: step 1 printed slot_hi (not kept in code): 70 at f300, 114 at f600, above the 32 gate. Design: bitmask, not the list in the plan. A list would need a sort to keep slot order (drop and free order changes VRAM placement). The bit walk gives slot order with no sort. Invariants: vup_bits == vram_up for every slot; att_bits set at stale marking and at vram_stage allocation, cleared when the slot is not live or holds no stage.
- Measured `opsplit f299` ops varies between runs at HEAD (901 and 904). mdlsum f899 tot ops was 1215 on HEAD and on CF7.

## Azahar budget (row starts used)
CF5 3 (HEAD x2, new x1). CF6 1 (step 1 count). CF7 4 (step 1 slot_hi, fsplit, mac01t HEAD, mac01t new). All under 8.

## Open items
- CF6 is stopped. Re-open only with a new plan (scan count is the gate).
- Console: CF0 pmu line is the miss counter for the next epic push. Read it at the push.
- map_restore_table.c is user file: modified in the tree, not committed, restored after each build.
- The working tree has other uncommitted files from other work (test_catalog.md, azt_scenarios.json, pushed_builds.txt, docs/handoff/investigate_*). Not mine.

## Commands
- Build: `/c/Python314/python 3ds/tools/gen_map_restore.py`, then `PM_ROW=<row> sh 3ds/build.sh`. Save and restore 3ds/src/map_restore_table.c around the build.
- Test: `PM_ROW=<row> powershell -File 3ds/tools/azt.ps1 mac01t_fsplit -Row <row> -Try [-Build <lane>]`. Baseline lane: build3ds/lane-cfdbase (HEAD binary, CF4 code = HEAD code).
- Symbols: `MSYS_NO_PATHCONV=1 docker run --rm -v "$(pwd -W)/build3ds:/b" --entrypoint sh pm3ds-build:1 -c '/opt/devkitpro/devkitARM/bin/arm-none-eabi-nm -n /b/pm_3ds.elf > /b/nm.txt'`.
