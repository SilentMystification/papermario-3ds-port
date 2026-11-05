# Diagnosis: bottom/top HUD memory numbers

Scope: diagnosis only. No source edited. Files read: 3ds/src/pm_gpu.c, 3ds/src/pm_main.c, 3ds/src/pm_mem.c, 3ds/src/pm_tex.c (grep and line ranges).

## Where the HUD memory text comes from

| Step | Source line | What it does |
|---|---|---|
| Init values | pm_main.c:329-330 | `reg_total` = `osGetMemRegionSize(MEMREGION_APPLICATION) >> 20`, `lim_mb` = `pm_mem_commit_limit_mb()`, `reg_free` = `osGetMemRegionFree(MEMREGION_APPLICATION) >> 20`. Integer MB. |
| Refresh | pm_main.c:508 | `reg_free` is read again once per second, inside the FPS block. Between reads the value is stale. |
| Format | pm_main.c:556 | `"%u/%uM C%uM"` = `reg_free / reg_total` and `C lim_mb`. |
| Limit value | pm_mem.c:157, pm_mem.c:81 | `lim_mb` = `commit_max >> 20`, the commit limit read once at boot (pm_mem.c:78). |
| Draw | pm_gpu.c:1240-1260 | `pm_gpu_hud` writes the two rows through `hud_put_line` (pm_gpu.c:1232). It returns early when `pm_con_trylock` fails, so a busy log thread skips the update. |
| Caller gate | pm_main.c:564-566 | Runs only when `pm_opt.dbg_hud` is set. |

Other memory numbers are log-only (`pm_mem_log`, pm_mem.c:159-190). They are not on the HUD. `pm_mem_crash_line` (pm_mem.c:143-154) is for the crash handler only.

## Per number

### N1. Top line, `reg_free/reg_total` (pm_main.c:330, 508, 556)
- Counts: free bytes in the APPLICATION memory region (FCRAM), in MB, refreshed 1 Hz.
- Misses:
  - VRAM. The PICA VRAM is not part of the APPLICATION region, so VRAM use is not shown. (gap G1)
  - Split between linear heap, newlib heap and the SOC buffer. One number only. (G2)
  - Sub-MB change. Truncation to MB. (G2)
  - Stale value up to 1 s old. (G2)
- Fix proposal:
  - G1, G2: add a second HUD field from data already read in pm_mem_log: `vramSpaceFree()` in KB (pm_mem.c:184) and `linearSpaceFree()`.
  - Read both in the same 1 Hz block as pm_main.c:508.
  - Keep `reg_free` for the FCRAM total.

### N2. Top line, `C<lim_mb>` (pm_main.c:329, 556; pm_mem.c:157)
- Counts: the commit limit given at boot (`commit_max`, pm_mem.c:81). It is a constant for the whole run.
- Misses: current commit use. Commit use is read in `pm_mem_crash_line` (pm_mem.c:147-148) but not on the HUD. (G3)
- Fix proposal:
  - G3: show `commit_cur` next to `commit_max` (2 lines in pm_mem.c, one format change in pm_main.c:556).
  - Use the same `svcGetResourceLimitCurrentValues` call as pm_mem.c:147-148.
  - Keep the boot limit as the second number.

### N3. Log line `linear used ... fixed part` (pm_mem.c:175, 182)
- Counts: `lin_used = pm_linear_bytes() - linearSpaceFree()`. This is the linear heap in use. `fixed part = lin_used - pm_tex_used()`.
- Misses / wrong:
  - `pm_tex_used()` (pm_tex.c:669) counts the texture cache. The cache slab is VRAM (`vramMemAlign`, pm_tex.c:427), not linear. The subtraction at pm_mem.c:182 takes a VRAM number from a linear number. The "fixed part" value is wrong by `pm_tex_used()`. (G4)
  - The H03 comment (pm_mem.c:14-16) says linear minus texture cache is constant. That was true only when the cache was in linear. It is not true now. (G4)
  - `PM_LINEAR_FIXED_KB` (pm_mem.c:20) and the `boot` split line (pm_mem.c:177-180) use the old numbers. Not verified against a current run.
