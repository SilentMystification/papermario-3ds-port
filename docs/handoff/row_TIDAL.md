# Row TIDAL: Sushie's Tidal Wave water distortion is very slow (user, 2026-11-09)

## User facts
O3DS XL: "Sushie's Tidal Wave attack's water effects are super laggy. Investigate the cause and fix it."

## Cause (shared oracle, code read; costs estimated)
EVS_Move_TidalWave (src/battle/partner/sushie.c:1140) -> ProcessTidalWave (:202-370) -> fx_underwater(0, ..., 120) (:336).
fx_underwater draw (src/effects/underwater.c:158-293): (a) copy pass: colour image = nuGfxZBuffer, 40 strips of 6 rows from
nuGfxCfb_ptr (:167-186); (b) mesh pass: 18 x 12 = 216 quads (:207-285), each with its own SetTile + LoadSync + LoadTile of a 17-21 px
window, SetTileSize, 4 x gSPModifyVertex (G_MWO_POINT_XYSCREEN), gSP2Triangles; (c) a final list (:287-289). Each quad moves tile 0 to
a new window (ensure_tile rewin nd_tex_set.c:398-416), so uvso differs and each quad closes its batch; ModifyVertex takes the CPU
vertex path. About 216 closes + 2100 op decodes per frame, about 8-15 ms core 0, for 120 frames.

## Start state
`sh 3ds/tools/wt_new.sh TIDAL`, branch TIDAL. PM_ROW=TIDAL, PM_LANE=TIDAL. One coder agent.

## Steps
1. Check: a Tidal Wave battle scenario (new scenario + bench: battle with Sushie, force Tidal Wave by padscript or a debug word;
   until the fx_underwater end; cap 30 s) with the prof/census line: closes per frame and draws per frame in the wave vs before it.
   Expect about +200 closes. If not, hand back with the values.
2. Fix (underwater.c, TARGET_3DS only): one SetTile/LoadTile of the whole 320x240 z-buffer copy; the 19 x 13 grid in a few vertex
   loads (<= 32 vertices per chunk) with absolute texture coordinates (s = (x + dx) * 32, t = (y + dy + unk_23 offset) * 32, 10.5);
   the distorted positions computed on the CPU (the same unk_23 offsets the ModifyVertex calls apply) under an ortho matrix in screen
   pixels, not the frustum + G_MWO_POINT_XYSCREEN. One batch, no ModifyVertex.
3. Gates: nd_host PASS; picture during the wave (distortion visible, no seams between cells; compare with the shot before the fix);
   census closes per frame down by about 200; memcheck OK. Console (user): prof during the wave.
4. Commit; delete this row file in the completing commit. Budget: 6 Azahar starts.

## Touch only
src/effects/underwater.c, azt_scenarios.json, bench_defs.json.

## Rules
CLAUDE.md. STOP RULE: 90 minutes or 150 tool calls. No Monitor or tail loops.
