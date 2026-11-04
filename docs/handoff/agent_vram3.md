# agent_vram3 handoff (branch native, 2026-11-04)

## State
- R5 committed: f87ece943 on native (OFF_N 4 to 3 full images, OFF_S 4 small slots kept). File: 3ds/src/pm_gpu.c only.
- R2 written, NOT verified, on branch vram3-r2: commit e58e6a8ae. The same change is in the worktree, uncommitted, because the native test build includes it. Do not merge vram3-r2 until the stereo gate passes.
- pm_tex.c was not edited by this agent (converter agent owns it).
- Not committed by this agent: 3ds/src/map_restore_table.c (user file), 3ds/logs/perf/pushed_builds.txt, 3ds/tools/azt_scenarios.json (modified by another agent).

## R5 (OFF_N 4 to 3): what changed
- pm_gpu.c enum: OFF_N = 3 (full images), OFF_S = 4 (64 x 64 slots), OFF_P = OFF_N + OFF_S. The old code used OFF_N for both; the 64 x 64 cap now uses OFF_S, so small slots stay at 4.
- full_reserve loops over OFF_N: one 512 KiB block less at boot. slab_init (pm_tex.c) takes the free bank space, so the cache grows with no pm_tex.c edit.
- Hard log line: `V05 demand: full image N in frame F, OFF_N 3: slot K is used again` (pm_log_e, first 20 cases). The once-only warning is kept for the 64 x 64 cap.
- Demand from code: the star transition (OVERLAY_START_BATTLE, src/screen_overlays.c:329) draws the previous frame (draw_prev_frame_buffer_at_screen_pos) and the star stencil D_8014E8F0 into the main target. It uses no full image. Pause peak measured 2. File select flip (4 panels) peak is not measured here. Open: 3 full images may be too few in the flip.
- Freed size is 512 KiB, not 256 KiB (one full image is 512 x 256 x 4 bytes).

## R2 (shared depth buffer): what changed
- pm_gpu_init: targetR frees its own D24S8 buffer (vramFree) and uses target's depth buffer. 375 KiB back to the cache.
- pm_gpu_eye: the right eye clears depth before its pass (C3D_CLEAR_DEPTH); colour only when r_wantclear.

## Measured (Azahar)
- Boot cap line (yard, start 4 and 6, lane native): `texcache bank A: 160KB`, `bank B: 2592KB`, cap 2752 KB. Before: 1856 KB. Change +896 KB.
- yard top shot 0000_0_11.ppm (start 6): byte equal to texf1_yard (HEAD shot). Start 4 wrote shots that the archive step did not find; start 6 is the valid run.
- pause_spin (start 7, lane native): 10 of 10 shots byte equal to texf1_pause. No V05 demand line in the log.
- yard_stereo_l baseline on lane r5base (start 8, HEAD before R5 and R2): shots written, log checks 2 of 2. Reference: build3ds/azt/shots/yard_stereo_l/.

## Open items
- R2 gate NOT run. Needed: yard_stereo_r on r5base; yard_stereo_l and yard_stereo_r on native. The VRAM3 budget is at 8 of 8 starts (test_budget.csv). Orchestrator reset: powershell -File 3ds/run_azahar.ps1 -ResetBudget VRAM3.
- R5 file select flip not run. Check with fileselect_rotate and the V05 demand line. If the line prints in the flip, OFF_N 3 changes the flip; revert f87ece943 or raise OFF_N back to 4.
- R2 risk: the left eye starts a frame with the right eye's last depth when the frame does not clear. Check the stereo shots.
- R2 risk: a mono frame after a stereo frame starts with the right eye's depth when the frame does not clear.

