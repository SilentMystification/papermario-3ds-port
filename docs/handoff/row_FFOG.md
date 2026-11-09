# Row FFOG: Forever Forest floor missing (fog) and background items culled on the wrong side (user, 2026-11-09)

## User facts
O3DS XL, main 21e3b029b: "Floor in Forever Forest does not render, showing just the background", sometimes, in some rooms (mim_*).
mim_01: "the culling of the background items is not right, things pop in and out weirdly": items on one side vanish when you turn
the other way (the cull turns the wrong way relative to the camera; same report as before FFOREST). Investigate by code.
The user reports are the faults; if Azahar does not show them, hand back with your exact setup so the orchestrator asks the user.

## User facts 2 (2026-11-09)
Floor: no pattern ("random"), any mim room. Cull: when the camera turns right, items on the LEFT side vanish (while still on screen).
Azahar (agent, base f8a962bac): not shown; cull plane signs (nd_shape.c:277-292) and cull_cross (nd_geom.c:53-59) correct by code.
Diagnostics merged 8bc6b761f (wzcull, dlcwz line); the dlcwz line never printed on mim_01, so the range cull may not be the path.
User (2026-11-09): plays in Widescreen, 60 FPS OFF. Oracle: the game bounds cull (model.c:3149-3290, wsCullX 1.25, wide aspect in
mtxPerspective) and the port range culls are symmetric; the 60 FPS interpolation cause is ruled out at 30 FPS. Next suspect: a stale
`combined` for IGNORE_MATRIX records at the native hit (pm_gpu.c:3160 model_vis_same, :3286 empty entry): the vis mask uses the
walker matrix left by the previous task. Old idea (orchestrator): in widescreen the visible width is 1.25x the 4:3 frustum; a cull that uses 4:3 planes or the game's own
4:3 visibility test drops items at the screen edges that only wide mode shows. Check whether the user plays in Widescreen.

## Floor (shared oracle, code read)
mim_01 ground o401 (SURFACE_OPA, y 0, +-550, 44 triangles, edges up to 508 units). Forest fog 990..1000, colour (0,0,0), black
background. Fogged map models draw through pm5. Cause 1: 3ds/shaders/pm5.v.pica:135-143 `alpha = clamp(fogu.x * z/w + fogu.y,
0, 255)`, 1/w = rcp(r5.w) (:75): a vertex behind the eye (w < 0) or near w = 0 gives full fog (black), and the clipper spreads it
over the visible floor. FFOREST's z clamp (:68-73) does not touch r5.z. Fix: fog only for w > eps: `slt r13.x, myc.eps, r5.w`,
`mul r12.x, r12.x, r13.x` before the clamp (behind/at the eye: fog 0). Other suspects if this check fails: typed-op state after a
native draw (HC4a nd_op_same), native vis-mask empty hit (pm_gpu.c:3160-3163, :3286; census ndmhit empty).

## Culling (code read in this row)
Find the cull used for map models and entities in mim (native range cull dlc_box_out / pm_dlc_vis_mask in nd_shape.c, planes from
dlc_planes; any port cull of background models). Name the plane build (file:line) and check the sign and the matrix order against
the camera (view vs view-projection, row vs column major, the wide 1.25 x scale). A wrong sign or a transposed matrix culls the side
opposite the view. Fix it; gate: mim_01 turn in place (padscript) with no item vanishing on screen.

## Start state
`sh 3ds/tools/wt_new.sh FFOG`, branch FFOG. PM_ROW=FFOG, PM_LANE=FFOG. One coder agent. Scenario mim01_ring_det (exists).

## Done criteria
nd_host PASS, memcheck OK. mim01_ring_det: floor present in all shots, distance fog kept; a turn-in-place run: no on-screen item
vanishes (log the cull count per frame before/after). spot_mac01_det byte-equal (no fog there). Delete this row file in the
completing commit (also row_FFOREST.md if both faults pass). Budget: 8 Azahar starts. One check per suspect, then the next.

## Rules
CLAUDE.md. STOP RULE: 90 minutes or 150 tool calls. No Monitor or tail loops.
