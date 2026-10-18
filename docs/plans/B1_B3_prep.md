# B1, B2, B3 prep (read only, 2026-10-18)

Source: AGENT_RULES.md, buffer_bridge.md (sections 1.2, 3, 4, 8, 9), the code at the working tree. Line numbers are from the working tree while F03 is open. Re-grep before you edit. UNV = by code reading only.

## Shared facts

- F03 (B4) edits now: pm_gbi.c (`G_TEXRECT` line 1625, `#include pm_depth.h`), pm_gpu.c, pm_gpu.h, CMakeLists.txt, new pm_depth.c, pm_depth.h, tools/test_zenc.c. Start B1 only after F03 is committed.
- The stray `tmpnofill` block from buffer_bridge.md section 0 is gone from pm_gpu.c (grep: no match).
- Order: F03 commit, then B1, then B2 and B3. B2 and B3 both edit `G_SETCIMG`, `G_SETTIMG` and pm_gpu.c `flush()`. Run them one after the other. Only one build agent at a time.
- The SLICES.md rows for B1, B2 and B3 are one line each. The full table is buffer_bridge.md section 1.2. This file adds no new design.
- Test command base: `powershell -File 3ds/run_azahar.ps1 -Turbo -Smoke -FaultStop -Debug "<switches>"`. Golden: `python 3ds/tools/golden.py toad yard battle` (add `title pause1` for B1). Confirm the warp switch syntax in pm_warp.c before the run. `mac_01` is reached from `toad_east` (pm_warp.c:33). The `flo_10` entry is not in pm_warp.c. The agent must find it in the map table.

## B1: img_role(), imglog, counters

Goal: no picture change. Golden 0 diff.

Edit order:
1. pm_gbi.c, after `cimg_siz` (line 98): add `enum { ROLE_MAIN, ROLE_DEPTHQ, ROLE_CPUFILL, ROLE_DEPTH, ROLE_CPUPAL, ROLE_SLOT }` and `static int img_role(int for_fill)`. Rows R1 to R6 of table 1.2. In B1, R4 (DEPTH) returns the value that R6 gives today for the z-buffer, so the picture does not change. The enum entry exists, the behaviour comes in B3.
2. pm_gbi.c line 102-107 `off_screen_check()`: replace the `!pm_fb_native(cimg) || cimg_siz != 2` test with `img_role() == ROLE_SLOT`. Keep the `pm_fb_native` result exactly equal for today's inputs. This is the risk: `pm_fb_native` is true for the z-buffer at siz 2, so today the z-buffer at siz 2 is MAIN. In B1 map it to MAIN, not to DEPTH.
3. pm_gbi.c line 1168 `rect_to_image`: the guard `!cimg || pm_is_cfb(cimg)` stays as is. Do not touch the tile test.
4. pm_gbi.c line 1584 (`G_FILLRECT` VISCVG): `pm_is_cfb(cimg) && VISCVG` becomes `img_role(1) == ROLE_CPUFILL`.
5. One `pm_zbuf_need()` call site. Today there are two: pm_main.c:195 (`__wrap_appendGfx_darkness_stencil`) and pm_gbi.c:1586 (VISCVG fill). Keep the pm_main.c wrapper (it runs before the display list names the address). Move the pm_gbi.c call into `img_role()` for the case "cimg in the z-buffer range or VISCVG". Note: `pm_zbuf_need()` also calls `pm_tex_invalidate` on every call. Check the cost if it moves into a per-SETCIMG path. Call it at most once per frame.
6. pm_gbi.c `G_SETCIMG` (line 1560): add the `imglog` line after the state update. Switch test with `pm_debug_has("imglog")`, cached in a static. Also `G_SETTIMG` (line 1660). One line: frame, op, role, address kind (cfb cur, cfb prev, zbuf, depthcopy, other), siz, stride, `omode_l`.
7. pm_gbi.c line 1049 and 1065: add `splits`, `slots`, `grabs` to the stats line. Add `static unsigned n_slots, n_splits, n_grabs` and reset them in `pm_gbi_end_frame`. `stats_line[64]` is too small. Make it 96 bytes. `slots` increments when `pm_gpu_off_begin` starts a new slot. Export one getter from pm_gpu.c (`pm_gpu_off_count()`), 3 lines in pm_gpu.c and one in pm_gpu.h. `splits` and `grabs` stay 0 until B2.
8. docs/MEMORY_BUDGET.md: add the VRAM line (buffer_bridge.md section 6).

