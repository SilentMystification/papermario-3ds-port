# Row SEAMFIX: seams in world geometry (T-junctions and gaps in the map meshes)

## User facts
User (2026-11-09): "Seams still appear in various levels ... in multiple places", "in world/level geometry", "on the ground, on walls
and probably other places". No visual confirmation before the fix (code trace by the oracle).

## Start state
Worktree build3ds/wt/SEAMFIX, branch SEAMFIX from main (hash in the first commit). PM_ROW=SEAMFIX, PM_LANE=SEAMFIX. One Opus agent
(oracle: the topology work is Opus-sized), no subagents.

## Cause (oracle, ranked)
R1 (main): T-junctions and small gaps in the map mesh data. Vertices within 1.5 units of a boundary edge, inside it: mac_01 266 coplanar
+ 154 angled (186 across two display lists; 188 gap 0, 232 gap 0.25-1.5 units), mac_00 243+306, kkj_14 140+166, iwa_10 186+134, kmr_20
391+308, nok_11 16+32. The N64 RDP writes a pixel with any of 8 coverage samples hit and the VI AA filter heals it; the PICA tests the pixel
centre with 1/16 px snap, so gaps of about 0.3-1.5 px show as background lines and gap-0 T-points as pinholes. Nothing repairs the
topology: nd_dlc.c:20-150 (pass 2 112-146) copies Vtx and indices verbatim; nd_shape.c:127-153 and the replay loop (about 515-565) draw
them; fallback groups (nd_shape.c:305ff dlc_run_group, pm_gbi_run) too.
R2: one world point through two matrices (identity model vs bakedMtx group, model.c:2893-2900, nd_recs.c:97, nd_mtx.c:116): about
0.01-0.05 px error, flips the 1/16 snap: dotted cracks. mac_01 55 of 148 models under non-identity groups (5 integer translation only).
R3 (residual): per-triangle near-plane clipping. R4 (texture): uvso has no +0.5 texel (nd_model.c:36-39); do not change in this row.
The old seam test (nd_seam_test.c) only proves native == old path; it never tests one point drawn by two triangles.

## Plan
1. Host tests (nd_seam_test.c, run by nd_host_test.sh): step 3 walks the shape tree, tags lists (identity, integer translation, other),
   counts boundary-edge T-points with int64 tests (0 < dot < L2, |cross|^2 <= eps^2*L2, skip |coord| > 8192). Step 4 rasterizes mac_01
   with a camera from an ndref kind-5 record: dp4_fp24, divide in fp32, 1/16 snap, pixel-centre top-left fill vs an any-of-8-samples
   fill; holes split by cause. Runtime log at the first compile of each map: `seamfix map <name> tp <n> ins <n> skip_mtx <n> skip_pool <n>`.
   Commit 1: tests + logging-only table builder (pixels unchanged, byte gates equal).
2. Map-wide T-point table on the walker side, lazy at the first dlc_build after pm_dlc_gen changes, from gMapShapeData; identity and
   integer-translation lists in world space; key (list, triangle ordinal, edge), T positions sorted by dot; grid hash 64-unit cells, int64,
   eps 1.0; heap (about 40 KB worst), freed in dlc_reset; cold noinline, no division in loops, no double.
3. nd_dlc_compile: NdDlcEnv callback tpts. Pass 1 adds the extra vertices/triangles to nvtx/ntri; pass 2 replaces each affected triangle
   by a fan from the opposite vertex through the sorted T points (winding kept; gat stays right). New Vtx position = the T vertex s16
   exactly (minus the receiver's integer translation); tc and colour/normal interpolated and rounded; flag 0; extend the group box.
   Entity compiles pass NULL. Pools: GSTAT_VTX 16384, index area 131072; sfull stays 0.
4. Residual, named not fixed: rotated/fractional baked lists (skip_mtx), fallback groups, R3.
Shaders unchanged; hot draw loop unchanged; compile-time only. No toggle, no stub. Commit 2: the compiler inserts the points.

## Done criteria
nd_host PASS incl. step 3 (T-points in compiled output = 0 for identity and integer-translation lists) and step 4 (mac_01 holes drop to
the residual; print before/after). Commit 1 byte-equal on spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det 0000-0015.
Commit 2: region diff base vs new on those scenes: changed pixels <= about 0.5%, every region thin (largest <= 64 px), on T-point edges;
nok11 changes least. Picture gate: new seam_mac01_east (warp=mac_01:0 pos=360:20:-280 det shots shotwin=200:6, cap 25) shows the floor
gaps of lists 115/180 closed; second spot outdoors near PT43 (Merlon's house, from the seamfix log list). memcheck margin >= 256 KB.
Console proof (user): PT43 mac_01, PT10 kkj_14.

## Rules
CLAUDE.md rules 5, 7, 8, 9, 11. No git stash, no push, no attribution, no toggles, no stubs. Never commit 3ds/src/map_restore_table.c.
DIRECT (Opus) changes nd_shape.c/nd_rec.c paths: merge main before each commit group.

## At the end
Delete this row file in the commit that completes the row. SendMessage main "SEAMFIX ready <hash>".