- Fix proposal:
  - G4: remove the `- pm_tex_used()` term from pm_mem.c:182, or subtract only linear texture users (pm_tex.c:1194 `vram_stage` is linear).
  - Confirm with one Azahar `mem` line. The "fixed part" must not change when the texture cache changes size.
  - Update the H03 comment after the check.

### N4. Log line `heap total/used/free-in-arena/sbrk-left` (pm_mem.c:185-189)
- Counts: newlib heap from `mallinfo()`: `uordblks` (used), `fordblks` (free in arena), and `sbrk-left = fake_heap_end - __ctru_heap - mi.arena`.
- Misses: not on the HUD. The heap is inside the APPLICATION region, so N1 counts it as used, but N1 cannot split it. (G2)
- Caveat: `mallinfo()` takes the malloc lock. pm_mem.c:142 says the crash path avoids it for that reason. Keep it out of any per-frame HUD path. (G5)
- Fix proposal:
  - G2: show `mi.uordblks` in the HUD only at the 1 Hz refresh.
  - G5: keep the call out of the crash path and out of the per-frame path.

### N5. Log line `linear total/free` (pm_mem.c:186-189) and `pm_linear_bytes()` (pm_mem.c:132)
- Counts: `__ctru_linear_heap_size`, set to `ours = PM_OURS - soc` (pm_mem.c:95, 102). `PM_OURS` is 14 MB (pm_mem.c:19).
- Misses / check:
  - The linear block allocated from the commit is `avail` (pm_mem.c:98). Only `ours` is the linear heap. The rest is newlib heap (pm_mem.c:103-104).
  - The log number for linear total is `ours`, not the whole linear allocation. The reader must not add it to the heap total.
  - Not verified: whether libctru uses the same `__ctru_linear_heap_size`. If it does, `linearSpaceFree()` matches `ours`.
- Fix proposal:
  - Print `ours` and `avail - ours` in one line, labelled "linear heap" and "newlib heap".
  - No code change in the allocator.

### N6. SOC buffer, 128 KB at 0x08000000 (pm_mem.c:92-111)
- Counts: `soc_size` (PM_SOC = 0x20000 = 128 KB) is subtracted from `avail` before the linear allocation (pm_mem.c:94). It is then allocated separately at 0x08000000 (pm_mem.c:107).
- Misses: no HUD or log line reports it. It is in the APPLICATION region used count (N1), but not in linear or heap. (G6)
- Fix proposal:
  - G6: add `pm_soc_buffer(&size)` (pm_mem.c:126-129) to the boot log line (pm_mem.c:171).
  - One line, no change to the allocation.

### N7. Stack (pm_mem.c:165-167)
- Counts: `pm_stack_used(0)` (game) and `pm_stack_used(1)` (audio) high-water, bytes, log only.
- Misses: not on the HUD. Stack is not in the APPLICATION free value unless it is carved from the region. Not checked in this diagnosis. (G7)
- Fix proposal:
  - G7: add the two stack high-water values to the 1 Hz HUD refresh only if the layout matters for playtest. Otherwise leave it in the log.

### N8. DSP / audio memory
- Allocations found by grep (all linear):
  - pm_dspaud.c:107 `linearMemAlign(need, 128)`
  - pm_dspaud.c:393 `linearMemAlign(FXR_BUFS * FXFRAME * 4, 128)`
  - pm_dsptest.c:137-138 `linearAlloc` (test code, check if built)
- These are in linear, so N3 (`lin_used`) counts them. They are not separated.
- Misses: no HUD or log line names the audio buffers. The audio stack high-water (N7) is the only audio number. (G8)
- Fix proposal:
  - G8: add a counter in pm_dspaud.c next to each `linearMemAlign`, and add its total to the boot log line.
  - Check first whether pm_dsptest.c is in the build.

