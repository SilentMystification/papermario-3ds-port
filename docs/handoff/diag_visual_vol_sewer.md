# Diagnosis: volcano fog (A) and sewer/ruins wrong texture (B)

## Status
- No Azahar run was made in this pass. No shots exist. No source edit, no log added, no build.
- Findings are static (code read). Each cause still needs its one test.

## Repro (to run next, PM_ROW=vis)
- Volcano: `PM_ROW=vis powershell -File 3ds/run_azahar.ps1 -Turbo -Capture -Timeout 90 -Smoke` with the switch `warp=kzn_19:0` (kzn_19 is cave room, linked, 14 shots seen before). Use `tour=kzn_19:0,kzn_08:0` for a second map. kzn_08 is NOT linked (rom skip), so use only linked kzn maps.
- Sewers: `warp=tik_03:0` (tik_03 is in the demo chain, linked). Add `tour=tik_03:0,tik_06:0`.
- Ruins: `warp=isk_04:0` (demo chain, linked). Add `tour=isk_04:0,isk_00:0`.
- Check the map names first with `3ds/tools/route_scenes.py` on the tour log. Names are unconfirmed for the exact fog map.
- Shots: build3ds/last_shot.png per run. Save each to 3ds/logs/vis/<map>_<frame>.png.

## Map names
- kzn = Mt. Lavalava (volcano). tik = Toad Town Tunnels. isk = Dry Dry Ruins. Confirmed from the area prefixes in docs/HISTORY.md (kzn_19, kzn_02, tik_07, isk_14).

## A. Volcano fog (static, not verified against N64)
Ranked:
1. Fog factor fallback for w <= 0. `3ds/src/pm_gbi.c:1948` uses `poly[i].z * 32767.f` when `poly[i].w <= 0`. That constant has no N64 basis. Vertices near or behind the near plane get a wrong fog alpha. Lava lakes and big floor quads hit this. Test: count w<=0 vertices per map with one log line in the fog branch (phase gate). Pass if the count is > 0 on kzn_19.
2. GV5 fog uniform, `3ds/src/pm_gbi.c:1296-1298`. `fogu[0] = fm / za`, `fogu[1] = fo - fm*zb/za`, from `pm_gpu_gv_zcoef`. If the zcoef fallback is wrong, the fog offset is wrong for every fog draw. Test: set `nofog` (switch PMDL "nofog") on the same shot. The fog layer must disappear. Compare the GV5 and CPU paths (`gv_fog_gpu_ok`, pm_gbi.c:1389) with the same shot.
3. Fog colour alpha in the mix. `3ds/src/pm_gbi.c:599` and `:602` set the mix weight to `fogc & 0xff` (mode 1) or to 0xff (mode 2). The N64 blender uses the shade alpha for FOG_SHADE_A. A wrong fog colour alpha gives a flat fog, not a tiled one. Test: log `fogc` once per map; compare with the DL G_SETFOGCOLOR word.
4. "Tiled" look is more likely from a texture, not from fog. The darkness/stencil path (docs/HISTORY.md:1277) reads an uninitialised I8 texture with mirror and clamp. Affected maps are listed there (kgr, omo, kpa, jan, kzn_08). It is not in the volcano linked set. Test: `xtrace=120:1` on the volcano map. If the draw is a texrect from an address never written, the cause is 4, not 1 to 3.

## B. Wrong texture (sewers and ruins, cache hits)
Ranked (line numbers from docs/handoff/review_tex_cache.md at HEAD 12c5e9887; grep at HEAD 1f99f88a6 matches 866 and 1295):
1. Partial-overlap invalidation missed. `3ds/src/pm_tex.c:790, 799-800`: only the slot start address is tested. A write into the middle of a slot leaves old texels. Test: log, on each invalidate, the start, end, and the slot img and slen. Pass if a write overlaps a slot but not its start.
2. RGBA8 composite lookup has no `slot_valid` and no dirty check. `pm_tex.c:1323-1337`, key built from addresses only (`pm_gbi.c:2414-2421`). A source change in place keeps the old composite. Test: log key hit and the shash at hit vs load. Pass if the shash differs and the hit is kept.
3. Bind of a dirty slot has no hash check. `pm_tex.c:606-617`. The DLC path is safe (`pm_dlc.inc.c:857`). Test: count binds where slot dirty=1 and shash mismatch.
4. Mario sprite buffer address reuse. Mario's animated texture is uploaded to an address that a map texture later uses. The hash compare at `pm_tex.c:866` runs only on a hit. Test: log SETTIMG address, fmt/siz, w/h, slot key, shash, hit or miss, and Mario's upload address and slot in one run. Pass if the same address and slot serve both.
5. Render-to-texture sampled by the wrong draw (see 4 in A). Not tested.

## Log to add (phase-gated, PM_LANE=vis, not yet added)
- In the SETTIMG tex load: addr, fmt, siz, w, h, slot, shash, hit (0/1), sampled slot.
- Only one commit, only if the test needs it.

## Next
- Run the repro list. Save shots. Run the single test per cause in order 1 (A) and 1 (B).
- Do not start a code agent until a test names a line.