Overlap: F03 touches pm_gbi.c (`G_TEXRECT`, the include, maybe the opcode switch) and pm_gpu.h. B1 touches `G_SETCIMG`, `G_SETTIMG`, `G_FILLRECT`, `off_screen_check`, the stats code, pm_gpu.h. Conflicts are textual only if F03 is uncommitted. The `pm_depth_query` early break at line 1625 uses `cimg == DepthCopyBuffer`. B1 must keep that test first and may rewrite it as `img_role() == ROLE_DEPTHQ` only after F03 is merged. X02 (B0) is not committed either (pm_gpu.c, pm_gbi.c dirty). Check `git status` first.

Test (one command): golden `python 3ds/tools/golden.py toad yard battle title pause1` must give 0 diff. Then one run with `-Debug "imglog stats"` and `warp=kgr_01:0`, then flo_10, mac_01, one water_splash battle. Check the `imglog` lines for the roles in table 1.2. Use one `tour=` run for the 3 maps.

Risks: step 2 (z-buffer at siz 2 must stay MAIN). Step 5 (`pm_tex_invalidate` cost, call order against the X02 wrapper). `stats_line` buffer size. The 10 KB code rule: B1 is about 1.5 KB EST.

## B2: T2 grab of the current frame buffer

Edit order:
1. Step 1 of the row, as a log and a note, no code: the GX queue order of `C3D_FrameSplit`, `GX_TextureCopy` and the slot clear. Read reference/soh-3ds `CopyFramebuffer` (platform/3ds/source/gfx_citro3d.cpp:3227). Write the result in HISTORY.md (one topic grep, not a read).
2. pm_gbi.c `G_SETTIMG` line 1660-1663: split the test. `timg` in the current frame buffer (`nuGfxCfb_ptr` range) becomes `grab_pending = 1` (T2). `timg` in another frame buffer keeps `pm_cfb_capture_prev()` (T3). Add `pm_is_cur_cfb()` next to `pm_is_cfb` in pm_main.c:175 (`in_buf(p, nuGfxCfb_ptr)`).
3. pm_gbi.c texture load path (where `load_inner` is called through `ensure_tile`): if `grab_pending`, call `pm_gpu_grab(uls, lrs)` before the load, then let `load_inner` find the slot (T1). The slot is keyed (`nuGfxCfb_ptr`, siz 2).
4. pm_gpu.c new `pm_gpu_grab(int x0, int x1)` near `pm_gpu_off_begin` (line 577): `flush()`, `C3D_FrameSplit(0)`, `GX_TextureCopy` of the strips from the main target colour buffer into a scratch texture (256 x 512 RGBA8, VRAM, `C3D_TexInitVRAM`, made once). Then `pm_gpu_off_begin(cfb, 320, 2)`, one rotated quad from the scratch, `pm_gpu_off_end()`. Rows: game x maps to target row `fy = 40 + x`, strip = 7,680 bytes per 8 columns (buffer_bridge.md section 3).
5. Reuse and cap: `grab_dirty` set by every draw to the main target (in `flush()` when `!off_active`). `GRAB_MAX` = 8 per frame, log once over the cap. Increment `n_grabs` and `n_splits` (B1 counters).
6. `offs[OFF_N]` has 4 slots (pm_gpu.c:137). A grab slot is the 5th use in a busy frame (V05 windows use some). Check `off_used >= OFF_N` at line 586. If the cap hits, raise OFF_N to 5 (+512 KB VRAM, UNV) and log it.

