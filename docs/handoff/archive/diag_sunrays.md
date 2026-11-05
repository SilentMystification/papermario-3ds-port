# Diagnosis: sun overlay and god rays, Dry Dry Desert (SUNRAYS)

## Status
- Cause not proven. No source edit. No build.
- Azahar: 1 start of 3 used. Scenario `sun_sbk` added (unverified, run with `-Try`).
- Binary: build3ds\pm_3ds.3dsx (16:36). Git HEAD at run: 521642b9e. [M]

## Effect path [C]
- Map call: reference/paperboat/src/world/area_sbk/sbk_00/main.c:24 calls N(SpawnSunEffect). Most sbk maps do the same.
- reference/paperboat/src/world/common/todo/SpawnSunEffect.inc.c:5 calls fx_sun with FX_SUN_FROM_LEFT.
- 3ds/src/effect_loads.c:739 fx_sun: load_effect(0x82), then sun_main.
- reference/paperboat/src/effects/sun.c: sun_render sets RENDER_MODE_CLOUD_NO_ZCMP (dist 10). sun_appendGfx sets a screen ortho (+-1600 by +-1200), prim alpha alpha>>1, and five ray layers.
- Each ray layer sets tile 1 with gDPSetTileSize: uls = offsetS + 44*i, width 63 texels, height 31 texels. offsetS scrolls per frame.
- Ray meshes come from the ROM. The 3DS build runs them through pm_gbi.c.

## Display lists, decoded from ver/us/baserom.z64 [M]
- Segment 9 is ROM 0x40BBA0. Base list 0x898: SETTIMG I8 at 0x09000200. Tile 1 is I4, 256 by 8 texels, line 16. Clamp S (cms=1, masks=8). Wrap T (cmt=0, maskt=3, period 8 texel rows).
- Ray lists 0x7C0 to 0x850: each draws 1 or 2 F3DEX2 quads (G_TRI2 0x06) from 4 or 8 vertices.
- Vertex UV in the ray meshes: s from -17.47 to 16.0, t from 0 to 66.12. Vertex alpha 255, 229 or 126.
- The texture rows 0 to 7 at ROM 0x40BDA0 are identical (128 bytes per row, nibble pattern 0000 cccc cccc 0000 5555...). The data varies in S only.

## Azahar shots, run 1 (frames 200 and 204) [M]
- Files: build3ds\azt\shots\sun_sbk\0000_10_0.ppm and 0001_10_0.ppm. Log checks: 3 of 3 pass.
- Screen is turned 90 degrees in the PPM (240 by 400).
- Sun disc is not clear in either shot.
- Sky strip, x 150 to 240 and y 130 to 340, at 4x: faint translucent cells with hard edges. Cell pitch about 19 screen px, cell width about 10 px. The cells lie along one diagonal.
- Two thin light streaks at the right edge look dashed (gaps).
- Frames 200 and 204 look alike by eye. No pixel compare run.

## Hypotheses (ranked) [E]
1. Tile 1 S clamp at the window edge. The ray vertices use s below 0 (-17.47 to -1.47). The N64 clamps to the window column at uls, and uls changes per layer and per frame. The port keeps the clamp on the GPU (pm_tex.c:120-124 wrap_of; pm_gbi.c:1146-1149 V21 comment, linear map in dual_bind). A clamp edge that sits at the texture edge, not at uls+63, gives hard bands that move with the scroll. Most likely. Untested.
2. T wrap period. Rejected for now: rows 0 to 7 are identical [M], so the 8-row repeat cannot make the seams. Only a T read beyond the loaded rows could, and the mask holds that off.
3. Fold of one draw across the window. pm_gbi.c:1131-1133 states that a wrap inside one draw is not reproduced. The ray draws span 64 texels in T and 16 in S. The fold (pm_gbi.c:1134-1160, t1_fold) moves the start but not the inside wrap. Possible seam source. Untested.

## Smallest fix (not applied)
- If hypothesis 1: in the aux texture build (pm_tex.c:1049-1058 and the wrap copy at pm_tex.c:328-334, 368-371), build the S axis at the tile window width with the clamp edge at the window, and set aux_shr_s only when the tile clamps. Keep the fold off for a clamped axis (already done at pm_gbi.c:1149).
- If hypothesis 3: skip t1_fold for a draw whose extent is more than the mask (pm_gbi.c:1139 test `ex[a] >= wsz`).

## Picture check plan
- Log one line per ray draw (pm_gbi.c, tile 1 path): ox1, oy1, t1_w, t1_h, t1_p[0..3], wrap S and T, draw u0 u1 v0 v1. Phase gate, debug only. One run.
- Compare with the sun_sbk frame 200 shot. Pass if the hard cells map to one of the logged values (clamp edge or fold origin).
- Fix pass: the sky strip has no cells in frame 200 and 204, and the ray streaks have no gaps. Read the shot by eye.

## Next
- One run of sun_sbk with the logging line (budget left: 2 starts). Then one fix run.
- Azahar may show the same cells as the console. Azahar lacks an N64 or console reference, so the console shot is needed for the final judgement.

