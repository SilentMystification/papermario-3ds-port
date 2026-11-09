# Row WALLT1: wall textures (4-bit aux tile with a SetTileSize origin) in hos_01, kmr_10 and other maps

## User facts
User (2026-11-09): hos_01 wall texture is corrupt: "line by line a texture renders down and then it loops. Each time is not the right
texture". kmr_10: "not all, but a lot of the walls" are wrong. Do not confirm visually before the fix (code trace done by the oracle).

## Start state
Worktree build3ds/wt/WALLT1, branch WALLT1 from main (hash in the first commit). PM_ROW=WALLT1, PM_LANE=WALLT1.

## Cause (oracle)
3ds/src/nd_tex_set.c tex_tile1, V21 branch lines 446-454, line 452:
`src += (size_t)(ult >> 2) * (size_t)l1stride + (size_t)(t1x >> 1);`
For a 4-bit tile 1 loaded by LoadTile (bpp 0, l1v 1) this moves the source pointer by the SetTileSize origin and keeps the window size;
td[1].x0/y0 (463-464) subtract the same origin in the UV map (nd_comb.c:653-654), so the window reads memory past the aux raster. On the
RDP, SetTileSize only moves coordinates ((s - SL) & mask over what LoadTile loaded). Tile 0 already follows that rule (lines 168-173, V13).
- hos_01 walls o178/o179 (panner 4), o208/o211 (panner 5): aux pan moves 0..128 texels -> the window slides down through memory.
- kmr_10 cliffs (kmr_layer_c2tif): SPECIAL 0x5F aux offset (model.c:1617-1626, 3708-3711), k0 offsetT 200 -> src aux+800 B, rows
  50..113 of a 64-row raster. o1329 (offset 0) renders right.
Same class: kmr_03, kmr_05, kmr_11, obk_03, obk_06, mac_01 (static); mac_00/02/05/06, nok_11-14, kzn_*, sam_02/06, kkj_*, kpa_32/33/102,
end_00/01, sbk_02, osr_00/01, obk_07, hos_20 (panned).

## Plan
1. Check first (code only, no Azahar start): decode kmr_10 k0 SPECIAL offsetT = 200 (tools/build/mapfs ShapeFile digest, print only);
   read nd_tex_set.c:405-468 and confirm the V21 branch is taken (tile_siz[1] = 0, l1v = 1). Status line with the values.
2. Edit only nd_tex_set.c tex_tile1: before the VIS3 block (about 418) `const int w_win = w;`. Before `else if (src && l1v) {` (446):
   else if (src && l1v && nd_ws.tst1.tile_wrap[1][2] && nd_ws.tst1.tile_wrap[1][3] &&
            w_win == (1 << nd_ws.tst1.tile_wrap[1][2]) && h == (1 << nd_ws.tst1.tile_wrap[1][3])) {
       /* WALLT1: window = the whole mask period of the loaded image (aux pan: hos_01; model aux offset: kmr_10). The RDP reads
        * (s - sl) & mask from what LoadTile put in TMEM: source = LoadTile origin, SetTileSize only moves coordinates (as V13). */
       src += (size_t)l1y * (size_t)l1stride + (size_t)l1x;
       stride = l1stride * 2;
   }
   Keep t1x = uls >> 2 there (no & ~1); td[1].x0/y0 unchanged. V21 comment: now only sub-windows of a sheet (fire_breath, dust).
3. Build, memcheck OK. Commit with the new scenario/bench and the hw note (shifts and compares only, no data).
Not in this row: the quarter-texel fraction of tile 1 SL; nok_11 kabemizu pan growth.

## Done criteria
1. nd_host PASS.
2. Byte gates vs build-at base: mdl5_iwa10_det equal; battle_start_f182_cold equal (a diff only in a kmr_03 field frame cliff region is
   allowed); spot_mac01_det equal or a diff only on the mac_flotif flower beds (shot_cmp.py box); nok11_det equal or a diff only on the
   kabemizu wall water. A diff anywhere else: revert.
3. New wall_hos01_pic (warp=hos_01:0 story=ch0_toad_town shots shotwin=200:2 loglevel=debug, until `shot 1 frame 201`, cap 20): the walls
   o178/o179 show the jimen soil with a smooth, cleanly tiled overlay, no foreign rows, no growing band.
4. kmr_10_band (existing): the strata stripe on the low cliff next to the ? block is continuous, no garbage rows (compare with o1329).
Affected scenarios (no others): the ones above. If a picture does not change: revert; next try dual_bind t1_p offsets (nd_comb.c:653-654).

## Rules
CLAUDE.md rules 5, 7, 9, 11. No git stash, no toggles, no stubs. Never commit 3ds/src/map_restore_table.c. Padscripts in the azslots sdmc.

## At the end
Delete this row file in the commit that completes the row. SendMessage main "WALLT1 ready <hash>".
