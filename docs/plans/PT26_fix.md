# PT26 (PT05): Merlon's door, camera moves weirdly. Result: reproduced the script, no port fault found

Build 970 (3ds/logs/builds/pm_3ds_970.3dsx), Azahar, row MERLON, 4 starts (all used). Evidence level: the door scene runs in Azahar. FS1 does not change the camera (tested). Root cause: NOT PROVEN.

## Repro commands
Script file: sdmc:/3ds/PaperMario/routes/merlon.txt (`go_-250_-230_600`, `go_-250_-300_200`, `up+A*8`, waits and `A*6` for the messages, then `up+A*8` again). Mario stops at the door at x -202, z -242 (go_ reports "leg blocked at -202 -242").
- Run 1 (every 30th frame shot): `PM_ROW=MERLON sh 3ds/run_azahar.sh -x 3ds/logs/builds/pm_3ds_970.3dsx -F -t 150 -u "route: seg end" -d "warp=mac_01:0 story=-106 partner=1 route=merlon shots loglevel=debug fixedseed statehash=30 fs1off"`
- Run 2 and 3 (one shot per game frame, frames 495 to 754): same switches, `shotwin=495:260` and `fs1off` (run 2) or `fs1force=3` (run 3).
- Run 4 (control, no BargeOut): `story=-107 route=merlon2` (walk to -215, -158).
- Logs: 3ds/logs/perf/merlon/ (r1.txt, off/, f3/, walk/; git-ignored). Shots are PPM 240x400, rotate by 90 degrees.
- story=-106 is STORY_CH0_MET_STAR_SPIRITS. The door scene needs story from -106 up to -103 (below CH1_SPOKE_WITH_MERLIN -102). Visit 1 shows MSG_MAC_Plaza_001F and sets GF_MAC01_Merlon_Shoo3. Visit 2 runs N(EVS_MerlonBargeOut) (src/world/area_mac/mac_01/npc.c:391). With story -107 and lower, the door only shows messages 001C, 001D, 001E and has no camera command.

## What happens in the scene (log and shots, run 2)
- f500: Mario is at (-215, 1000, -158) (SetPlayerPos, npc.c:431 area). He stays at y 1000 until about f790. f800: Mario at (-222, 45, -158), f900: (-222, 20, -158).
- Camera: the camera is CAM_UPDATE_FROM_ZONE (mac_01/main.c:116). Between f520 and f540 the view changes from a wide view of the door to a close view of the house wall from the left. At f600 Mario is not on screen and the camera is closer and lower than in the walk control (run 4, Mario at -198, 20, -144). No camera script command runs in this part of BargeOut: only ShakeCam (npc.c:411, 5 steps) comes before f600. UseSettingsFrom, SetCamSpeed 90, PanToTarget come at npc.c:482-484, SetCamType, SetPanTarget, SetCamDistance 200, SetCamPitch 15 -10 at npc.c:497-501, ResetCam at npc.c:505.
- So the visible camera move at f520 to f540 comes from the zone controller (src/cam_mode_zone_interp.c:585-800). It reads the target from gPlayerStatus pos (src/7B440.c:137, 77480.c:755). The ray test test_ray_zone_aabb(x, y + 10, z) (cam_mode_zone_interp.c:16, 694) runs from y 1010 down. The zone of the last hit triangle wins (src/collision.c:874-882: no nearest test). This code is the same as the decomp except two port guards (cam_mode_zone_interp.c:~625 yinterpRate < 1 reset, ~693 cs == nullptr fallback to controlSettings, commit 2ae937272).

## FS1 test (camera with and without frame skip)
Same route, same shotwin. Mean absolute pixel difference (grey, 0 to 255) per frame, fs1off against fs1force=3: f500 0.4, f520 2.2, f540 0.3, f560 1.1, f580 1.5, f600 5.4 (speech bubble timing), f620 1.1, f640 0.3, f660 0.2, f680 0.5, f700 0.1, f720 0.0, f740 0.7. Camera angle and zoom are equal in both runs at the same game frame, side by side (contact sheet looked at for f500 to f980). Run 3 drew 166 of 260 frames, so the skip ran. Result: FS1 draw skip does not change the camera path. This matches the code: update_cameras runs from step_game_loop (src/state_world.c:18), not from gfx_draw_frame. render_frame (src/cam_main.c:98) only reads camera matrices and calls mdl_update_transform_matrices. The statehash differs between the two runs from f60 on, because the route input follows the pad scan count and gRandSeed is draw coupled. This is a known effect (docs/plans/route_notes.md) and is not a camera fault.

## Root cause
- Proven: the camera move seen at the door in BargeOut is the zone camera following Mario. It is the decomp behaviour (same script, same zone code). FS1 is not the cause.
- Likely (not proven): the user sees this scripted move, and the O3DS makes it look worse: the scene takes 15 to 25 game steps per second there, so ShakeCam (5 steps, 2.0 amplitude, src/evt/cam_api.c:258) lasts 0.2 to 0.33 s instead of 0.17 s and the zone interpolation (per step) takes longer.
- Guess: a console-only fault in camera or collision math with float precision (test_ray_triangle_down on the zone mesh with start y 1010). It cannot be shown in Azahar. Not tested: story -105 to -103, the door with a partner out, N3DS.

## Fix text
No source fix is justified by the evidence. Do not change cam_mode_zone_interp.c or the FS1 code for this row. If the user confirms the camera moves in the Merlon scene only (not at other doors), a test patch for the graphics lane is: log gCameras[0].targetPos, curYaw, curPitch, boomLength, curSettings pointer and gPlayerStatus.pos every step under a switch `camlog` in pm_warp.c (next to pm_statehash, line 459). Run once with fs1off and once with fs1force=3 and compare the columns. This was not done here because the source is locked for this row.

## What the user must send
1. Story state (last chapter or item list) and the visit number at the door (1st: message, 2nd: Merlon knocks Mario out).
2. Which part moves: at the start (Mario flies back), while he lies on the ground, or when Merlon walks (the pan to -250, 20, -280).
3. Three photos 0.5 s apart, or a short description: shake, jump to another angle, zoom, or a pan that runs back.
4. The game log of that run with `loglevel=debug` (pull-o) and the shot of the same moment in Azahar: `warp=mac_01:0 story=<same story> route=merlon` gives the Azahar view for comparison.
