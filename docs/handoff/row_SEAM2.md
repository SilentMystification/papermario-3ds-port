# Row SEAM2: seams on building geometry that come and go with the camera (user, 2026-11-09)

## User facts
User: "I also still see seams on building geometry sometimes. Fix those." (towns, e.g. mac_*). Investigate by code, not shots.

## Cause (shared oracle, code read, main 00a4e1901), ranked; one check per class, a false class moves to the next
1. (c2) T-junctions left open by SEAMFIX. The PICA tests the pixel centre after a 1/16 snap, so a T-junction opens a background
   line that moves with the camera (the N64 hid it with coverage + VI). SEAMFIX (nd_seam.c, built per shape at nd_shape.c:133-161)
   skips points across lists with their own matrix or inexact vertices: nd_seam.c:195 (skip_mtx), :196 (skip_ref). XL console log:
   `seamfix map mac_01_shape tp 365 ins 264 skip_mtx 101 skip_ref 0`. Buildings are separate models with their own transform (cls 2).
   Check: the seamfix line per map (mac_01, mac_00, mac_02) plus a cold switch line listing the first 8 skipped points (list address,
   receiver model, world position).
   Fix: lists with a static matrix (R1 static-matrix bit 8: finalMtx == savedMtx, or bakedMtx) enter the soup in world space with that
   matrix; the point-on-edge test runs there; the split vertex is inserted in the receiver's own local space by the edge parameter
   t = dot / L^2 (interpolate position, UV, colour of the receiver edge; no inverse matrix). Skip only lists whose matrix changes per
   frame. Host test: extend the nd_seam host test with a two-list fixture (identity + translated matrix), expect one insertion.
2. (c4) bilinear at a texture edge: covered by row TEXKEY for cutouts; only if 1 is false, an offline shape scan of the named model.
3. (c3) coplanar decal depth fight (DECAL_EPS 0.003, pm_gpu.c:385-388): only if the seam model is DECAL_*; offline check.
4. (c1) edges not bit-identical between paths: lowest; nd_mtx host test only if 1-3 are false.

## State (2026-11-09; agent stalled and stopped)
Branch SEAM2 05792cdd7: T points of lists under their own matrix split in object space; seamfix mac_01 ins 264 -> 365, mac_00
488 -> 666; spot_mac01_det 953 px differ on building edges; nd_host PASS. Not merged: the run-time static-matrix check (R1 bit 8,
finalMtx == savedMtx) is not written, so a list whose matrix moves per frame could get wrong points. Next: that check, then merge.

## Start state
`sh 3ds/tools/wt_new.sh SEAM2`, branch SEAM2. PM_ROW=SEAM2, PM_LANE=SEAM2. One Haiku agent.

## Touch only
3ds/src/nd_seam.c, nd_shape.c (only the SEAMFIX build call, :133-161; row HC4 edits nd_shape.c later: keep it minimal), the nd_seam
host test, azt_scenarios.json, bench_defs.json.

## Done criteria
nd_host PASS (new fixture). seamfix skip_mtx about 0 on mac_01, mac_00, mac_02 (log). One picture of a mac_01 building edge. Byte
check spot_mac01_det and spot_mac00_det vs stored base: they will differ at seams; report the diff pixel count and bbox. memcheck OK.
Delete this row file in the completing commit. Budget: 6 Azahar starts. 2 tries per named cause, then hand back with the values.

## Rules
CLAUDE.md. STOP RULE: 90 minutes or 150 tool calls. No Monitor or tail loops.
