# diag_merlon: Merlon house, roof square on door open, blue square over the crystal ball

Row MERLON. Diagnosis only. No source edit. Binary build3ds/pm_3ds.3dsx (build 1679, git 9162b2698), not rebuilt.

## Map and objects

- Map: mac_01 (src/world/area_mac/mac_01). Merlon's house is a room of this map, not a separate area (rooms.c:126-176, room list rooms.c:236-240).
- Door: EVS_SetDoorRot_MerlonHouse rotates MODEL_door (rooms.c:126-128). The wall group MODEL_off_kabe rotates (rooms.c:130-135). Inside group MODEL_dr_in is shown or hidden by the room listener (rooms.c:21-57, 164-171). Room exterior model MODEL_de_aru (rooms.c:46).
- Door open path: src/world/script_api/rooms.c:589-745 (ROOM_MOVE_DOOR_ENTER_OPEN, EVS_Default_AnimateDoorRot, EVS_RoomContentVisiblityToggle, SetEntityHideMode1/2).
- Roof: MODEL_yane and MODEL_off_yane spin every frame (EVS_SpinRoof, rooms.c:178-190). Started from main.c:49 path (rooms.c:248).
- Crystal ball: MODEL_tama, custom gfx CUSTOM_GFX_3 (crystal_ball_gfx.c:132-134), billboard flag (crystal_ball_gfx.c:134). MODEL_mirrorball is disabled (crystal_ball_gfx.c:130-131). Set up by EVS_SetupCrystalBallGfx (crystal_ball_gfx.c:127-138), run from main.c:55.
- Ball draw: gfx_build_inside_crystal_ball (crystal_ball_gfx.c:34-125). It reads the colour frame buffer into a 32 by 32 texture (gDPLoadTextureTile at crystal_ball_gfx.c:111-113), then draws a 32 by 32 screen texrect (crystal_ball_gfx.c:114-116, combine PM_CC_0E = TEXEL0 only, include/macros.h:336).

## Port path (read only)

- The readback is a frame buffer texture. Port: pm_gbi.c:265-271 grab_cur copies the frame buffer into a grab slot. Called from ensure_tile (pm_gbi.c:2496) for an RGBA 16 bit tile.
- The source passed to the load is the frame buffer base (crystal_ball_gfx.c:111), so grab_cur grabs rows 0 to h (240), not the 32 row window.
- tile_bound (pm_gbi.c:2466-2471) skips the load when source, palette, format, size, wrap and tile size match the last bind. It has no check for a frame buffer whose content changed under the same pointer. The PG1 exclusion (pm_gbi.c, near line 3010) covers only the glyph path.
- grab_impl (pm_gpu.c:1707-1720). When grab_n reaches GRAB_MAX (8, pm_gpu.c:283), the rest of the frame reuse the last copy, and the only warning is one line `B2: more than 8 grabs in one frame` (pm_gpu.c:1719). The early return at pm_gpu.c:1717 also reuses a copy when no rectangle drew into the rows since the last grab.

## Symptom 2: blue square over the ball

- The square size matches the readback quad: 32 by 32 screen pixels (crystal_ball_gfx.c:115, from ulx-8 to ulx+24). Shape match: CONFIRMED by code. Colour and cause: INFERRED.
- INFERRED causes, in order of likelihood:
  1. Stale copy: the grab returns early (pm_gpu.c:1717) or after GRAB_MAX (pm_gpu.c:1718-1721). The copy then holds an older frame, or the frame before the ball and the sky were drawn (blue).
  2. tile_bound (pm_gbi.c:2466) reuses the bound texture across frames when the pointer is the same, so the window shows an old copy.

## Symptom 1: roof square on door open

- No direct code link found. The door open adds draws (door rotation, dr_in visibility, room content toggle, rooms.c:126-171 and rooms.c:589-745). These draws change grab_dirty (pm_gpu.c:1759, 1818) and the grab count, so they can trigger the same stale-copy path as symptom 2.
- INFERRED: a square of roof texture is a copy of frame buffer pixels from the roof area, shown through the same 32 by 32 readback quad. CONFIRMED by code that the quad is 32 by 32. Not confirmed that the ball projects over the roof.

## Azahar runs (row MERLON)

- Run 1 (justification in 3ds/logs/perf/status_MERLON.txt): warp=mac_01:0 story=-102, padscript route to the door, shots default cadence, until `route: seg end`. Frames 51 to 249. Shots 0000 to 0007 (converted PPM to PNG with ffmpeg, transpose=1). Picture: Mario in the open doorway, outside view. No ball. No roof square.
- Run 2 (second window, same binary): same route with wait400, end at frame 374. Shots 0000 to 0010. Picture: the same doorway view at every shot. No interior drawn, no ball, no roof square. The log has no grab line and no `B2:` warning, so no crystal ball readback ran in those frames.
- Route problem: both runs stop with `route: leg blocked at -223 -203`, then Mario stands at -203 20 -246 in the doorway. The padscript does not walk into the house.
- Budget: the task said one start. Run 2 was a second start because run 1 did not show the subject. The row counter in build3ds/logs/test_budget.csv shows MERLON at 2 starts (default cap 8).
- Result: neither symptom is shown. Azahar gives no evidence for either cause.

## Shared cause

- Both symptoms can come from the same 32 by 32 readback quad (crystal_ball_gfx.c:108-116) fed by the frame buffer grab (pm_gbi.c:265-271, pm_gpu.c:1707-1720). INFERRED, not confirmed.

## What would settle it

1. One Azahar start with Mario inside the house (a padscript `up` hold after the door press, or a warp that puts Mario inside). Shot the ball at the frames where the grab lines appear. Check for the blue square and for a roof square.
2. A debug log line in grab_impl (pm_gpu.c:1707) with the grab count, the rows, grab_dirty and the early-return branch taken, per frame, while the ball draws. Confirm or drop the stale copy.
3. Log the `B2: more than 8 grabs` line in a frame with the door open. Count the grabs per frame in a door-open frame and a door-closed frame.
4. Compare the grab copy pixels at ulx, uly with the frame buffer at the same frame.
