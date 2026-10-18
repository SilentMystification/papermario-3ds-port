# Render gaps: render-to-image and read-back cases

Read-only study, 2026-10-18. Method: grep and code reading. No build. No Azahar run.
Status words: works = a slice verified it. approximated = runs, result differs from N64. broken = wrong or empty result. unverified = by code reading only.
X02 is in the working tree (uncommitted). No log line says it is tested.

Answer: X02 is not the only case. There are 13 cases. 5 are handled. 2 have a row (X02, F03). 6 have no row.

## A. Colour image set to something other than the main frame buffer

| # | Case (decomp site) | Used by | Port now | Row | Risk |
|---|---|---|---|---|---|
| 1 | Rotating window drawn into a game buffer (draw_box.c:771, 784) | pause page flip, file select flip, battle menu windows | works (off-screen target, V05) | V05 done | low |
| 2 | Shading palette, 16x1 image `SpriteShadingPalette` (sprite_shading.c:389, 403) | every shaded sprite | works (software texrect, S15) | S15 done | low |
| 3 | Darkness stencil in z-buffer memory: 32 bit then 8 bit, 64x64 (screen_render_util.c:152, 189, 207) | kgr_01, kgr_02, omo_12, omo_14, kzn_08, jan_11, kpa_01, kpa_03 (files that name darkness), battle toy_tank and big_lantern_ghost, Watt light | broken on main, fix in the working tree (not committed, not tested) | X02 | medium: shared 150 KB z-buffer image, 2 slots on one address, TMP `tmpnofill` hook still in code |
| 4 | Depth copy: colour image `DepthCopyBuffer`, source tile from z-buffer (model.c:4399, 4505) | `bulb_glow` (hos_05 lamp, Watt battle, lightning effect). `motion_blur_flame` passes id -1 | broken: reads NULL z-buffer, depth 0, glow always shown | F03 (design in docs/plans/F03_F08.md) | low |
| 5 | Z-buffer clear fill with colour image = z-buffer (background_gfx.c:398, battle/16C8E0.c:847, hud_element.c:1781) | every frame, world, battle, HUD camera | approximated: the port draws the fill on the screen. Fill word 0xFFFC has alpha bit 0, so alpha is 0 and nothing shows. Unverified. | none | low: breaks if a fill value with bit 0 set is used |
| 6 | Model drawn into the z-buffer as a colour image with Z_CMP, then the frame buffer is decal-copied with prim depth 32734 (flo_10/waterfx.c:232, mac_01/crystal_ball_gfx.c:89) | flo_10 water reflection, mac_01 crystal ball (mirror ball) | broken or empty: there is no depth image to test against. The model draws on the screen in prim colour (alpha 0 in the combiner, so it may be invisible). Unverified. | none | low to medium: 2 maps, visible effect missing or wrong |
| 7 | `underwater` effect: frame buffer copied into the z-buffer, warped mesh drawn from it (underwater.c:167, 187, 283, 285). It also uses `gSPModifyVertex` (G_MODIFYVTX) | Sushie battle (partner/sushie.c:336), script call `fx_underwater` in fx_api.c | broken: G_MODIFYVTX is skipped (not in the opcode switch), texture source is stale memory | none | low: one effect |
| 8 | `flame` effect scratch images: 32x64 I8 in `D_800B3AE0`, 2-cycle combiner, chroma key (effects/gfx/flame.c:38, effects/flame.c:187) | 7 call sites, all isk battle stages (isk_04 to isk_08) | approximated at best: X02 sends I8 images off-screen, but the chroma key (G_SETKEYR, G_SETKEYGB) is not handled. Not seen. | none | low |
| 9 | Soft reset tile pattern: colour image = `ResetSavedFrameImg` (main.c:159) | reset only (no 3DS path found) | unverified, likely never reached | none | low |
| 10 | Pause coverage: colour image `nuGfxCfb[1]` with VISCVG fill (background_gfx.c:328) | pause open | works (D03: port writes full coverage) | D03 done | low |

Not cases: 14 other SETCIMG sites set `nuGfxCfb_ptr` back (cam_main x2, curtains, windows, title screen, hos_05 intro, background_gfx, flame.c, underwater end, model.c end, hud_element, 16C8E0 end). They are the main frame buffer. `func_80027BAC` (background_gfx.c:280) has no caller.

