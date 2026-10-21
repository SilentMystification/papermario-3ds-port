# PT05: Merlon's door, camera moves weirdly (NOT reproduced, no root cause)

Map: mac_01 (Toad Town plaza). The door is the Merlon house room (src/world/area_mac/mac_01/rooms.c:126-176, CreateMapRoom in EVS_SetupRooms). Evidence level: GUESS for everything below. Azahar was not used for this item (3 starts: 1 lost to a wrong stop pattern, 1 for PT10, 1 for PT06 and PT10 together).

## Script path (read, not run)
- Door trigger runs EVS_EnterRoomDoor (src/world/script_api/rooms.c:589-720). It calls the listener N(EVS_RoomListener_MerlonHouse) with ROOM_UPDATE_ENTER_BEGIN.
- Story below STORY_CH0_MET_STAR_SPIRITS: N(EVS_MerlonShooAway) (mac_01/npc.c:362-389) shows MSG_MAC_Plaza_001C, 001D or 001E. No camera command. The listener sets LVar0 = -1 and EnterRoomDoor takes the cancel label (rooms.c:735-740). No camera move.
- Story at or above MET_STAR_SPIRITS, third visit onward: N(EVS_MerlonBargeOut) (mac_01/npc.c:391-520). It does ShakeCam(CAM_DEFAULT, 0, 5, 2.0) in a thread (src/evt/cam_api.c:258-330: guTranslateF into camera->mtxViewShaking, +/- 4 units every step, 5 steps, applied in src/cam_main.c:73-74). Mario goes to y = 1000 (SetPlayerPos -215 1000 -158, a knockdown entity stands in). At the end: UseSettingsFrom(CAM_DEFAULT, -250, 20, -280), SetCamSpeed(90.0), PanToTarget (npc.c:482-484).
- Only if the cancel path fails: EVS_EnterRoom_MoveCam (rooms.c:449-458): SetRoomCamMoveSpeed(1.796) then InterpCamTargetPos(0, 1, ..., 10 steps) then SetRoomCamMoveSpeed(-1). The function static `savedCamMoveSpeed` (rooms.c:297) holds the old speed.

## Leads (GUESS, none checked on a run)
1. The camera in this map is CAM_UPDATE_FROM_ZONE with SetCamLeadPlayer false (mac_01/main.c:116-119). The zone controller reads the player position. While Mario stands at y = 1000 in BargeOut, the camera may follow y. The decomp does the same, so this is a difference only if the port reads another player position. Check: log gPlayerStatus.pos y and gCameras[0].targetPos each step during BargeOut.
2. ShakeCam amplitude is per step, not per second. At 15 to 25 steps per second the 5 shake steps last 0.2 to 0.33 s instead of 0.17 s. This feels longer but is not a bug.
3. FS1 skipped draws (pm_main.c:336-352) do not change camera state: the game step still runs (pm_gfx_retrace_skip calls gfx_func(3)). Not a cause.
4. A return value problem of ExecWait: if the listener's LVar0 = -1 is lost, the door opens although Merlon shoos Mario. Then EVS_EnterRoom_MoveCam and the player walk run while Merlon speaks. Check: log line `scripts` dump or a shot sequence at the door.

## What the user must provide
- The story state (item list or the last chapter) and which visit (first, second, third) of the door.
- A hardware shot sequence of the camera move (3 shots 0.5 s apart) or a screen recording description: which way does the camera go (shake, pan to the house, zoom, jump back)?
- The game log of that run with `loglevel=debug` (scripts, Merlon messages).
- With the answer, an Azahar test is cheap: `warp=mac_01:<entry>` with `story=ch1_start` and a padscript into the door (2 starts).
