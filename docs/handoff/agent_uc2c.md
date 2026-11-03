# agent_uc2c: UC2 reccheck on branch uc2c (INCOMPLETE, row budget used: 8 of 8 starts)

## State
- Branch uc2c. Commits: 33978a344 (group key, 0x227 mask), 797d35a48 (docs), then the commit of this file's version (texture mask, texunit key, UC2D/UC2L logs).
- Gate NOT passed. yard_rec: bad 0 only on the binary of START 20 (group key plus 0x227 mask). mac01t_rec_chk: bad 300, miss 300, hit 3600 at f614 (START 26 binary). The START 27 binary (UC2L log) is the current tree.
- No Azahar start is left for row UC2. Next start needs an orchestrator -ResetBudget UC2.
- Build tree must match the commit. Rebuild lane UC2 before any start (gen_map_restore, PM_LANE=UC2 sh 3ds/build.sh, restore 3ds/src/map_restore_table.c from the saved copy).

## Measured (rec line, window f614 unless noted; baseline HEAD 617540255 with reccheck)
| Run | Build | hit | bad | miss | mg | Result |
|---|---|---|---|---|---|---|
| START 17 yard_rec f348 | HEAD | 2375 | 4800 | 0 | - | baseline shot 0000_0_11 |
| START 18 yard_rec f348 | group key | 2103 | 1621 | 0 | 10 | top shot equal |
| START 20 yard_rec f349 | group key + 0x227 mask | 2103 | 0 | 0 | 10 | top shot equal |
| START 22 mac01t_rec_chk f614 | HEAD | 5000 | 12427 | 300 | 0 | HEAD picture (shots equal to mac01t_rec no-check run) |
| START 23 mac01t_rec_chk f614 | + 0x227 mask | 3600 | 6900 | 300 | 0 | first bad word j 103 (masked later, UC2D line was wrong) |
| START 24 mac01t_rec_chk f614 | + texture address mask (0x85, 0x95, 0x9D) | 3600 | 3600 | 300 | 0 | bad 1 per compare |
| START 25 mac01t_rec_chk f614 | same, UC2D uses the rec_same mask | 3600 | 3600 | 300 | 0 | first bad j 106: reg 0x080 TEXUNIT_CONFIG, a 00011001, b 00011003 |
| START 26 mac01t_rec_chk f614 | + ctx+0x110 texunit config and unit pointer bits in the key | 3600 | 300 | 300 | 0 | bad = miss = group length differs (na != nb) |
| START 27 mac01t_rec_chk | log only (UC2L) | - | - | - | - | UC2L: record len 202, frame len 196. First header diff at word 118: record 0x804F00C8 (incremental, reg 0xC8, 4 params, light env), frame 0x803F02C0 (DVLE constants) |

Rule for masks: 0x227 (index buffer address), 0x85, 0x95, 0x9D (texture unit address) are per-frame addresses. They are not compared in UC2. UC3 must patch them at replay.

## Word decode (CONFIRMED)
- libctru GPUCMD_Add (gpu.o 0xd0 to 0xe4): param0 is stored BEFORE the header. Params after the header go to reg + k + 1 when incremental.
- Word j=75 (window of 0x803F0104 group) is ctx+0xF8 = STENCIL_OP reg 0x106. Frame 0x110 = port C3D_StencilOp(KEEP, ZERO, ZERO) at pm_gpu.c ~846. Earlier 0x105 mapping was wrong.
- Texunit config reg 0x080 is written by C3Di_UpdateContext (base.o 0x1f0 to 0x22c): ctx+0x110 = (ctx+0x110 & ~7) | bits of pointers ctx+0x118, 0x11C, 0x120 (non-null) | 0x10000.

## Open item (not fixed): light env group length
- START 27 log: record captured a light env write (reg 0xC8, incremental, 5 words) at word 118 that the frame did not write. The frame writes DVLE constants (0x2C0) there.
- Probable cause (INFERRED): the light env dirty bit (ctx+0x20 bit 0x400, C3Di_LightEnvUpdate at ctx+0x10c) is set by a port light call that happens in the capture but not in the frame check. Or the light shadow (sh5_fx) is not cleared by sh_clear. Check: grep the light setters (C3D_LightEnv*) in pm_gpu.c, and whether sh_clear clears the light shadow.
- Options: (a) clear the light shadow in pm_gpu_rec_begin so the capture and the frame make the same light calls; (b) refuse records with a light env write (pm5 class, plan UC4). Pick by measurement. One start.

## Next steps
1. Orchestrator: -ResetBudget UC2.
2. Fix the light env length (open item). Commit on uc2c if the mac01t_rec_chk shot shows bad 0 and miss 0.
3. Gate: yard_rec and mac01t_rec_chk, bad 0 per window, miss unchanged (300 at f614 is HEAD; a pass needs 0 or a reason), hit above 0, top shots 0000_0_11 and 0000_1_2 and 0001_1_2 byte-equal to HEAD (base shots in C:\Users\Andrew\AppData\Local\Temp\claude\uc2c\base_yard_rec and base_mac01t_rec).
4. UC3 (plan_uniform_cut.md section 4) only after the gate passes.

## Commands
- Build: `python 3ds/tools/gen_map_restore.py`, `PM_LANE=UC2 sh 3ds/build.sh`, restore map_restore_table.c.
- Start: `PM_ROW=UC2 PM_LANE=UC2 powershell -File 3ds/tools/azt.ps1 mac01t_rec_chk -Row UC2 -Build UC2 -Try`.
- Log lines: build3ds/lane-UC2/last_log.txt (grep `rec f`, `UC2D`, `UC2L`).

## Acronyms
UC2 = reccheck row. UC3 = replay row. ctx = citro3d context (`__C3D_Context`). PICA = GPU command set. mg = valid records not compared (group differs at capture start). bad = compared words that differ. miss = patch tables that differ. hit = compares.
