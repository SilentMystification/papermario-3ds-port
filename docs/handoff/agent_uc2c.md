# agent_uc2c: UC2 reccheck, fragment-op group key (branch uc2c)

## State
- Branch uc2c from main 617540255. Code commit on uc2c (3ds/src/pm_gbi.c, 3ds/src/pm_gpu.c only).
- INCOMPLETE. Azahar start budget for row UC2 is used up (START 19 refused by azt.ps1). Gate not run to the end.
- Mapping of the record window word j=75 is CONFIRMED (not INFERRED). Details below.

## Word j=75 mapping (CONFIRMED)
- libctru `GPUCMD_Add` (gpu.o disassembly, offsets 0xd0 to 0xe4): stores data[0] at offset, then the header, then data[1..] after it. So param0 comes BEFORE the header. Pad 0 goes after an odd count (0x64).
- Group from `C3Di_EffectBind` (effect.o, offsets 0x50 to 0x58): 4 words from ctx+0xF0, header 0x800F0104 becomes 0x803F0104 (count 3).
- Window index: word 72 = ctx+0xF0 = reg 0x104 (ALPHA_TEST). Word 74 = ctx+0xF4 = reg 0x105 (STENCIL_TEST). Word 75 = ctx+0xF8 = reg 0x106 (STENCIL_OP). Word 76 = ctx+0xFC = reg 0x107 (DEPTH_COLOR_MASK).
- `C3D_StencilOp` (effect.o, offsets 0x0 to 0x18): word = fail | zfail<<4 | zpass<<8, stored at ctx+0xF8.
- Port call pm_gpu.c line 846: `C3D_StencilOp(KEEP, ZERO, ZERO)` = 0x110. That is the frame value. Record value 0 = KEEP, KEEP, KEEP.
- The earlier diag (diag_uc2_j75.md, diag_uc2_0x110.md) mapped j=75 to 0x105. That mapping was wrong by one register.

## Change 1 (tested, START 18)
- RecKey has `grp[4]`: ctx+0xF0..0xFC at capture start, read after the `pm_gpu_rec_begin` flush (`pm_gpu_grp_get`, pm_gpu.c).
- rec_try reuses a record only when `grp` matches. Mismatch counts `mg` and captures again. rec line has `mg`.
- Result yard_rec: bad 4800 to 1621, hit 2375 to 2103, miss 0, mg 10. Top shot 0000_0_11 byte-equal to the HEAD shot (START 17).

## Change 2 (NOT tested)
- pm_gpu_rec_same masks param0 of each 0x227 write (INDEXBUFFER_CONFIG, index buffer address, frame ring). This was the first unmasked diff after change 1 (UC2D first j 156, reg 0x227, a 8838a49a, b 8838a482).
- Expected: bad falls toward 0. Not measured. Needs one start.

## Open items
- bad > 0 after change 2: a new investigation names the next register. Do not start a code agent from a guess.
- mac01t_rec gate not run (START 17 style baseline needed for mac01t_rec).
- Gate step: bad 0 per window on yard_rec and mac01t_rec, miss unchanged, top shots byte-equal to HEAD.

## Next steps
1. ResetBudget for row UC2 (orchestrator).
2. Build: `python 3ds/tools/gen_map_restore.py`, `PM_LANE=UC2 sh 3ds/build.sh`, restore `3ds/src/map_restore_table.c` from the saved copy.
3. Write the justification line to 3ds/logs/perf/status_UC2.txt. Then `PM_ROW=UC2 PM_LANE=UC2 powershell -File 3ds/tools/azt.ps1 yard_rec -Row UC2 -Build UC2 -Try`.
4. Read the `rec f34x` line and the `UC2D first` line in the log. Compare top shots with build3ds/azt/shots/yard_rec (saved baseline: C:\Users\Andrew\AppData\Local\Temp\claude\uc2c\base_yard_rec).
5. If bad is 0, run mac01t_rec with the same steps (baseline first).

## Commands
- Log: C:\Users\Andrew\AppData\Roaming\Azahar\sdmc\3ds\PaperMario\log.txt (grep `rec f`, `UC2D`).
- HEAD yard_rec baseline line: `rec f348 cap 48 build 9 ... hit 2375 bad 4800 miss 0` (START 17).

## Acronyms
UC2 = reccheck epic row. ctx = citro3d context (`__C3D_Context`). PICA = GPU command set. GPUCMD = libctru command list API. KEEP, ZERO = stencil op values. mg = count of valid records not compared because the group differs.
