# Row KMR11W: dark rim lines on every cutout edge (kmr_11 fortress windows, foliage)

## User facts
User (2026-11-09, O3DS XL, build 92e013100): "the windows on goomboss's fortress (KMR_11) have visible border lines on the top and
bottom, perhaps a transparency issue." User direction: if it is a general texture rendering issue, fix it as a whole.

## Start state
Worktree build3ds/wt/KMR11W, branch KMR11W from main. PM_ROW=KMR11W, PM_LANE=KMR11W. One Haiku agent.
KMR10M edits nd_tex_set.c (no code overlap). Both change kmr_10 pixels: the row that merges second rebuilds its base build.

## Cause (oracle, about 80%)
- 3ds/src/nd_tmpl.c:283 `if (m & 0x1000u) o->atest = o->blend ? 0x08 : 0x30;` N64 TEX_EDGE (AA_EN, IM_RD, CVG_X_ALPHA, ALPHA_CVG_SEL,
  blender CLR_IN,A_IN,CLR_MEM,A_MEM) gets blend 0 and alpha test GREATER 0x30 (written at :357, blend ONE/ZERO at :368). Used by the
  walker (nd_set.c:163-166) and native models (nd_model.c:167, :333, :359).
- Bilinear filtering of an alpha-0 texel with a dark key colour gives colour = alpha x neighbour colour. The 3DS draws every fragment
  with alpha > 0.19 at full weight: a near-black rim 1 px wide. The N64 turns alpha into coverage and its AA blender mixes the pixel
  with the frame buffer, so the rim fades.
- Shot evidence (kmr11_cut run 150641, shot 0010): rim pixels 0d0f14..07080b next to frame texels about 535a5d (ratio 0.13-0.2).
  Dark-pixel count (max channel < 20) in box (315,115,385,170): 96 on main; door box (170,100,260,170): 46.
- Scope: every TEX_EDGE draw with bilinear filter and a dark transparent key. 158 of 380 CI map textures with transparency have a dark
  key (Toad Town trees mac_britsh*, iwa rock edges, nok foliage, jan_*, kmr_sakutif). HUD and XLU effects are not affected.
- Ruled out: wrap/clamp (madotif rows 0 and 31 are alpha 0), tile size (model.c:2165 exact), palette alpha conversion (pm_tex.c
  :418-427 exact), texel centre (pm_texf_centre 0: a separate half-texel item).

## Fix steps (nd_tmpl.c, nd.h, the host test)
1. 3ds/include/nd.h: `int nd_edge_aa(uint32_t m);` (rule: AA_EN + IM_RD + CVG_X_ALPHA + ALPHA_CVG_SEL set, FORCE_BL clear, cycle 2
   blender M=CLR_MEM and B=A_MEM, not decal):
   `(m & 0x7048u) == 0x3048u && ((m >> 20) & 3u) == 1u && ((m >> 16) & 3u) == 1u && (m & 0xc00u) != 0xc00u`
2. nd_tmpl.c nd_render_state (about :282-284): keep the o->blend line, then
   `if (nd_edge_aa(m)) { o->blend = 1; o->atest = 0x1F; } else if (m & 0x1000u) o->atest = o->blend ? 0x08 : 0x30;
    else if ((m & 3u) == 1u) o->atest = 0x08;`  Comment KMR11W (N64 coverage = alpha x cvg; alpha < 32 -> coverage 0, dropped;
   the AA blender mixes partial pixels with memory; PICA SRC_ALPHA approximates it). RM_TEX_EDGE (FORCE_BL, no IM_RD) stays 0x30.
3. nd.h:394 ND_OML_READ: add bits 3, 6, 13, 14 (0xF0FF1C33u -> 0xF0FF7C7Bu); update the comment at :391 (nd_model.c:331 checks it).
4. 3ds/tools/nd_tmpl_test.c:71-73: before the `same` test, if nd_edge_aa(om_l) && rs.blend==1 && rs.atest==0x1F && d->tk[14]==0
   && d->atest==0x30, set rs.blend=0, rs.atest=0x30 (held oracle data predates the rule). nd_model_test.c case 1 must still pass.
5. Only if the picture does not change or console frame time gets worse: revert 1-4; in pm_tex.c expand() (CI and RGBA16 branches,
   about :418-430) set the RGB of each alpha-0 texel to the mean of its opaque 4-neighbours (alpha stays 0); same in
   3ds/tools/texbake.c if it mirrors expand. Byte checks on scenes that use it.

## Gates
- nd_host PASS.
- kmr11_cut (until `shot \d+ frame 261`, cap 30 s): shots 0010 and 0011 show the windows and door with no dark line; the dark-pixel
  count in box (315,115,385,170) of shot 0010 drops well below 96 (door box below 46). Tower windows stay whole.
- Against a build-at of the base: spot_mac01_det, mdl5_iwa10_det, nok11_det may differ only on cutout silhouettes (no holes, no colour
  shift inside surfaces). Count diff pixels; open one diff image per scene.
- gameover_fps60 byte-equal.
- memcheck OK (about +0 KB). Console (user): prof push at mac_01 0 0 200, frame time vs 92e013100 (blend costs fill rate).
