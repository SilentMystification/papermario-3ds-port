# Handoff: sun rays fix (SUNFIX, rows SUN1 to SUN3)

## State
- No fix. Sun rays (fx_sun, Dry Dry Desert, sbk_00) still show cells and streaks in sun_sbk. Shot unchanged by every edit tried.
- Build: build3ds/pm_3ds.3dsx is archived build 1526 (3ds/logs/builds/pm_3ds_1526_542f2d72d.3dsx, sha256 2ed9666b...). Not replaced. Catalog not run. No push.
- Azahar starts (row SUN2): 4 of 8 used. sun_sbk stays unverified in azt_scenarios.json.
- Debug probes (switch suntile, debug log only): suntile_t0 (tile 0 rows), suntile t1 draw (tile 1 state), pm_sun_base (sun list words).

## Facts
- Tile 1 at the ray draw: I4, tmem 256, window 219 0 471 124, src 0x5046a6, w 256 h 32 (diag_sunrays.md SUN3 fact 3).
- ROM sun block (0x40BBA0 to 0x40C5A0, splat effect_gfx_sun) is dropped by the packer (ROM READ IN DROPPED RANGE). Keeping it did not change the shot. Not in the tree.
- Base list SETTILESIZE tile 0 is not logged. Cause open.

## Open items
1. Bytes at 0x5046a6 against ROM 0x40BDA0.
2. Writer of TMEM 256 (LoadBlock path, DLC replay in 3ds/src/pm_dlc.inc.c).
3. Quarter texel of tile 1 (uls 219 gives 3 quarter texels): small, untested.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=SUN3 PM_BUILD_CAP=900 sh 3ds/build.sh; cp 3ds/src/map_restore_table.c.bak 3ds/src/map_restore_table.c
- Test: PM_ROW=SUN2 powershell -NoProfile -File 3ds/tools/azt.ps1 sun_sbk -Row SUN2 -Try
- Shot compare: python 3ds/tools/shot_cmp.py <dirA> <dirB>
