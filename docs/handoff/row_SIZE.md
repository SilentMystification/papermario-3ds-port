# Row SIZE: 3dsx under the O3DS XL netloader limit

Console facts (2026-11-07): the netloader received 13,167,980 B and refused 13,363,940 B ("not enough free ram" at 24 percent). nds1 builds about 13.37 MB. Target 12.8 MB or less.

## Oracle plan SIZE
Target: 13.37 MB down to 12.8 MB or less, so at least 0.57 MB, plus margin (the console limit lies between 13.17 and 13.36 MB). Ranked by MB per effort and risk:
1. Effect graphics from the resident ROM, about 0.6-0.75 MB, medium effort, low to medium risk.
   - src/effects/gfx/*.o puts 0.62 MB of .data into the 3dsx. The same bytes are kept resident in the ROM image (the effect_gfx ranges, kept because the romdrop counter showed reads).
   - These lists are segment-09 data on the N64 (the effect code does gSPSegment(0x09, shared->graphics), energy_orb_wave.c). In the port the D_09xxxxxx symbols are real .data addresses, and the absolute pointers inside them cost relocations too.
   - Fix: define the D_09xxxxxx symbols as their segmented N64 values (the 0x09 offset, via the map_syms.lds defsym mechanism already in use). At effect load, the port copies the effect's ROM range from the resident ROM into the heap buffer that shared->graphics points to (the N64 dma_copy). The walker's ptr_of already resolves segment 09.
   - The copy byte-swaps by a per-range type map (Gfx words 32-bit, Vtx 16-bit fields, textures unswapped). Generate that map from the effects/gfx C sources (gen script, the same input as splat). Drop effects/gfx from the link.
   - Gate per effect block: nok11_splash, sbk56_superblock, iwa_10, battle_start, intro_native byte-equal.
2. Delete debug-only code and data, about 0.1-0.3 MB, low effort, low risk. WK-4 (group X: dd_* drawlog, suntile, tritrace, combchk, cc07, sp0/mr0/ms0, gp*, stale switches) and the stale per-row diagnostic blocks (orbdbg after FX closes, rectreg/rd_* if present, mdl0, bgrect). Keep the census and prof (cold). Measure with map_sizes.py before and after. Gate: build plus mac_01 byte-equal.
3. -Os on the passing groups only (src/world, area_mac, src/evt, top-level src/*.c, src/entity), about 0.15 MB, low effort. Gate: a full catalog run (the hang risk showed -Os is not free); mac_01 byte-equal.
4. The -Os hang in battle/effects/pause/os: find it, do not ship it.
   - Likely class: data layout, not code UB. At -Os GCC drops the extra alignment of large arrays (DATA_ALIGNMENT is skipped when optimising for size). Arrays then move, so a list that "runs on into the next array" (the C2J and H06c adjacency rules in CMakeLists.txt) reads different words. Op df at the hang fits: a list end read in the wrong place.
   - Method: bisect by object file inside the failing groups (one file at -Os per build, the step-25 hang scenario). For the culprit object, diff the map (symbol addresses and padding) at -O2 and -Os, then read the list that crosses the gap.
   - Fix: add the missing End or merge into one array under TARGET_3DS. That fixes a latent adjacency bug that a later layout change at -O2 could also expose.
   - Gain is small (the rest of the 0.19 MB), so this is a correctness row more than a size row.
5. Relocations (0.77 MB): no action. The 3dsx format needs one relocation entry per absolute pointer run, and pointer-heavy game data (scripts, lists) needs them. Item 1 removes the effect-list relocations as a side effect. A custom loader or a fixed-base trick is out of scope.
Not worth it: battle, map and entity .data (8.6 MB). It is mostly script and pointer structures, which would need N64 overlay loading with relocation to take from the ROM. Entity model rasters and vertex arrays (src/entity/model 0.42 MB) can follow item 1's pattern later if more margin is needed.
Measurement:
- Per build, 3ds/tools/map_sizes.py: section and group totals and the top symbols.
- The 3dsx file size and the reloc table size (the 3dsx header's reloc counts, or 3dsxtool -v).
- The romdrop counter stays 0 for the newly-read effect ranges (they are now intended reads).
Gate:
- 3dsx at most 12.8 MB.
- mac_01 byte-equal; the effect scenes byte-equal to the pre-row build.
- One O3DS XL netloader send: received and boots to the title.

## Rows
SIZE-1 (MEM-A agent, now): item 1, effect graphics from the resident ROM. Then item 3 if the target is not met.
SIZE-2 (with WK-4, later): item 2 debug code deletion.
SIZE-3 (later, correctness): item 4, the -Os adjacency bug.

## Oracle decision SIZE-1 shots (2026-11-07)
Cause: LOG5 a9f558df33 turns logging off in prof mode. pm_log_prof_row(1) sets lv_live = lv_disk = lv_con = -1 (pm_log.c ~520), so the pm_log_d "shot N frame F" line of dump_shot (pm_gpu.c ~2065) is dropped. run_azahar.ps1 uses that line as the until-line and to copy the files from the slot SD, so no shots are collected. dump_shot itself still writes the files (confirm: list the slot sdmc/3ds/PaperMario/shots for 0000-0005 with this run's time).
Design: test-event lines are a harness contract and bypass the logging gate. Add pm_log_evt(fmt, ...) in pm_log.c: written to the disk and live sinks in every mode (logging on, off, prof), still obeying the io gate. Use it for the shot line and every line a scenario uses as an until-line (grep the until patterns in azt_scenarios.json for producers: shot, ndref ... done, encwarp window end, and the like). Picture scenarios keep prof=4.
Files: pm_log.c, pm_gpu.c dump_shot, the until-line producers, one note in docs/handoff/test_catalog.md.
Gate: spot_mac01_pic with prof=4 collects 6 shots and its log shows the shot lines. log5 benches still pass (pm_log_d stays off in prof mode).
Order: SIZE-0 (this fix) first, on its own branch logevt from nds1 (worktree build3ds/wt/logevt); commit; SendMessage main "LOGEVT ready <hash>". Then rebase size1 on logevt and run the SIZE-1 pixel gates (mac_01, nok11_splash, sbk56_superblock, iwa_10, battle_start, intro_native byte-equal vs logevt base), the romdrop counter, and fix 3ds/tools/gen_effect_ld.py so it does not re-add the 109 gfx lines.

## Rules
Own worktree from nds1 (branch size1). Code first (CLAUDE.md rule 5). No attribution lines, never commit 3ds/src/map_restore_table.c, keep Azahar open. Report the 3dsx size after each commit. The orchestrator pushes to the console when the file is at most 12.8 MB. Delete this file when SIZE-1..3 are done.
