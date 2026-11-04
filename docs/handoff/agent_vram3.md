# Handoff: VRAM3 (code agent, branch native, session 4)

## State
- BIS1 done for the wall: commit 982bdddbc (pm_tex.c back to 124e1d468, texword line logs filter). Wall 0x0100169c binds again (run 6, texture frame 38, wrap 3 3, filter 1). Details: docs/handoff/agent_slot1_texd1.md, section BIS1.
- BIS1 open: 0x00571b90 (tik_03 coin) not bound in entry 0 within 300 frames. Values not obtained.
- VRAM3: not started (no code change, no Azahar run). Step 1 (trace of the 4 off-screen full images) is an investigation: give it to a fresh agent (CLAUDE.md rule 3).

## Facts for VRAM3 (read, not verified on console)
- OFF_N = 4 full images (pm_gpu.c:279). full_blk[OFF_N] reserved at boot (pm_gpu.c:355, 365). A pass uses full_tex_init (pm_gpu.c:383-390), no VRAM alloc at run time.
- V05 log lines at pm_gpu.c:1681 to 1691: "more than 4 off-screen images in one frame" (64-wide images use OFF_N + off_sused, full images use off_used % OFF_N). The first V05 line shows demand above 4 in one frame.
- Full images are PM_OFF_W x PM_OFF_H, RGBA8 (tex_hdr, pm_gpu.c:~374). RGBA5551 would halve them (1024 KiB total for 4 at 320x240 RGBA8, 512 KiB in 5551; check the sizes in the code before the change).
- Savings chosen by the user: (a) RGBA5551 for off-screen full images, (b) fewer off-screen images by reuse, (c) one shared depth buffer for both eyes (375 KiB). The freed bytes go to the texture cache.

## Next steps
1. Fresh investigation agent: log, per frame, when each full image is live (pass start to last read). Scenario: yard (kmr_20 entry 4) and mac_01; count of distinct off-screen full images per frame (V05 lines). Log only.
2. Decide (a), (b) or (c) from that count. Keep the code change on a branch until the gates pass.
3. Gates: boot cap line, yard, mac_01 and pause_spin shots, battle_start_fb frames 0 to 210 equal except the RGBA5551 quantisation.
4. Justification line in 3ds/logs/perf/status_VRAM3.txt before each Azahar start. Budget VRAM3: 0 of 8 used.

## Commands
- Build: `PM_ROW=VRAM3 PM_LANE=native sh 3ds/build.sh`
- Test: `PM_ROW=VRAM3 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 <scenario> -Row VRAM3 -Build native -Try`
- Old-build worktree (124e1d468, lane bis): C:\Users\Andrew\Documents\GitHub\papermario-3ds-port-bis. Remove with `git worktree remove` when no longer needed.
