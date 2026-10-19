# V23c plan: two hand cursors on the pause map

Status: read-only check, no device run.

## Likeliest cause
The pause hand has HUD_ELEMENT_FLAG_DROP_SHADOW (src/pause/pause_main.c:625).
hud_element_draw_rect draws the hand twice: first the shadow, then the hand (src/hud_element.c:1264-1266).
The shadow copy sits at +2,+2 pixels (src/hud_element.c:308-310) and has a dark tint.
On the 3DS screen this dark copy can look like a second hand, most of all with wide and 3D on.
The original game draws the same shadow.

## Causes ruled out
- pm_interp (3ds/src/pm_interp.c:122-165) only replaces matrices of paired leaves. It draws nothing.
- pause_map.c draws only the Mario marker (pause_map.c:244). It does not draw a hand.
- gPauseCommonHIDs[0] is drawn once, in pause_draw_cursor (pause_main.c:488).
- pm_gpu_tex_rect wide extension (pm_gpu.c:1183) only extends a rectangle on the game edge.

## Azahar test (budget row V23c)
1. Switches: fps60 on, wide on, 3D on (Azahar stereo mode).
2. Warp to a map, open pause, go to the map tab. Take 1 shot at 2x zoom on the hand.
3. Check: one hand plus a dark copy offset about 2 px down and right (x1.25 wide scale).
4. Repeat with 3D off. Check that the shadow offset differs between the eyes.
5. Control: build with the DROP_SHADOW flag removed at pause_main.c:625. The second hand must go.

## Next step
If the shadow is the cause, ask the user: keep the original shadow, or draw it with lower alpha.
