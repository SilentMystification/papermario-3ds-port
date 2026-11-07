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

## SIZE-4 low-memory hint (user, 2026-11-07)
User: when there is not enough RAM, the error message tells the player to start the Homebrew Launcher in title (non-applet) mode, for example "Hold R while you start any game to open the Homebrew Launcher with full memory, then start Paper Mario again."
Where: 3ds/src/pm_os.c:240-244 (nomem, "Not enough memory for the ROM: need %u MB, free %u MB", shown by rom_fail -> pm_fatal_screen). Also any other boot-time fatal for memory (grep pm_fatal_screen and the linear/heap alloc failures in pm_mem.c, pm_main.c).
How: detect applet mode with libctru envGetAptAppId() != APPID_APPLICATION (or the heap/linear size). In applet mode append the hint (R is the Luma3DS default key; say "Hold R (the default key)"). Outside applet mode keep the plain message. Enlarge the buffer (160 is too small) and check pm_fatal_screen wraps long text on the bottom screen.
Gate: build; Azahar run with a debug switch that forces the low-memory path (e.g. an existing romfull or a new memforce debug switch, debug tools may stay): the bottom screen shot shows the full hint text, no cut. Host or log check that applet detection reads the app id.

## Oracle decision SIZE-1 segment 9 (2026-11-07)
State: size1 20ab5d115 + 3434c270e + 2204b7415 on nds1 5680d32b8; 3dsx 12,763,200 B; mac_01 3/3 byte-equal; romdrop 0. nok11_splash FAIL (no grab line): D_09 symbols and list words are raw 0x09xxxxxx, inside the host heap range (0x08000000 + ~51 MB), so ptr_of (pm_gbi.c 405-414) returns them raw (is_host first).
Design (option b): 1. 3ds/ld/effect_gfx_syms.lds defines D_09xxxxxx = 0xF9xxxxxx (top nibble F, same 24-bit offset). Not a user address, so ptr_of takes the segment path; check it masks (w >> 24) & 0xF. 2. pm_fx_gfx_load (3ds/src/pm_fx_gfx.c) rewrites each segment-9 address operand in the copied lists (G_DL, G_BRANCH_Z, G_VTX, G_SETTIMG, G_MOVEMEM, G_MTX, G_MOVEWORD with a segment) to the host address graphics + offset. Other segments stay. Rerun on the size1 tree: grep -rnE "D_09[0-9A-F]{6}_[0-9A-F]+(\[[^]]*\])?(\.|->)|=\s*D_09[^;]*\[" src (C dereferences of effect data; 0xF9 is unmapped, a missed one faults loud).
Gate: nok11_splash grab line and byte-equal to the reference; sbk56_superblock, iwa_10, battle_start, intro_native byte-equal (cold, nds1 base); mac_01 byte-equal; fxtest blocks (ndref_fx_*) no fault; romdrop: effect gfx reads only from pm_fx_gfx_load. Then SIZE-4.

## Oracle replan SIZE-1 (2026-11-07): PARKED
State: SIZE-4 merged (nds1 55a41fa6b). SIZE-1 failed 4 tries (0x09 raw: host wins; 0xF9/0x79 ABS: loader Error 1; 0x0E ABS: .data tables rebased wrong; section placeholder: crash 0x11111118 at PC 0x00163D20, ent_gdl hashing the placeholder words). WIP: branch size1, patch scratchpad sz/size1_seg9_wip.patch.
Cause: 3dsxtool turns every R_ARM_ABS32 in .data/.rodata into a load-time rebase; any symbol form (ABS or section) gets relocated. Linkable form: a plain integer constant (macro), no symbol.
Design if reopened: generated 3ds/include/pm_fx_ids.h with #define D_09xxxxxx_yyyyyy ((void*)(0x0E000000u | 0xxxxxxx)), force-included for src/effects/*.c; a script wraps each extern D_09 declaration in #ifndef TARGET_3DS; ptr_of maps 0x0E to segbase[9]; load_effect keeps the operand rewrite; C reads of effect bytes (got_item_outline, butterflies, quizmo_stage, sleep_bubble, fright_jar) through a resolver macro. Gate: nok11_splash grab and byte-equal (cold, det), sbk56_superblock, iwa_10, battle_start, intro_native, fxtest no fault, mac_01.
Decision: park. Need 0.19 MB to get under the accepted 13.17 MB, 0.56 MB for 12.8 MB. Order: SIZE-2 (debug-only code and strings, 0.1-0.3 MB), SIZE-3 (-Os on passing groups, about 0.15 MB; mind the -Os step-25 hang from data adjacency), the delete rows (FB-4, GM-3, ST-3, MOVE debug deletes, 0.1-0.2 MB). Re-measure after each (map_sizes.py, 3dsx size). Reopen SIZE-1 only if still above 12.8 MB.

## SIZE-3 state (2026-11-07; step 1 merged in nds1 5616cd985)
Step 1: -Os hang = src/os/sqrtf.c self-call at -Os (fixed, vsqrt.f32); missing returns and 10 uninitialised locals fixed; -O2 gates byte-equal. 3dsx -O2: 13,357,980 B. Not done: model.c renderModeIdx, lodMode, baseVtx (shared draw code, own byte gate).
Step 2 (-Os on world, evt, entity, top-level src/*.c, not model.c and sprite.c): 13,235,400 B (-122 KB) but battle_start_f182_cold shots 0001-0007 differ (33-40 px at x 2/202, y 52-117); -Os on audio alone gives the same pattern; -Os on os alone is equal. Not committed. A build-flag-only change that moves pixels is the same class as the mac_00 layout-dependent row (undefined behaviour, row MAC00b): step 2 waits for MAC00b and the model.c uninitialised fields. Even with step 2 the file is above 13.17 MB; the delete rows (FB-4, GM-3, ST-3) and SIZE-1 (parked) remain the size path.

## Rules
Own worktree from nds1 (branch size1). Code first (CLAUDE.md rule 5). No attribution lines, never commit 3ds/src/map_restore_table.c, keep Azahar open. Report the 3dsx size after each commit. The orchestrator pushes to the console when the file is at most 12.8 MB. Delete this file when SIZE-1..3 are done.
