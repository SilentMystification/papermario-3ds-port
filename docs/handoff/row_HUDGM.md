# Row HUDGM: the per-task matrix table holds 1024 matrices, not 256 (late HUD triangles dropped)

(Model: Haiku. 60 lines or fewer. The agent reads only this file, CLAUDE.md and what this file names.)

## User facts (bugs only)
User (2026-11-11, O3DS XL, widescreen on; the user says widescreen does not matter): "it's the background circles in the HUD that
don't render"; the battle menu icons and text draw. The circles: HID_Spotlights and HID_HighlightSpot (src/battle/btl_states_menus.c:633,
:646, HES_ProjectorSpot IA8 32x32, alpha 150/180), lit quads of render_complex_hud_element mode 2 (src/hud_element.c:1569-1627).

## Cause (oracle code read; one of two paths that can drop them, the other is row HUDRING)
- The circles are the last triangles of a battle frame (the HUD after the stage, actors and effects). Each vertex load asks
  gv_matrix_id (3ds/src/nd_mtx_set.c:54-77) for a table entry of its combined matrix (lit loads also compare the normal rows, so every
  pushed HUD element is a new entry). The table holds 256 entries per task (:56-57 malloc, :73 `gm_n < 256`); past that it returns
  GV_NONE, nd_geom_vtx gives the load no ring slot (nd_geom.c:323-325) and draw_tri drops every triangle (nd_geom.c:291-293, TRI-4).
  Texture rectangles (icons, text) use no table entry: they still draw. The reset is per task (nd_cen.c:535-536).
- Format readers and writers of the matrix index (all listed): nd_mtx_set.c:45 gm_idx[512], :51 and :67 `& 511u`, :56-57, :73;
  pm_gpu.c:148 pcomp[257]; pm_gpu_int.h:64 pcomp_gen[257], :69 RECT_GM = 256 (the rectangle palette slot after the last matrix);
  vert_gmi is unsigned short (GV_NONE 0xffff, nd_int.h:53); pal_g is unsigned (pm_gpu_int.h:43). nd_cen.c:535 memsets gm_idx by sizeof.
- Memory: gm_tab 64 KB and gm_nrm 48 KB on the heap (malloc at first use), pcomp +48 KB and pcomp_gen +3 KB static, gm_idx +3 KB.

## Start
`sh 3ds/tools/wt_new.sh HUDGM`; PM_ROW=HUDGM PM_LANE=HUDGM on every call.

## Touch only
3ds/include/pm_gpu.h (one new define), 3ds/src/nd_mtx_set.c (:45, :51, :56-57, :67, :73), 3ds/src/pm_gpu.c (:148 only),
3ds/src/pm_gpu_int.h (:64, :69). Other rows: HC6B (pm_gpu.c model hit/submit, nd_emit.c, nd_model.c), UB1 (pm_gpu.c :3750-3796),
WALLSHADE (pm_gpu.c :3869), HUDRING (nd_geom.c). Do not edit their places. `git merge main` before the completing commit.

## Exact edits
1. pm_gpu.h: add `#define PM_GM_N 1024u /* HUDGM: game matrices per task (nd_mtx_set.c gm_tab); the rectangle palette slot is PM_GM_N */`.
2. nd_mtx_set.c: `gm_idx[512]` -> `gm_idx[2u * PM_GM_N]`; both `& 511u` -> `& (2u * PM_GM_N - 1u)`; both `malloc(256 * ...)` ->
   `malloc(PM_GM_N * ...)`; `gm_n < 256` -> `gm_n < PM_GM_N`; the comment "512 slots for at most 256 entries" -> "2 * PM_GM_N slots".
3. pm_gpu.c:148 `pcomp[257]` -> `pcomp[PM_GM_N + 1u]`.
4. pm_gpu_int.h:64 `pcomp_gen[257]` -> `pcomp_gen[PM_GM_N + 1u]`; :69 `RECT_GM = 256` -> `RECT_GM = PM_GM_N`.
5. `grep -n "256\|257\|511\|512" 3ds/src/nd_mtx_set.c 3ds/src/pm_gpu_int.h` and `grep -n "pcomp\[\|RECT_GM" 3ds/src/*.c`: write the
   remaining hits in the commit; an index of gm_tab, pcomp or pcomp_gen still bounded by 256 is a stop (hand back the line).

## Gate (CLAUDE.md 2; the wheel is the user's console check)
- `PM_ROW=HUDGM PM_LANE=HUDGM sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK (write the static delta).
- `python 3ds/tools/bench.py run nd_host --row HUDGM --why "matrix table size"`: PASS.
- Byte compare on two scenes that do not draw the wheel: `bench.py base spot_mac01_det`, `bench.py base mdl5_iwa10_det`, one
  `bench.py run spot_mac01_det,mdl5_iwa10_det --row HUDGM --why "gm table 1024: no change under 256 entries"`, `cmp` each: 6/6.

## Stop
A failed gate, an edit that does not fit the code, or a test-tool fault: commit, then hand back the exact output (CLAUDE.md 4-5).

## At the end
Delete this row file in the commit that completes the row. Hand back: the ready hash, the gate results, the memcheck line.