## SUN1 result (row SUN1, debug switch suntile, azt sun_sbk, 2 starts used of 8)

Corrects the first section above. Evidence is from the ROM bytes and the two logs (3ds/logs/perf is not used; logs in the scratchpad).

- Tile 1 is MIRROR in S, not clamp. SETTILE cms=1 is G_TX_MIRROR (bit 0); clamp is bit 1 (cms=2 or 3). The port maps it the same way (pm_tex.c wrap_of: mode & 2 clamp, mode & 1 mirror). Hypothesis 1 as written (clamp at the window edge) is rejected [M].
- Tile 1 window per layer at f200: uls 981, 986, 985, 174, 219 (10.2), that is texel 245, 246, 246, 43, 54. The window is 63 texels wide, 31 tall. Mask 8 (256 texels), T repeat mask 3 (8 rows). The port loads the image from column 0 at mask width (VIS3 branch, pm_gbi.c near 3067) and offsets by -uls. So the port texel is mirror256(s - uls). This is the N64 model [E]. Not verified on pixels.
- Open difference: the quarter-texel part of uls (uls & 3, here 1) is not in the tile 1 offset (t1_p[2] uses td[1].x0 = uls >> 2 only). Size: 0.25 texel. Effect on the shot not measured [E].
- Tile 0 is the ray texture (I4, line 1, window 16 by 64; S repeat mask 4; T mirror mask 6). The SETTIMG image is I8 128 wide at 0x09000200 (ROM 0x40BDA0). The tile 0 source row stride in the port is set at pm_gbi.c near 3224 ("4-bit from 8-bit" branch, stride = 2 * timg_stride). Whether the N64 reads these rows at 8 bytes per tile line (the TMEM layout of the LoadTile with line 1) is NOT checked. This is the next candidate [E].
- Ray vertices from ROM (group 0, verts 0 to 3): s 0 16 16 0, t 0 0 64 64 texels (tc / 32). The vert_st values in the log were stale: gv_xf fills vert_st only when the CPU path needs it. Do not read vert_st in the GPU path.
- Sun base list D_09000898 (ROM 0x40C438 to 0x40C518) has no G_TRI and no G_VTX. It sets textures and modes only. The sun disc is therefore not a triangle of this effect. Ray groups 0 and 1 (ROM 0x40C520, 0x40C538) draw two quads each. Groups 2 to 4 were not decoded [M].
- Effect spawn and draw confirmed: "sun draw f200 alpha 255 fromRight 0 scroll 245.30 235.62 224.41 10.70 10.77" [M]. The camera does not decide the sun position: the effect is a screen ortho overlay (+-1600 by +-1200).
- Shot 0000 (frame 200), 2x read by eye: blue sky at the right edge, two diagonal dashed light streaks at the right edge, no sun disc. The frame has no clear disc because the effect has none [E].
- pm_log lines are cut at about 200 characters. Split long log lines.

## Next (SUN1 continues, budget 6 starts)
1. Log td[0] at the tile 0 draw: source pointer offset, stride, w, h, bpp, and the TMEM address (tile_tmem[0]), one line each under 200 characters.
2. Compare the port tile 0 row layout with the N64 read at line 1 (8 bytes per row, 16 rows per 128-byte source row). Decide from the ROM bytes at 0x40BDA0 (I8 image rows 0 to 7) what the N64 texel rows are.
3. Only then choose the fix (SUN2). The scenario camera has no sun in view to fix: the sun is a screen overlay. Scenario view change is not the fix.

## SUN2 attempts (row SUN2, 5 starts used of 8). Result: no fix. Edits reverted.

- Model check, ROM bytes (python, not in repo). TMEM rows from a LoadTile of an 8 texel wide I8 source at 0x09000000 (source row pitch 8 bytes, TMEM row pitch 128 bytes), read by the tile 0 line 1 (8 bytes per texel row, I4 16 texels): texel rows 0,4,8,12,16,20,32,40,48,56,63 are a ramp of centre values (0 at T 0, 2 to c at T 4 to 12, 0 at T 16 and T 32, nonzero from T 40). The same read of the 0x09000200 image rows (the port source at the ray draw, rows 0..7 are "cccc"): row T 8 and up show MIPS code bytes (27bdffb0 ...). So the two sources differ by a large amount.
- Port source at the ray draw (run 4 and run 5 logs, frame 200): the LoadTile of tile 7 at TMEM 0 logged img 0x139b380 stride 296. The ROM decode says the SETTIMG before it is FD880007 (width 8) at 0x09000000. The port value is not the ROM value. The segment 9 mapping or the SETTIMG width in this build is not confirmed. Open.
- The TMEM record for TMEM 0 is absent at the ray draws ("gather none", run 4). Only the tile 1 record (TMEM 256, img 0x504670, stride 128) is present. Cause not found.
- Gather patch (tile 0 rows read from the TMEM model): built, run once (run 1 and run 2). Pixel diff of sun_sbk 0000 against the before shot: 0 px changed. Reverted with a reverse patch (no commit).
- Shot pixels before SUN2 (sun_sbk 0000): unchanged by any SUN2 build. The fault is not yet named.

