# agent_playfix handoff

## State
- Branch native, HEAD 896506297. No push. map_restore_table.c not committed (user file, restored after each build).
- Azahar starts: TEX1 7 of 8, UI2 3 of 8. HANG1 1.

## Commits (native)
- 4d68fae60 HANG1: idle vblank wait polls the counter, 40 ms cap, logs timeout (pm_main.c). Azahar yard passes.
- b8b04b9f7 azt: nok14_lake scenario (TEX1, unverified); mac01t until f899 -> f783 (f899 never printed). mac01t passes 6 of 6 in 27 s.
- fc602ce72 UI2: 16 row HUD image draws 16 rows (hud_element.c). battle_start: 3 px change on a HUD icon (x 201, y 88-90). Enemy bar not in shots.
- 896506297 azt: caps from measured runs (yard 27, mac01t 23, nok14_lake 20, battle_start 20 s).

## Branch
- tex1-clamp e8896aa97: clamp-pad fix (pm_tex.c expand, pm_tex_tx1.h). Yard and mac01t shots byte-equal to HANG1. nok14 shot: only a moving sprite differs. Untested on the subject. Not on native.

## Open rows
- HANG1a: one full-height grab per frame for water_splash.c, scenario nok11_splash, cap 15 s. Not started. The copy is made per window in grab_cur (pm_gbi.c:266, rows y to y+h). Open question: how pm_tex_load (cfb pointer) finds a pre-made copy. Not read.
- HANG1a conflict note: K1 removed the GRAB_MAX cap (pm_gpu.c:1838). Decision made: cap only the splash, keep K1.
- HANG1b frame begin pm_gpu.c:1956: no change, watchdog phase already names it.
- TEX1: find a nok_14 shot with lake terrain in view (pos= and camera), test tex1-clamp, then merge into native. Current nok14_lake (27 0 -44, entry 0) shows no terrain change; entry 0 and position unverified.
- UI1: stereo excluded (user fact). Candidate: rainbow palette path msg_draw.c:1347. Test on the Toad House menu, 3D slider 0, fps60 off. Not started. fps60 is off by default (pm_interp.c:83).
- UI2 rest: enemy bar bottom row proof (need a battle window with the enemy bar), danger D (hud_element.c:1895), star points depth, HP block HUD update, Bombette bar, star meter, blue switch drawlog. Not started.
- AUD1: dropped (23 voices kept, effect return needs channel 23).
- Caps: other scenarios still at 45 or 90 s. Time them and set cap = measured + 5 s.

## Findings
- Near-plane clip is absent (clip_bits, pm_gbi.c:881). The path flicker is a missing clip, not the odd-w flip (V02 homogeneous cull).
- Sky wrap split already exists (background.c, 78b4d2db1).
- Texture pad: expand and tx1 pad follow the mask period on clamp axes. Fixed on tex1-clamp only.

## Commands
- Build lane: `PM_LANE=<lane> sh 3ds/build.sh` (run `py 3ds/tools/gen_map_restore.py` before, restore map_restore_table.c after).
- Azahar: `PM_ROW=<row> powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 <scenario> -Row <row> -Build <lane>`.
- Justification: one line in 3ds/logs/perf/status_<row>.txt before each start.
