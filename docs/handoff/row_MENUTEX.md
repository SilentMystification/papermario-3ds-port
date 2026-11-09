# Row MENUTEX: Frame skip to General, and a Texture filtering debug folder

## User facts
User (2026-11-09): "frame skip should come out of render and go into general." "make a debug texture filtering option that has
toggles / settings for texture filtering available."

## Start state
Worktree build3ds/wt/MENUTEX, branch MENUTEX from main (hash in the first commit). PM_ROW=MENUTEX, PM_LANE=MENUTEX.

## Code facts
- pm_options.c: dbg_lbl[] (about 356) has "Frame skip" (RW_DBG + 7) shown in the Render folder; tab_ids (about 385) builds the General
  tab (tab 0: RW_FPS, RW_WIDE, ...) and the Debug folders (RW_RENDERF "Render >").
- Texture filter today: pm_tex.c:1791-1793 tex_filt from the game's G_TF bits (V15: sampler state at bind, not in the cache key);
  pm_tex.c:1959 and 2045 C3D_TexSetFilter (debug switch texnear forces nearest); native model path in pm_gpu.c / nd_tex.c binds the same.
  Texel centre: nd_model.c:36-39 uvso has no +0.5 texel for bilinear (the N64 centres texels at integer s, the PICA at +0.5).
  Hardware: nearest, bilinear, mipmaps with nearest or linear between levels; no anisotropic filtering, no MSAA.

## Plan
1. Move the Frame skip row from Render to the General tab (after Widescreen). Same variable and behaviour; delete the old row.
2. New Debug > Render > "Texture filtering >" folder (debug tool; values in RAM, saved with the other debug options if the Render rows
   are saved):
   - Filter: Game (default, the G_TF bits as now) / Bilinear all / Nearest all.
   - Texel centre fix: Off (default, as now) / On (+0.5 texel offset for bilinear draws, so bilinear matches the N64 centre).
   - Mipmaps: Game (default) / Off (base level only), only if the port uses mip levels (check; omit the row if not).
   - Each change logs `texfilt: <row> <value>` and takes effect at the next bind (no cache flush needed: the filter is sampler state;
     the centre offset is a uniform/UV offset: read where uvso is applied).
   Replace the texnear debug switch with the Filter row's Nearest value (one control, delete the switch path) only if the switch is
   not used by a scenario; else keep the switch as the boot value of the row.
3. The defaults give the same pixels as now.

## Done criteria
1. nd_host PASS. memcheck OK.
2. Byte gates with the defaults: spot_mac01_det and battle_start_f182_cold byte-equal to base.
3. One Azahar run (menu by padscript, warp=random): shots of the General tab with Frame skip, the Texture filtering page; then
   Filter = Bilinear all: one field shot where the textures are smoother than the base shot; log lines `texfilt:`.
Affected scenarios (no others): the menu scenario, spot_mac01_det, battle_start_f182_cold.

## Rules
CLAUDE.md rules 5, 7, 9, 11. No git stash, no stubs. Never commit 3ds/src/map_restore_table.c. Padscripts in the azslots sdmc.

## At the end
Delete this row file in the commit that completes the row. SendMessage main "MENUTEX ready <hash>".