## B. Frame buffer or z-buffer read as data or texture

| # | Case | Used by | Port now | Row | Risk |
|---|---|---|---|---|---|
| 11 | Last-frame read (`draw_prev_frame_buffer`, screen_render_util.c:30): the port copies the last PICA frame into the previous frame buffer on SETTIMG (`pm_cfb_capture_prev`) | battle start swirl (OVERLAY_START_BATTLE, encounter.c, f8f60), OVERLAY_BLUR (Kammy, mega_quake), dizzy_dial, firework_rocket | works (X01, D03) | X01 done | medium for speed: a read back of 150 KB per frame while active. Not measured on hardware. |
| 12 | Pause blur: CPU filter reads `nuGfxCfb[0]`, writes `nuGfxZBuffer` (background_gfx.c:342) | pause | works, approximated (full coverage) | D03 done | low |
| 13 | Current-frame grab: the game loads `nuGfxCfb_ptr` (the frame being drawn) as a texture mid-frame (effects/water_splash.c:206, motion_blur_flame.c:248, underwater.c:173, bulb_glow.c:247, effect_utils.c:77 and 93, flo_10/waterfx.c:63, mac_01 crystal ball :110, kgr_02/unused.c:55) | water splash (about 10 battle call sites), Super Block orbs, flo_10 wavy water, mac_01 mirror ball, Sushie underwater | broken by code reading: SETTIMG on `nuGfxCfb_ptr` fills the OTHER buffer, the grab reads the stale buffer. No frame readback of the picture drawn so far. | none | medium: visible wrong or empty texture. Fix needs a mid-frame PICA readback (flush plus read, costly) or a render-to-texture copy. |

## C. RDP features the port approximates

| Feature | Count in src | Port now | Row |
|---|---|---|---|
| COPY cycle (background.c:236, background_gfx.c:287 (unused), 16C8E0.c:771, state_logos.c:41) | 4 sites | works (dsdx / 4, lower right inclusive) | none needed |
| TEXRECTFLIP | 0 uses | opcode handled, not needed | none |
| TLUT in TMEM, palette lookup (shading palette) | 1 site | works, software path (S15). CI textures decode in pm_tex. | S15 done |
| gDPLoadMultiTile (two tiles, TMEM halves) | 13 lines, 7 files | works for ROM or memory images (`dual_bind`). Breaks for a frame buffer source (case 13). | none |
| G_LOADBLOCK (gDPLoadTextureBlock 16 lines, gDPLoadBlock 1 line, PAL language select only) | 17 | opcode ignored. Texture comes from SETTIMG and SETTILE. Works in golden frames. | none |
| Chroma key (G_SETKEYR, G_SETKEYGB) | 2 files (flame.c, ring_blast.c) | not handled | none, risk low |
| G_MODIFYVTX | 1 file (underwater.c, 4 lines) | skipped | none (case 7) |
| Per-vertex fog factor, fog table | 24 files, 118 lines | blend and fog colour mix only | F08 |
| Alpha dither and noise, colour dither | 12 and 21 files | ignored (no PICA equivalent), cosmetic | none needed |
| VISCVG coverage fill | 1 site | works (D03) | D03 done |
| Prim depth fill (G_ZS_PRIM) | transitions, case 6 | works for fills (X01), not for the case 6 decal | X01 done |

## Summary

- Cases: 13 (A 10, B 3). Section C lists features, not cases.
- Handled: 1, 2, 10, 11, 12 (plus COPY, palette, VISCVG, prim depth).
- Has a row: 3 (X02, uncommitted), 4 (F03), fog (F08).
- Open with no row: 5 (verify), 6 (flo_10 and mac_01 z-buffer mask), 7 (underwater, G_MODIFYVTX), 8 (flame, chroma key), 9 (soft reset), 13 (current-frame grab). The ring_blast chroma key has no row either.
- Biggest risk: case 13. Many effects read the frame being drawn. The port gives them a stale buffer. A fix needs a PICA readback or a render-to-texture copy during the frame.
- Next step: add one row for case 13 (with 6 and 7 as sub-cases). Check case 5 with `xtrace` in one frame.