### N9. Other linear users not on the HUD
- Found by grep, all linear, all counted in N3 but not named:
  - pm_gpu.c:1113-1114 `gring`, `gidx` (GRING_VTX + GSTAT_VTX) x 16, GRING_IDX x 2.
  - pm_gpu.c:1157 `vert_base` (VERT_SLOTS x sizeof Vert).
  - pm_gpu.c:1073 `C3D_Init(0x180000)` command buffer 1.5 MB (linear, per the comment at pm_gpu.c:1073).
  - pm_tex.c:664 `scratch` (TEX_SCRATCH_PLANE x 2).
  - pm_tex.c:1194 `vram_stage` (gpu_size).
- Misses: no per-owner line. Only the sum (N3). (G9)
- Fix proposal:
  - G9: no new allocation counter. Print the sizes of the fixed blocks once at boot with the existing `boot` line (pm_mem.c:177-180).

### N10. Texture cache in VRAM (pm_tex.c:426-435, 669)
- Counts: `pm_tex_used()` = `used` (pm_tex.c:669). Slab is 4096 KB in VRAM (`vramMemAlign`, pm_tex.c:427). The log line at pm_tex.c:432 prints VRAM free before and after.
- Misses: not on the HUD. The number is used in the log only (pm_mem.c:182, 186). (G1, G4)
- Fix proposal:
  - G1: show `pm_tex_used()` in KB on the HUD next to VRAM free (N1 fix).

### N11. VRAM free (pm_mem.c:184)
- Counts: `vramSpaceFree()`, in KB, log only. The constant `6 MB` is the PICA VRAM size, a fixed value.
- Misses: not on the HUD. (G1)
- Fix proposal: see G1.

## Gap summary

| ID | Gap | Affected numbers | Fix (max 5 lines) |
|---|---|---|---|
| G1 | VRAM (texture slab, VRAM free) not on HUD | N1, N10, N11 | Add `vramSpaceFree()` KB and `pm_tex_used()` KB to the 1 Hz HUD refresh (pm_main.c:508). |
| G2 | HUD shows only FCRAM region free, MB, 1 Hz; no linear/heap split | N1, N4, N5 | In the same 1 Hz block, read `linearSpaceFree()` and `mallinfo().uordblks`, and print them as a second HUD line. |
| G3 | "C" value is the boot commit limit, not current use | N2 | Print `commit_cur` with `commit_max` using the same call as pm_mem.c:147-148. |
| G4 | "fixed part" subtracts a VRAM number from linear used | N3, N10 | Remove the `- pm_tex_used()` term at pm_mem.c:182 or subtract only linear users. Check the H03 comment. |
| G5 | `mallinfo()` takes the malloc lock | N4 | Keep `mallinfo()` out of per-frame and crash paths. Read it only at the 1 Hz refresh. |
| G6 | SOC buffer (128 KB) reported nowhere | N6 | Add `pm_soc_buffer` size to the boot log line (pm_mem.c:171). |
| G7 | Stack high-water not on HUD | N7 | Log only. Add to HUD only if needed for playtest. |
| G8 | Audio linear buffers not named | N8 | Count each `linearMemAlign` in pm_dspaud.c and print the total at boot. Check whether pm_dsptest.c is built. |
| G9 | Other linear owners not named | N9 | Print the fixed block sizes once in the boot line (pm_mem.c:177-180). |

## Not verified in this diagnosis

- Whether the APPLICATION region free value (N1) includes linear and newlib heap. This is expected, not measured.
- Whether `linearSpaceFree()` uses `__ctru_linear_heap_size` (libctru source not read).
- Whether pm_dsptest.c and pm_dspaud.c are in the current build.
- The current size of `PM_LINEAR_FIXED_KB` (pm_mem.c:20). The value is from an old measurement.
- No Azahar or hardware run. No value in this file is a measured number.
