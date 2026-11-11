# Row HUDRING: the walker vertex load wraps the ring at a new mesh (late HUD triangles dropped when the ring chunk is full)

(Model: Haiku. 60 lines or fewer. The agent reads only this file, CLAUDE.md and what this file names.)

## User facts (bugs only)
As row HUDGM: the battle wheel circles (HID_Spotlights, HID_HighlightSpot; lit IA8 quads, src/hud_element.c:1569-1627) do not render;
the icons and text draw. O3DS XL.

## Cause (oracle code read; the second path that can drop them, row HUDGM is the first)
- nd_geom_vtx (3ds/src/nd_geom.c:296-379) copies a load to the ring with pm_gpu_gvtx_load (pm_gpu.c:3018-3025), which returns -1 when
  `nb.gring_n + n > GRING_VTX` (16384, pm_gpu_int.h:68); then the load has no ring slot (nd_geom.c:325, gv_fb[GVR_RING]) and every
  triangle of it drops (nd_geom.c:291-293). pm_gpu_gtrip also refuses when the index ring is full (pm_gpu.c:2824 `nb.gidx_n + 3 >
  GRING_IDX`). Rectangles and sprites wrap to a new ring chunk first (ring_need, pm_gpu_rect.c:75-79; pm_gpu.c:2323, :3933); the
  walker loads do not ("the walker paths keep the real limits", pm_gpu_rect.c:16): their earlier vertex slots point into the chunk.
- The HUD draws last in the frame, so in a heavy frame it is the first to lose its triangles; the texture rectangles still wrap and draw.
- Safe point for a wrap: a load at slot 0 after a triangle starts a new mesh (nd_geom.c:314 clears vert_live): no live slot of the old
  chunk is read after it.

## Start
`sh 3ds/tools/wt_new.sh HUDRING`; PM_ROW=HUDRING PM_LANE=HUDRING on every call.

## Audit (oracle): names used and their declarations
ring_need: pm_gpu_rect.c:75 (prototype pm_gpu_int.h:74, not included by nd_geom.c: the block-scope extern of edit 1 is the only one
there); pm_gpu_gv_room: pm_gpu.h:22 (included, nd_geom.c:11); vert_ring, vert_live: defined in nd_geom.c (:27; sizeof is the array).

## Touch only
3ds/src/nd_geom.c: nd_geom_vtx only, the block at :314 (`if (v0 == 0 && tri_since_vtx) memset(vert_live, ...)`). WALLSHADE edits
nd_geom.c gv_params_calc and pm_gbi_gv_at (:61-88): not those lines. `git merge main` before the completing commit.

## Exact edits
1. Replace `if (v0 == 0 && tri_since_vtx) memset(vert_live, 0, sizeof vert_live);` (:314) with:
   `if (v0 == 0 && tri_since_vtx) {`
   `    memset(vert_live, 0, sizeof vert_live);`
   `    if (!pm_gpu_gv_room(n + 64u)) { extern int ring_need(unsigned nv, unsigned ni); if (ring_need(n + 64u, 384u)) memset(vert_ring, 0xff, sizeof vert_ring); } /* HUDRING: a new mesh wraps the ring chunk (as rectangles do); the slots of the old chunk are void */`
   `}`
   (pm_gpu_gv_room is pm_gpu.c:3017; 64 spare vertices cover the wide edge copies and gtrip_copy of the mesh, 384 indices its triangles.)
2. No other edit. vert_ring is the GV_NONE-filled array the task reset uses (nd_cen.c:537).

## Gate (CLAUDE.md 2; the wheel is the user's console check)
- `PM_ROW=HUDRING PM_LANE=HUDRING sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK.
- `python 3ds/tools/bench.py run nd_host --row HUDRING --why "walker ring wrap"`: PASS.
- Byte compare on two scenes that do not draw the wheel: `bench.py base spot_mac01_det`, `bench.py base mdl5_iwa10_det`, one
  `bench.py run mdl5_iwa10_det,spot_mac01_det --row HUDRING --why "no wrap in a light frame: unchanged"`, `cmp` each: 6/6.

## Stop
A failed gate, an edit that does not fit the code, or a test-tool fault: commit, then hand back the exact output (CLAUDE.md 4-5).

## At the end
Delete this row file in the commit that completes the row. Hand back: the ready hash, the gate results, the memcheck line.