## R4 plan (RGBA5551 off-screen full images, 256 KiB saved per full image, 768 KiB for 3)
Facts from the devkitPro image (libcitro3d.a, disassembled in pm3ds-build:1):
- C3D_RenderTargetCreateFromTex(tex, face, level, depthFmt) has no colour format argument.
- Its colour format is set in C3D_FrameBufTex (framebuffer.o): colorFmt = tex->fmt (low 4 bits of the bitfield word at tex+4). Enum values match: GPU_RGBA5551 = 2 and GPU_RB_RGBA5551 = 2.
- So a GPU_RGBA5551 C3D_Tex gives a GPU_RB_RGBA5551 render target. No second call.
- depthFmt -1 (as now) means no depth buffer in CreateFromTex.
Steps:
1. tex_hdr (pm_gpu.c) takes the format: C3D_TexInit(t, w, h, GPU_RGBA5551). Data is 2 bytes per pixel: 256 KiB.
2. full_reserve: block size half of BLK_FULL for full images. copy_blk stays 512 KiB (grab is RGBA8 and reads copy_blk as 256 x 512 RGBA8).
3. off_snapshot: bytes = w * h * 2 for a 5551 slot. GX_TextureCopy copies raw bytes, so the size must match the format. Check the 16-bit tile layout (same 8 x 8 tile order assumed).
4. pm_tex.c: the bind path for an off-screen slot takes the tex from offs[]. It must sample GPU_RGBA5551 and must not assume 4 bytes per texel. pm_tex.c belongs to the converter agent: report, do not edit.
5. Format per slot: a slot keeps its format (its render target is created once). Read which siz each full slot draws in the offuse trace first. If a slot draws both siz 2 and siz 3, a 5551 slot cannot serve siz 3. Plan: 5551 only for slots used as siz 2.
6. Expected: the N64 source is 16-bit 5551, so the stored value may match the source. Alpha is 1 bit, as in the source. Not measured. Gate: pause_spin 10 shots against texf1_pause (read the picture; blend results may change) and yard top shot.
7. Cache grows by 768 KiB with R4 for three full images (copy block kept).

## Commands
- Build: PM_LANE=native sh 3ds/build.sh (Docker image pm3ds-build:1). Run /c/Python314/python 3ds/tools/gen_map_restore.py first; save and restore 3ds/src/map_restore_table.c around it.
- Baseline lane: PM_LANE=r5base sh 3ds/build.sh (HEAD before R5 and R2): build3ds/lane-r5base/pm_3ds.3dsx.
- Test: PM_ROW=VRAM3 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 <scenario> -Row VRAM3 -Build native -Try
- azt.ps1 archives shots to build3ds/azt/shots/<scenario>/.
- Justification lines: 3ds/logs/perf/status_VRAM3.txt (8 starts).
- Pre-change shots: C:\Users\Andrew\AppData\Local\Temp\claude\shots\texf1_yard, texf1_pause.

## Next steps
1. Orchestrator resets the VRAM3 budget. Run yard_stereo_r on r5base, then yard_stereo_l and yard_stereo_r on native (3 starts). Compare byte by byte. R2 passes only if both eyes equal the r5base shots.
2. If R2 passes: commit the pm_gpu.c R2 hunks on native (or merge vram3-r2). If it fails: revert the R2 hunks and keep R5.
3. Run fileselect_rotate on native once and read the V05 demand line.
4. R4 after the converter agent (pm_tex.c) is done.

## Update 2 (2026-11-04, after the coordinator reset: budget 8 more starts, used 8 of 8 by the end)
- R2 FAILED and is reverted (branch vram3-r2 deleted). Gate: yard_stereo_r on native vs r5base shows missing ground and dark stripes at the left edge (base_R.png vs new_R.png in the scratchpad). Top right-eye shot differs in 66156 bytes. Left-eye top shot 0000 is byte equal. Bottom screen differs in about 11300 bytes in both eyes (console text, build string, not compared). Cause not decided. Likely the depth clear (C3D_CLEAR_DEPTH) does not finish before the right draws, or the left depth state is wrong. Do not retry without a diagnosis.
- R5 open point CLEARED for fileselect_rotate (start 12, R5 build): no V05 demand line in the whole run (pm_log_e always prints). Debug offuse trace was not on (its switch is at debug level). One run only.
- R4 DONE: commit 4b3e8db7e on native (3ds/src/pm_gpu.c only). Full slots are RGBA5551, 256 KiB each (BLK_FULL16). tex_hdr takes the format. Snap copy of a full image uses w*h*2 bytes. Grab texture stays RGBA8. A full request with siz not 2 draws on the screen (log "R4: full off-screen image with siz", first 20). pm_tex.c unchanged: C3D_TexBind takes the format from the texture.
- Measured R4 gates: pause_spin (start 14) 10 of 10 shots byte equal to texf1_pause. yard (start 16) top shot byte equal to texf1_yard. Boot cap 3136 KB (928 A + 2208 B). Two starts (13, 15) wrote no shot file to the archive (the run stopped at the until line); the repeats (14, 16) passed.
- Cap history: 1856 (HEAD) -> 2752 (R5 + R2) -> 3136 (R4 + R5, R2 reverted). Expected with R2 reverted and R4 on: 1856 + 512 (R5) + 768 (R4) = 3136. Matches.
- Not done: offdump (debug, V05 switch offdump) reads 4 bytes per texel of full images; it prints garbage for full images now. Debug only.
- Budget: VRAM3 at 8 of 8 (csv). Next starts need a reset.
