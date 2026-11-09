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

## State and reframe (2026-11-09, branch TIDAL 8dc8d067e)
Census (base): +2172 gbi ops and about +13 ms gbi per effect frame; closes only +42. Try 1 (patch scratchpad
tidal_underwater_strip.patch: one tile of the z-buffer copy, quarter-pixel grid) gave a flat blue block: geometry right, texture
not. Oracle: the z-buffer window resolves to the copy slot at any size (nd_target.c:91, nd_tex_set.c:41-44, :132-176, pm_tex.c
~1663-1680); a flat colour means the texture coordinates are in the wrong unit (tc must be texel x 32, S10.5, relative to the tile,
and gSPTexture 0xFFFF) - a factor 8 off with quarter pixels. New approach (try 2): skip the z-buffer copy pass; TARGET_3DS:
SetTextureImage(nuGfxCfb_ptr), one SetTile/LoadTile 0..319 x 0..239 RGBA16 clamp (ensure_tile -> grab_cur makes one GPU copy),
tc = (x*32, y*32) per grid vertex, gSPTexture(0xFFFF, 0xFFFF), the SBLK2 grab hold (G_PM_ND_GRABHOLD) around the mesh. First log
once: tex_ss/ts, td[0] x0/y0/w/h, off_ox/oy, uvso, tc range of 2 vertices.

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
