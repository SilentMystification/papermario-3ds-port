# Row HUDGM: the per-task matrix table holds 1024 matrices, not 256 (late HUD triangles dropped)

(Model: Haiku. 60 lines or fewer. The agent reads only this file, CLAUDE.md and what this file names.)

## User facts (bugs only)
User (2026-11-11, O3DS XL; widescreen does not matter): "it's the background circles in the HUD that don't render"; the battle menu
icons and text draw. The circles: HID_Spotlights, HID_HighlightSpot (src/battle/btl_states_menus.c:633, :646), lit quads of
render_complex_hud_element mode 2 (src/hud_element.c:1569-1627).

## Cause (oracle code read; one of two paths, the other is row HUDRING)
The HUD draws last in a battle frame. Each vertex load asks gv_matrix_id (3ds/src/nd_mtx_set.c:54-77) for a table entry; the table holds
256 per task (:73); past it GV_NONE gives the load no ring slot (nd_geom.c:323-325) and every triangle drops (nd_geom.c:291-293).

## State
Branch HUDGM, commit 22a7fa890 has edits 1-4 below; the build failed (nd_int.h:223 conflicting type). The coder continues from it.

## Full audit (oracle, grep of 3ds/src, 3ds/include, src, include for gm_idx, gm_tab, gm_nrm, gm_n, gm_last, pcomp, pcomp_gen,
## RECT_GM, PM_GM_N, vert_gmi, pal_g, 256/255/511/512 and narrow fields): every reader and writer
- Header declarations: 3ds/include/nd_int.h:220 (gm_last, gm_n: unsigned, no size), :221-222 (pointers, no size), :223
  `extern unsigned short gm_idx[512];` (sized: MUST change), :152 vert_gmi unsigned short (holds 0..1023 and GV_NONE 0xffff: ok).
- Writers/readers with a size: nd_mtx_set.c:45, :51, :56-57, :67, :73 (edit 2); pm_gpu.c:148 pcomp (edit 3); pm_gpu_int.h:64
  pcomp_gen, :69 RECT_GM (edit 4); nd_cen.c:535 `memset(gm_idx, 0, sizeof gm_idx)` (needs the size after edit 5).
- Index users with no size (no edit): pm_gpu.c:473-483, :811-819 (pcomp_calc), :1192-1195 (pal_ntab[gm] = gm_nrm), :1363-1365,
  :2638, :2687, :2844, :2910, :2950, :2963; pm_gpu_int.h:43 pal_g unsigned; pm_gpu_rect.c:87, :108, :115 (RECT_GM); nd_geom.c:27,
  :119, :233-238, :346; nd_shape.c:207, :374; nd_ent.c:292; nd_cen.c:442, :536.
- Not this index (no edit): nd_mtx_set.c:80-81 mtx_cache[256]/mtx_tag[256] (model matrix slots, header w1 bits 0-7); pm_gpu.c:834
  rd_uf[256]; the ring vertex palette slot (pm_gpu.c gtrip_copy `k`, rk[] uint8: a slot below GPAL 21, not a matrix index).
- No 8- or 9-bit field stores a matrix index (no bitfield, no uint8_t gm in nd.h, nd_int.h, pm_gpu_int.h, nd_model.c, nd_shape.c,
  nd_ent.c; NdNatC holds no matrix index: its palette is built at each hit).

## Exact edits (1-4 are in 22a7fa890; 5-7 are new)
1. pm_gpu.h: `#define PM_GM_N 1024u`.
2. nd_mtx_set.c: gm_idx[2u * PM_GM_N]; `& 511u` (two) -> `& (2u * PM_GM_N - 1u)`; malloc(PM_GM_N * ...) (two); `gm_n < PM_GM_N`.
3. pm_gpu.c:148 `pcomp[PM_GM_N + 1u]`.  4. pm_gpu_int.h:64 `pcomp_gen[PM_GM_N + 1u]`, :69 `RECT_GM = PM_GM_N`.
5. 3ds/include/nd_int.h:223: `extern unsigned short gm_idx[512];` -> `extern unsigned short gm_idx[]; /* HUDGM: 2 * PM_GM_N entries (pm_gpu.h, nd_mtx_set.c:45) */`
   (nd_int.h is included before pm_gpu.h in nd_geom.c:10-11 and nd_cen.c:10-11, so the header cannot use PM_GM_N).
6. 3ds/src/nd_cen.c:535: `memset(gm_idx, 0, sizeof gm_idx)` -> `memset(gm_idx, 0, 2u * PM_GM_N * sizeof gm_idx[0])`.
7. nd_mtx_set.c comments :33 "256" -> "PM_GM_N", :44 "512 slots for at most 256 entries" -> "2 * PM_GM_N slots for PM_GM_N entries".

## Touch only
3ds/include/pm_gpu.h, 3ds/include/nd_int.h (:223), 3ds/src/nd_mtx_set.c, 3ds/src/pm_gpu.c (:148), 3ds/src/pm_gpu_int.h (:64, :69),
3ds/src/nd_cen.c (:535). Other rows: HC6B (pm_gpu.c model hit/submit, nd_emit.c, nd_model.c), UB1, WALLSHADE, HUDRING (nd_geom.c),
HCCUT2: not their places. `git merge main` before the completing commit.

## Gate (CLAUDE.md 2; the wheel is the user's console check)
- `PM_ROW=HUDGM PM_LANE=HUDGM sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: rc 0, memcheck OK (write the static delta).
- `python 3ds/tools/bench.py run nd_host --row HUDGM --why "matrix table size"`: PASS.
- `bench.py base` for mdl5_iwa10_det and spot_mac01_det, one `run mdl5_iwa10_det,spot_mac01_det` (the second is warm; cmp refuses a
  cold row run), `cmp` each: 6/6.

## Stop
A failed gate, an edit that does not fit the code, or a test-tool fault: commit, then hand back the exact output (CLAUDE.md 4-5).

## At the end
Delete this row file in the commit that completes the row. Hand back: the ready hash, the gate results, the memcheck line.
