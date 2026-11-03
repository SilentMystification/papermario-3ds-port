# agent_uc2c: handoff (branch uc2c). Read this first.

## 1. State
- Branch uc2c. HEAD 0ecfddd28. Base main 617540255.
- UC2 (reccheck row, plan_uniform_cut.md section 4) gate: PASSES on the current build (0ecfddd28) for yard_rec and mac01t_rec_chk window f307. Window f614 of mac01t_rec_chk has hit 0 (no valid compares). Gate line "hit above 0" is met in f307 only.
- UC3 (replay uniform patch) NOT started.
- Uncommitted: none of mine. 3ds/src/map_restore_table.c is the user file (never commit). Other agents own 3ds/logs/perf/pushed_builds.txt and 3ds/tools/azt_scenarios.json changes.
- Azahar budget for row UC2: 2 starts used since the last reset (01:40:41 void, 01:42:18 void, 01:48:35 valid, 01:5x yard valid = 3 with START 28, 30, 31). Check 3ds/logs/test_budget.csv before the next start.

## 2. Commits on uc2c (since main)
- 33978a344 UC2: group key (ctx+0xF0..0xFC at capture start) and 0x227 index address mask. Measured: yard_rec bad 4800 -> 1621 (key) -> 0 (mask).
- 797d35a48 docs.
- 9162b2698 UC2: texunit config key (ctx+0x110 with unit pointer bits), texture address mask (regs 0x85, 0x95, 0x9D), UC2D names the masked first bad word, UC2L length log.
- 0ecfddd28 UC2: flags word ctx+0x20 and unit 0/1 slot indices in the key. Measured: mac01t_rec_chk f307 bad 0 miss 0; yard_rec bad 0 miss 0 hit 2003.

## 3. Measured table (rec line; HEAD = 617540255)
| Scene | Window | Build | hit | bad | miss | mg |
|---|---|---|---|---|---|---|
| yard_rec | f348 | HEAD | 2375 | 4800 | 0 | - |
| yard_rec | f348 | group key | 2103 | 1621 | 0 | 10 |
| yard_rec | f349 | +0x227 mask | 2103 | 0 | 0 | 10 |
| yard_rec | f348 | 0ecfddd28 | 2003 | 0 | 0 | 14 |
| mac01t_rec_chk | f614 | HEAD | 5000 | 12427 | 300 | 0 |
| mac01t_rec_chk | f614 | 9162b2698 | 3600 | 300 | 300 | 0 |
| mac01t_rec_chk | f307 | 0ecfddd28 | 2878 | 0 | 0 | 99 |
| mac01t_rec_chk | f614 | 0ecfddd28 | 0 | 0 | 0 | 0 |
Top shots: yard 0000_0_11 and mac01t 0000_1_2, 0001_1_2 byte-equal to HEAD (base copies in C:\Users\Andrew\AppData\Local\Temp\claude\uc2c\base_yard_rec and base_mac01t_rec).

## 4. Decoded causes (CONFIRMED)
- GPUCMD_Add order: param0 before the header, params after it are reg + k + 1 when incremental (libctru gpu.o).
- Word j=75 of the frame group = ctx+0xF8 = STENCIL_OP 0x106 = C3D_StencilOp(KEEP, ZERO, ZERO) = 0x110 (the old 0x105 mapping was wrong).
- Index address reg 0x227 and texture unit address regs 0x85, 0x95, 0x9D: per-frame addresses (staging, frame ring). Masked in reccheck. UC3 must patch them.
- Texunit config reg 0x80 = ctx+0x110, built in C3Di_UpdateContext from unit pointers at ctx+0x118/0x11C/0x120 (base.o 0x1f0 to 0x22c).
- Reg 0xC8 = TEXENV1_SOURCE (5 words, one 6-word write). Emitted when citro3d dirty bit 26+1 is set. C3D_SetTexEnv (texenv.o) does nothing unless ctx+0x20 bit 0 is set. flush_draw sets tex1 = plan.uses_tex1 && pm_tex_bind_aux() (pm_gpu.c ~994). The unit 1 slot index at capture start fixed the window f307 mismatch.

## 5. Open items
- mac01t_rec_chk window f614: hit 0, mg 0, cap 4. Records are not captured or compared there (state 3 churn guard or refusal). Check the rec line fields ref, emp, ak in that window before UC3. Not measured further.
- The key set now: grp[7] (ctx+0xF0..0xFC, ctx+0x110 mixed with pointer bits, ctx+0x20, unit slots). RecKey is 56 B. Consider the struct size under hw_guidelines (L1 data).
- UC2L and UC2D log lines stay (log only).
- Untested: the console. Azahar numbers are direction only.

## 6. Next steps
1. Orchestrator: confirm the UC2 gate result. Run `-ResetBudget UC2` only when the budget is spent.
2. UC3 (plan_uniform_cut.md section 4, row UC3, files 3ds/src/pm_gpu.c): move the uniform block of flush_draw (lines ~892 to 944) into unif_raw(); replay per record draw (plan section 2d steps 1 to 3); run-end clear (step 4); cullm check. Gate: mac01t_rec one record, then all records, then yard_rec: top shots byte-equal to HEAD; bad 0; runs > 0.
3. Patch words to patch at replay: 0x227 p0 and 0x85/0x95/0x9D (see pm_gpu_rec_same mask loop). Keep the mask list in sync with the patch table.

## 7. Commands
- Process check (before any start): write azlist.ps1 with `Get-CimInstance Win32_Process -Filter "Name='azahar.exe'"`, run it with `powershell -NoProfile -File`. Wait up to 5 min, poll each 10 s. Do not kill another run's Azahar.
- Build: `python 3ds/tools/gen_map_restore.py`, `PM_LANE=UC2 sh 3ds/build.sh`, then restore 3ds/src/map_restore_table.c from the saved copy C:\Users\Andrew\AppData\Local\Temp\claude\uc2c\map_restore_table.c.saved.
- Start: `PM_ROW=UC2 PM_LANE=UC2 powershell -File 3ds/tools/azt.ps1 <yard_rec|mac01t_rec_chk> -Row UC2 -Build UC2 -Try`.
- Log: build3ds/lane-UC2/last_log.txt (grep `rec f`, `UC2D`, `UC2L`). Status: 3ds/logs/perf/status_UC2.txt (justification line before each start).

## 8. Acronyms
UC2 = reccheck row. UC3 = replay row. ctx = citro3d context (`__C3D_Context`). PICA = GPU command set. TEV = texture environment. TEXENV1 = stage 1 of TEV. mg = valid records not compared (key differs at capture start). bad = compared words that differ. miss = patch tables that differ. hit = compares. L1 = level 1 cache.
