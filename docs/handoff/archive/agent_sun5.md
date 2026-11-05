# Agent SUN5 + ROMKEEP check (epic SUN5+ROMKEEP)

## State
- SUN5: I4 offset fix applied in 3ds/src/pm_gbi.c (tex_op, tile 1, V21 branch). NOT committed. Partial: the cells in the sky strip are gone; the dash pattern on the lower ray streak remains. Row not finished.
- Binary: build3ds\pm_3ds.3dsx built 00:22 with the SUN5 edit (includes ROMKEEP commit 6571d97d5 from HEAD).
- ROMKEEP check: boot and memory line pass, heavy map pass, sun_sbk pass (earlier run). No commit.

## Cause (proven by reading)
- pm_gbi.c V21 branch (tile 1, `src && l1v`) is reached only when bpp == 0, that is I4. The branch chain before it fails for bpp 0.
- Old code: `src += ult>>2 * l1stride + (uls>>2)`. uls>>2 is a texel count. For I4 the byte index is texel>>1 (pm_tex.c nib(): `src[(y*stride+x)>>1]`, high nibble first). So the old offset was 2x too far.
- td[1].x0 = uls>>2 (texel units) is the UV origin in dual_bind (pm_gbi.c t1_p[2]). The old src and x0 disagreed by the same factor.
- Sun draw values (old, from diag_sun4 log): src +54 bytes (texel 108), x0 54, stride 256 texels.

## Fix (new path, no toggle)
- t1x = (uls>>2) & ~1 (even loaded origin). src += (ult>>2)*l1stride + (t1x>>1). td[1].x0 = t1x.
- Sun draw new values (by code): src +27 bytes (texel 54), x0 54, stride 256. Even offset, so no parity loss.

## Other I4 draws (code reading)
- Only the V21 branch changes. Its entry condition is tile 1, I4, and an earlier LoadTile (l1v). Draws with uls < 4 texels give t1x = 0: no change.
- Tile 0 I4 path (pm_gbi.c near 3240 `tile_siz[0] == 0 && r_v`): not edited. Same unit pattern exists there (r_x in bytes). Not changed by this row. Open.
- Text glyphs: tile 0, CI4 (fmt 2, siz 0) glyph path (pm_gbi.c near 2713). Not on the V21 branch. Unchanged.
- Not enumerated: other tile 1 I4 draws with uls >= 4. None found by this check.

## Test results
- SUN5 run 1 (sun_sbk, PM_ROW=SUN5, -Try): log checks 3 of 3. Shot 0000 frame 200 vs earlier shot (sun_before_0000.ppm in agent scratchpad): 4007 px differ, bbox x 63-220, y 119-348. Cells gone. Dashes present on the lower streak (gaps). Not a pass.
- ROMKEEP yard (boot and memory): pass. `rom resident 25747KB of 40960KB (packed), cap 47742KB, margin 21994KB`. `mem split ... margin 9240KB`. Both at least 1.0 MB.
- ROMKEEP forest_mim (heavy map, mim_05): 5 of 5 log checks pass. Shot: Mario and partner in Forever Forest, HUD present.

## Open items
1. SUN5 dashes. Remaining cause not named. Hypothesis: transparent texel runs in the ROM row (zero texels at 0-3, 12-15, 176-179 and so on; period 12 at texels 0-15) are read by the window. Test needed: log the first and last texel index (s) that the ray draw samples, with td[1].x0 and w, for one draw. Name the value used.
2. The loaded width for an odd uls is w (window length), one texel short when t1x was lowered. Only odd uls is affected. Not hit in sun_sbk (uls 219 gives 54, even).
3. Tile 0 I4 path has the same byte/texel unit pattern (pm_gbi.c near 3240). Separate row, not in scope.
4. Shared tree: the other agent's runs overwrite build3ds\last_log.txt and sdmc log.txt. Two of my logs were lost. Check Get-Process azahar before every start, and copy the log right after the run.
5. The ROMKEEP binary includes the SUN5 edit. The memory line is from the same build.

## Commands
- Build: `python 3ds/tools/gen_map_restore.py`, then `sh 3ds/build.sh` (Git Bash), then restore 3ds/src/map_restore_table.c from the user copy.
- SUN5 test: `set PM_ROW=SUN5` then `powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 sun_sbk -Row SUN5 -Try`
- Crop: Python PIL on build3ds/azt/shots/sun_sbk/0000_10_0.ppm, crop (150,130,240,340), 4x NEAREST.
- ROMKEEP: `azt.ps1 yard -Row ROMKEEP`, `azt.ps1 forest_mim -Row ROMKEEP -Try`.

## Budget
- SUN5: 1 start of 8 used.
- ROMKEEP: 3 starts used (yard run 1, log lost to a concurrent run; yard run 2; forest_mim).
- The ROMKEEP sun_sbk check ran under row SUN5 (1 start, counted there).
