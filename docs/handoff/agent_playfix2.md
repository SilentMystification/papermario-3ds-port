# agent_playfix2 handoff

## State
- Branch native, HEAD on entry 465c8f81b. No push. map_restore_table.c not committed.
- Branch hang1a-cap 9a0e4b877: HANG1a change (pm_gbi.c grab_cur splash branch, water_splash.c PM_SPLASH_ON/OFF marks, gbi_custom.h, scenario nok11_splash, catalog row). Untested, not merged.
- Azahar starts: HANG1a 2 of 8 (both crash). TEX1 7 of 8. UI2 3 of 8. UI1 0 of 8.

## HANG1a blocker
- nok11_splash (fxtest=89, nok_11 entry 0) stops with svcBreak in gxCmdQueueAdd one frame after "shot 12 frame 403".
- Same stop on baseline (lane hang1base, no splash change). Cap cannot be read until this is fixed.
- Logs: build3ds/lane-hang1a/last_log.txt, build3ds/lane-hang1base/last_exception.txt.
- Investigation: docs/handoff/diag_hang1a_svc.md. Constant not found (libctru source not on PC). Proposed fix is INFERRED only.

## Open rows (not started)
- TEX1: 1 start left in the row (7 of 8). Find a nok_14 window with lake terrain, test tex1-clamp e8896aa97, merge into native.
- UI1: menu text and glove drawn twice. Toad House menu, 3D slider 0, fps60 off. Candidate palette path msg_draw.c:1347.
- UI2 rest: danger D (hud_element.c:1895), star points depth, HP block HUD update, Bombette bar, star meter, blue switch drawlog, enemy bar proof.

## Commands
- Build: `py 3ds/tools/gen_map_restore.py`, then `PM_LANE=<lane> sh 3ds/build.sh`, then restore src/map_restore_table.c from the user copy.
- Test: `PM_ROW=<row> powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 <scenario> -Row <row> -Build <lane>` (add -Try for an unverified scenario).
- Justification: one line in 3ds/logs/perf/status_<row>.txt before each start.

## svcBreak cause (INFERRED, from source; not measured)
- Queue limit: citro3d gxQueue.maxEntries = 32 (pica2 c3d/source/base.c:91). gxCmdQueueAdd breaks when numEntries == maxEntries (libctru gxqueue.c:64).
- Queue runs only at C3D_FrameBegin or the frame end path (renderqueue.c:122, 240).
- In-frame texture load: C3D_SyncTextureCopy calls C3D_FrameSplit then queues one GX_TextureCopy (renderqueue.c:430-435). C3D_TexLoadImage uses it (texture.c:122).
- The splash loads one window per particle (water_splash.c:206, 40 parts). That is about 40 queued copies in one frame, more than 32. Grabs add a few more.
- Fix needs a drain: gxCmdQueueWait on the bound queue when the count nears 32. The bound queue pointer is not public (ctx->gxQueue in C3Di_GetContext). Open decision: (1) expose the queue pointer and wait at about 24 entries, or (2) one full-frame load per splash frame and UV sampling (bigger change in water_splash.c and pm_gbi.c).

## Update (HANG1a done on native)
- Break fix: pm_gx_room (pm_gpu.c) drains the GX queue past 24 entries. Called before the splash texture copy (pm_tex.c) and grab copies (pm_gpu.c).
- G_NOOP removed from g1_skip (pm_gbi.c) so the splash marks reach the handler.
- Azahar nok11_splash lane hang1a3: no break, 21 frames, one grab per frame (rows 0-240). Shot a at frame 412. Picture: faint droplets near Mario at 240x400, needs a person to confirm.
- Native HEAD: merged hang1a-drain (a4d3a2aed). Starts: HANG1a 4 of 8.
- Remaining rows: TEX1 (1 start left), UI1 (0 of 8), UI2 rest (3 of 8).
