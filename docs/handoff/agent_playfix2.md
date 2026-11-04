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
