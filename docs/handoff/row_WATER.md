# Row WATER: water flicker (model cache hit binds the full shared-aux texture; tile 1 sub-texel offset)

## User facts
User (2026-11-09): "many different water scenes have the water flicker. Do not visually confirm, just figure it out."

## Start state
Worktree build3ds/wt/WATER, branch WATER from main (hash in the first commit). PM_ROW=WATER, PM_LANE=WATER. One Sonnet-size agent.

## Causes (oracle, code trace)
B1 (largest class: shared aux EXTRA_TILE_AUX_SAME_AS_MAIN, one LoadBlock, header mainH doubled (tools/build/mapfs/tex.py:82): tik_* taki,
nok suimen/kirakira, jan nami, flo water, hos_03-05 water, trd water/taki, kgr_02, kpa_133/134, sam_11, sbk_56 mizu1, iwa_10 t_taki1,
isk suna_4; also lava, mim_darkgreen2, mac_mirror2): on a miss pm_gpu.c:2862-2866 builds the draw with wdesc.h = td[0].h (32x32
window, uvso 1/32, hint hs = the 32x32 slot); on a hit pm_gpu.c:3124-3127 uses nd_tex_handle_desc (full 32x64, nd_model.c:116) ->
nd_tex_get -> nd_tex_unit_hint (nd_tex.c:113) loads/binds the 32x64 slot under uvso built for 32 rows. Pans step below one texel on many
frames, so the key repeats and the hit draws wrong; changing frames miss and draw right: flicker. ndmcheck never takes the hit;
ndc_compare does not compare u[0]. Fits the SBKW residual (sbk_56 native vs nomodelnat 10/12 shots differ in the water box).
F1 (all panned 2-tile draws): nd_comb.c:653-654 t1_p[2] = (tile_x0*rs - (td[1].x0 - ox1))/tw1 drops the tile 1 fraction (td[1].x0
drops uls & 3, nd_tex_set.c:528), error fx1 - fx0 up to 0.75 texel, changes every 1-3 frames: the overlay shimmers.
(A1, the 4-bit aux tile source, is WALLT1; not in this row.)

## Plan
1. Check first: mfr=200:12 on tik_19: water geometry shows `h` (hit) on frames with no mfk w18-21 line.
2. B1 fix: NdMEnt tail gets uint16_t ww, wh (nd.h:398; 58 of 64 bytes used; keep the _Static_assert); the store (pm_gpu.c:2935) sets
   them to the wdesc size, or 0 when the handle desc was used; the hit (pm_gpu.c:3124) copies the desc with w/h replaced when ww is set,
   before nd_tex_get. Cold debug counter of hits that used the window. Note for KEYMISS (row_KEYMISS.md M2): keep this invariant.
3. F1 fix in dual_bind: t1_p[2] = ((tile_x0 + tile_fx)*rs - ((td[1].x0 - ox1) + fx1))/tw1 with fx1 = (tile_sz[1][0] & 3)*0.25f, same
   on t. Per bind, float only; draws with no fraction keep the same bytes.
hw: one compare and a 16-byte copy per hit; no loops, no division per vertex; no toggles; no stubs.

## Done criteria
nd_host PASS; memcheck OK. Det picture series shotwin=200:12 on sbk_56 or tik_19 (class B), mac_00 or nok_11, iwa_10; flicker metric on
the water box: for each f, min(d(f-1,f), d(f,f+1)) - d(f-1,f+1) <= 0. B1: sbk_56 native vs nomodelnat byte-equal in x120..153 y87..314
12/12 (was 10/12 differ), tik_19 the same. Byte gates vs build-at base: battle_start_f182_cold equal; spot_mac01_det diffs only on
mac_mirror2 (B1) / mac_flotif (F1); mdl5_iwa10_det only on taki/dropwater; nok11_det only on suimen/kabemizu; each B1 diff region equal
to nomodelnat.

## Rules
CLAUDE.md rules 5, 7, 9, 11. No git stash, no push, no attribution. Never commit 3ds/src/map_restore_table.c. DIRECT/DOORHITCH edit
pm_gpu.c and nd_model.c in parallel: merge main before each fix.

## At the end
Delete this row file in the completing commit. SendMessage main "WATER ready <hash>".