Overlap: B1 must be committed (counters, `imglog`, role T2 in `img_role`). F03 reads the PICA depth buffer on the CPU (`pm_depth_query`). `C3D_FrameSplit` ends the command list. Check that F03's read of the last finished depth buffer still reads the right buffer after a split. B3 edits `flush()` and `G_SETCIMG` too. Do B2 first or B3 first, not both at once.

Test (one command): `run_azahar.ps1 -Turbo -Smoke -FaultStop -Debug "imglog stats offdump=<frame>"` with `warp=flo_10:<entry>`. `offdump` of the grab slot shows the scene. Log shows grabs per frame. Then `golden.py toad yard battle` 0 diff. Not tested: timing (HW only).

Risks: ordering of split and copy (UNV in Azahar). `pm_cfb_capture_prev` uses the CPU read of the last frame. If the split starts the queue early, the capture must run before the first split of the frame (buffer_bridge.md section 3 check). PO3 batch rework later (section 9).

## B3: DEPTH role

Edit order:
1. Step 1, log only: in `G_SETCIMG` and at each draw with `cimg` in the z-buffer range (`nuGfxZBuffer`, 150 KB), log cycle type, `omode_l`, `omode_h`, combiner, `prim` for flo_10, mac_01, toad. Use the B1 `imglog` line, add the combiner and prim to it. Confirm the R4 condition and the case 6 depth value. If the case 6 combiner output is not a constant, stop and report.
2. pm_gbi.c `img_role()`: R4 returns `ROLE_DEPTH` when `cimg` is in the z-buffer, `cimg_siz == 2` and (FILL cycle or `omode_l` has Z_CMP or Z_UPD). This changes the B1 mapping (z-buffer siz 2 was MAIN).
3. pm_gbi.c `G_FILLRECT` (line 1575): if `ROLE_DEPTH`, do not draw colour. Call a new `pm_gpu_fill_depth(ulx, uly, lrx, lry, depth)` with the decoded fill word (0xFFFC = far). Place before `off_screen_check()` at line 1596.
4. pm_gpu.c new `pm_gpu_fill_depth`: a copy of `pm_gpu_fill_rect_z` (line 937) with colour write off. The code already has `r_nocol` (line 290: `GPU_WRITE_DEPTH`) and depth test `GPU_ALWAYS` for the always-write case (line 292). Reuse the flags. Do not add a new state.
5. Case 6: triangle draws with `ROLE_DEPTH` draw with `r_nocol = 1` and a constant depth from step 1. Set in the draw setup near `sync_render()`. Check `sync_render` for where `r_nocol` is set today (grep `r_nocol` in pm_gpu.c and pm_gbi.c before the edit).
6. `off_screen_check()` must not start a slot for `ROLE_DEPTH`. It must draw on the main target.

Overlap: this changes `G_SETCIMG` consumers and `flush()` depth state, as B1 and B2 do. F03 reads the depth buffer. B3 writes the depth buffer. After B3 the world depth is cleared by case 5 between cameras (new behaviour). F03 gets a different d24 than before for the HUD camera. Re-run the F03 check (`hos_05` F03 query lines) after B3.

Test (one command): `-Debug "imglog drawlog=<frame>:<n>"` with `warp=mac_01:<entry>` (crystal ball) and flo_10: the decal draw passes in the model area only. Then `golden.py toad yard battle title`. Each diff needs one cause. Expected change: `battle` or `toad` where the HUD was hidden by world depth (buffer_bridge.md section 4).

Risks: the case 6 depth value (stop rule if not constant). Golden diffs from case 5. Prim depth (`G_ZS_PRIM`, `prim_z`, line 1599) still uses `fill_z` and `rect_z`: keep both paths separate. PO3c later moves the value into a uniform.

## Not checked

Hardware timing for B2. Code paths with `gpuvtx` (PO3). Case 9 (no path).