## Ranked candidates (numbers as far as known)
1. Tile 0 source image at the ray draw. The port source (0x09000200 region, or img 0x139b380 stride 296) differs from the LoadTile source (8 texel wide I8 at 0x09000000, Model A). Expected effect: large, all rays. Test: log the SETTIMG and the LoadTile values (imglog switch exists) and compare with the ROM.
2. TMEM row pitch. Tile line 1 (8 bytes) against 128 byte load rows: the port reads source rows; the RDP reads TMEM words. Only matters after 1 is fixed.
3. Tile 1 quarter texel offset: uls & 3 = 1 (0.25 texel) is not in t1_p[2] (pm_gbi.c dual_bind). Size 0.25 texel. Small.

## Next
- Resolve candidate 1 first: compare the SETTIMG width and address of the DL in RAM with the ROM at 0x40C490 (run with imglog=200:1 on sun_sbk, one start).

## SUN3 result (row SUN2, budget reset, 4 starts used of 8). No fix. Data edit reverted.

- Run 1 (debug only, suntile tile 0 row probe): tile 0 at the ray draws is RGBA16 (fmt 0, siz 2) 32 by 32 at 0x56e880, not I4. The SUN1 tile 0 note (I4, 16 by 64) is the base list value, which this log does not show (see fact 2).
- Fact 1, dropped ROM: "ROM READ IN DROPPED RANGE 0040bba0 n 2560" [M]. 0x40BBA0 to 0x40C5A0 is splat segment effect_gfx_sun (type code). The RR1 code rule in 3ds/tools/gen_rom_remap.py drops it (the drop table entry 0x0035B9D0 to 0x00E3B870). fx_sun gets zeros for the sun list and images.
- Fact 1, test: keep line `0x0040BBA0 0x0040C5A0 effect_gfx_sun` in 3ds/rom_code_keep.txt, regenerate 3ds/src/rom_remap.h. The RAM sun list at +0x898 then reads e7000000 00000000 / d7000002 ffffffff (RAM words are little-endian; ROM bytes match). Shot sun_sbk 0000: 0 px changed (runs 2 and 4, shot_cmp EQUAL 2 of 2). Edit reverted: keep line removed, rom_remap.h regenerated and equal to HEAD.
- Fact 2: the base list SETTILESIZE of tile 0 (ROM 0x40C4C0, window 0 0 60 252) is never logged, even with the list read at +0x898 [M]. Logged tile 0 windows after the sun draw are 0 0 124 124 and 0 0 60 60 (other draws). Cause not found. Candidate: the DL compile and replay path (3ds/src/pm_dlc.inc.c, G_SETTILESIZE at lines 589, 1174, 1198), which may skip or replay the op. Not checked.
- Fact 3, the value the ray draw uses [M]: tile 1 at each sun draw (4 draws at frame 200, log line "suntile t1 draw"): fmt 4 (I4), siz 0, tmem 256, window uls 219 ult 0 lrs 471 lrt 124 (texel 54.75 to 117.75 by 0 to 31), td1 src 0x5046a6, w 256, h 32, stride 256, x0 54, y0 0. cur_tile -1. The tile 1 image is 0x5046a6, not a segment 9 address (segment 9 is 0xd202c0 in this run). Not yet checked: the bytes at 0x5046a6 against ROM 0x40BDA0.
- Candidates from SUN2 (tile 0 source, TMEM row pitch, quarter texel): tile 0 is not the texture of the ray draws in this run (the draws bind tile 1). Candidate 1 (tile 0 source) is weakened. Candidate 3 (quarter texel of tile 1, uls 219 gives 3) is still open and small. Not tested in SUN3.
- Debug probes kept (switch suntile): tile 0 row bytes and window (suntile_t0, 3ds/src/pm_gbi.c), tile 1 state at each draw (suntile_tri), sun base words (pm_sun_base, called from src/effects/sun.c under TARGET_3DS).

## Next (SUN4)
1. Read the bytes at td1.src 0x5046a6 (row 0 and row 1, 16 bytes each, stride 128 bytes) in the RAM log, and the ROM bytes at 0x40BDA0 (I8 rows, 128 bytes each). Decide the source row of the tile 1 image: LOADBLOCK at 0x09000200 (ROM lrs 510 lrt 28) into TMEM 256.
2. Find which code writes 0x504670 (the tile 1 TMEM record of SUN1, stride 128): the LoadBlock path in pm_gbi.c (G_LOADBLOCK) and the DLC replay.
3. Check the DLC replay for the base list (pm_dlc.inc.c) with one probe on the base list SETTIMG and SETTILE values.
4. Restore build3ds/pm_3ds.3dsx from 3ds/logs/builds/pm_3ds_1526_542f2d72d.3dsx after any debug build (sha256 2ed9666b...).
