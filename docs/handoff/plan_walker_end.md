# Plan WEND: every walker GPU draw goes native (oracle, 2026-11-06)

Walker work = a walker GPU draw: wd in the WCEN3 census (cen_add in pm_gpu.c), any draw that does not close through nd_native_submit. Model records, sprite records, entity segments and ent_gdl draw natively already (interpretation cost only).

## Census at HEAD (CEN-1a, 951ff79e5; table in agent_wcen.md; replaces the wd numbers below where they differ)
Walker wd per 300 frames is CPU rect in every scene: mac_01 607, iwa_10 593, battle kmr_03 2401 (effect_renderUI 1736), sbk_56 1487 (rect 611, clip 876 = sun_appendGfx), pause 40482 (rect 40272, nr8 210, draw_msg glyphs), text 40746 (rect 40536, nr8 210), golden 600, nok_11 605. Not seen: wide, ring, seam, pm5, nr1 to nr7, nr9, other. entity_model, appendGfx_model, sprite_component, walking_dust, stars_shimmer: 0 wd (WorkerBackUI 2136 rect in one mac_01 window). So: RECT is the main stream. TRI-2 runs only if a wide-mode census (battle_wide, pause_spin_wide) shows wide wd. TRI-3 = sun_appendGfx clip at sbk_56. OFF = nr8 (unit 0) at text menus first (after STALE2); OFF-1/2 only if pause_spin_grab, battle_start_fb show nr2/nr3. MDL-1/2 (R13, R11 forms) not yet listed: one ndmcheck run first; MDL-3 only if spr_fb shows wd.

## Inventory (wd per 300 frames, mac_01 / iwa_10, agent_wcen.md WCEN3)
1. camera_render (background pre-render) 289 / 283: CPU textured rect, `pm_gpu_tex_rect(` in pm_gbi.c. Stream RECT.
2. background lists 015fe3d8, 0155a1a8: 145+144 / 142+141: same. Stream RECT.
3. gfx_game_mode_front_ui, WorkerFrontUI: 21 / 11: CPU textured and fill rects. Stream RECT.
4. walking_dust, stars_shimmer, sun, effect_renderUI: 126 / 49: frame-heap VTX+TRI, cause not named (`draw_tri_cpu(`, `gv_tri(`, nraw_why). Stream RECT-2.
5. WorkerBackUI: 96 / 2: not named. CEN-1a, then RECT-2.
6. appendGfx_entity_model: 1 / 630: wide ortho UI quads with vertices on the game edge (WIDE1 fallback; `wide_edge_k(`). Stream TRI-2.
7. appendGfx_model: 1 / 198: nd_model why 4 (lit/fog pm5), 2 (R13), 3 (R11), 6 (tex1/noise), 15 (zmark+decal), 11/12. Streams PM5, MDL, TRI-1.
8. sprite_component: 0 / 171: spr_fb (imgfx meshes, shaded sprite two tiles). Stream MDL-3.
9. off-screen and grab producers: not censused; nraw_why 2, 3, 8 (`grep -rln gDPSetColorImage src`). Stream OFF.
10. zmark, nozt, noise: nraw_why 4, 5, 7. Streams TRI-1, OFF-3.
11. CPU triangles (clip, seam): draw_tri_cpu, pm_gpu_tri3d, pm_gpu_vert_clip (`seam_obj++`, `seam_clip++`). Stream TRI-3.
12. pause, file menu, title, text box, battle: wd not measured. CEN-1a.

## Decisions (Haiku does not change these)
- D1 (user, 2026-11-06: "Commit to S10 now") Goal is S10: pm_gbi.c deleted. WEND (wd 0, CPU paths deleted) runs in parallel with S5, S7, S8, S9 (producers write native records, no N64 words), then S10-1 to S10-4. No wait for a console number.
- D2 Why codes, fixed: rect_cpu, tri_cpu_clip, tri_cpu_seam, wide_edge, ring_miss, nraw1 cmdbuf, nraw2 off, nraw3 grab, nraw4 zmark, nraw5 nozt, nraw6 tex1, nraw7 noise, nraw8 unit0, nraw9 tmpl, pm5, other (with the op). No new codes.
- D3 One refusal line per reason in nd_raw_native (CEN-1b). Each stream edits only its own line.
- D4 2D rects: palette slot RECT_GM 256, depth folded as in tri_raw. No new palette slots.
- D5 Off-screen: the target switch stays in citro3d (off_begin, FrameDrawOn). Native draws change only palette, viewport, scissor.
- D6 When the last citro3d draw is gone (TRI-4), delete nd_leave_native and every citro3d draw-state shadow in the same row.
- D7 pm5 reuses the prog5 register layout. No new shader.
- D8 Debug switches allowed in diagnosis rows. The stream's deletion row removes them. A remaining case becomes a named fallback with a counter, never a switch.
- D9 mac_01 byte-equal only when shared code changes (nd_native_submit, nd_emit, nd_tmpl depth/TEV, pcomp_calc, palette). Else right by eye at the named scene.
- D10 Two tries per named cause, then hand back with the measurement.

## Streams and ownership
| id | owns | waits for |
|---|---|---|
| CEN | pm_gpu.c cen_add, cen_frame_end, the refusal line of nd_raw_native; pm_gbi.c os_report | nothing (first) |
| RECT | pm_gpu.c pm_gpu_fill_rect, pm_gpu_tex_rect(_z), pm_gpu_fill_depth, rect_begin, rect_raw_*, flush_impl_body rect branch, pcomp_calc, pm_gpu_gtrip, pm_gpu_gappend, pm_gpu_gvtx_load | running |
| TRI | pm_gbi.c draw_tri, gv_tri, draw_tri_cpu, off_screen_check; pm_gpu.c pm_gpu_tri3d, pm_gpu_vert_clip, wide_edge_k, pm_gpu_set_zmark, zmark/nozt refusal lines | CEN-1b for TRI-1 |
| PM5 | nd_tmpl.c program/attr/uniform build (not tev_op_*); nd_emit.c; pm_gpu.c graw_is_pm4, g_use5 block of flush_draw; nd_model.c why 4 line | FX (super block), RECT-1 for PM5-2 |
| OFF | pm_gpu.c pm_gpu_off_begin/off_end, grab_impl, off_snapshot, off/grab/noise refusal lines; pm_tex.c pm_tex_native_unit0 off-screen branch | CEN-1b; STALE2 for OFF-3 |
| MDL | nd_model.c (not the why 4 line), pm_gpu.c pm_gpu_model_native; pm_gbi.c rec_run, load_shaded_sprite; nd_sprite.c | FX, CEN-1a forms |
Running elsewhere: FX (effects, nd_tmpl.c tev_op_*, pm_comb.c), STALE2 (pm_tex.c, nd_tex.c), LOG (pm_log.c).
Merge order: CEN-1b before OFF, TRI-1, PM5. RECT-1 before PM5-2. FX before PM5-1 and MDL-1. STALE2 before OFF-3. Deletion rows last: RECT-3, TRI-4, OFF-4, PM5-4, then END.
Every row: commit through the agent's own temporary index, own hunks only, a status line before each Azahar start.

## Rows
CEN-1a WHY: a why code (D2) on every wd in cen_add; one line per producer `wcen4 <prod> wd <n> why <code>:<n>...`. Scenes: wcen_mac01, wcen_iwa10, wcen_battle, wcen_sbk56, pause_menu, text_menu, golden_multi (title), nok11_splash. Table wd x why per scene into agent_wcen.md (replace the old table). Gate: per scene sum of why = wd; no picture change. Also list the R13 aux forms (nd_model why 2) and R11 cases (why 3) seen.
CEN-1b SPLIT: the one refusal line of nd_raw_native becomes one line per reason, same order, same counters. Gate: mac_01 byte-equal, nraw_why counts unchanged.
CEN-2 RECENSUS: after each stream's deletion row, rerun the 8 scenes. Gate: wd for that stream's codes 0.
RECT-2a NAME: for walking_dust, stars_shimmer, sun, WorkerBackUI read the CEN-1a why. RECT-2b FIX, one row per cause (30 to 100 lines): vertex not in the ring (vert_ring GV_NONE): load into the ring in G_VTX; matrix not in the palette: add the entry; batch cut by state: use the merge key. Gate: wd 0 for these producers at mac_01, iwa_10, sbk56; dust, stars, sun right by eye. RECT-3 DELETE: CPU branches of pm_gpu_fill_rect and pm_gpu_tex_rect, rect_end, rect dbg_*. Gate: builds, mac_01 byte-equal to the RECT-2 build.
PM5-1 TEMPLATE: NdTmplKey gets use5; nd_tmpl.c builds prog5, attr4 and the pm5 uniform block (lflag, fogu, light table, 7 registers per palette entry), same layout as the citro3d path. Host test first: nat_host_test emitted words vs an ndref capture of one pm5 batch. PM5-2 RAW: graw_is_pm4 accepts g_use5 for the native close; fog map and lit map from the CEN-1a why 4 table; gate why 4 wd 0, shots by eye, mac_01 byte-equal. PM5-3 MODEL: nd_model_state drops why 4 (gv->fxon to the pm5 template); gate ndm why 4 = 0, by eye, mac_01 byte-equal. PM5-4 DELETE: g_use5 citro3d block of flush_draw, sh5_*, u5_* without callers.
OFF-1 TARGET: nd_raw_native accepts off_active: palette uses off_proj, viewport/scissor of the target (D5). Scenes pause_spin_grab, battle_start_fb. Gate why 2 = 0, shots by eye. OFF-2 GRAB: accept grab_bind, unit 0 words from the grab slot; gate why 3 = 0 at pause_spin_grab and the motion_blur_flame part of sbk56_superblock. OFF-3 UNIT (after STALE2): pm_tex_native_unit0 returns the off-screen image slot with the snapshot rule (a batch that samples the image it writes uses a snapshot); noise unit 2 native (nraw_why 7, ndm why 6 noise); gate why 8 and 7 = 0 at intro_native, nok11_splash. OFF-4 DELETE: off-screen CPU close branches in flush_draw, noise_bound.
TRI-1 ZMARK (after CEN-1b): nd_raw_native draws r_zmark in two passes (template pass 1 then pass 3, as nd_model_draw); nd_model why 15 in the same row. Gate why 4 = 0 where censused, by eye, mac_01 byte-equal. TRI-2 WIDE: partial-edge ortho draws: the vertex copy moves edge vertices to the screen edge (exactly the CPU rule) before the ring write, in wide_edge_k's callers. Gate entity_model wd at iwa_10 630 to about 0 (interp 0); wide HUD by eye at battle_wide, pause_spin_wide. TRI-3 CLIP: name seam_obj/seam_clip draws (CEN-1a); GPU clipping (w > 0 user clip plane, as sh_user_clip sets) instead of CPU vert_clip. Gate seam counters 0, V18 intro pan by eye. TRI-4 DELETE: draw_tri_cpu, pm_gpu_tri3d, pm_gpu_vert_clip, tri_raw, put4, flush_draw_cpu, the Vert batch (vert_base), nd_leave_native/raw1_reset if no citro3d draw is left (D6).
MDL-1 R13: decode the R13 aux words (why 2) as aux_ok forms, one form per row (forms from CEN-1a). Gate ndm why 2 = 0 at iwa_10, mac_01 byte-equal. MDL-2 R11: custom gfx pre/post (why 3) runs on the state side, then draws natively; gate why 3 = 0 where censused. MDL-3 SPRITE: shaded sprite as the two-unit native form (load_shaded_sprite to pm_tex_native_unit1); imgfx meshes through the ring. Gate sprite_component wd 0 at iwa_10 and battle_start, spr_fb 0. MDL-4 DELETE: pm_gpu_model_fb_cached and the walker geometry fallback in rec_run (`if (!nat) rec_dl`) only if fb is 0 across the CEN scenes; else the remaining cases become named fallbacks in agent_s6.md.

## End state
wd 0 in every CEN scene. Every GPU draw goes through nd_native_submit. Deleted: the CPU vertex batch, citro3d draw calls (C3D_DrawArrays, C3D_DrawElements), sh_* draw-state shadows, tev_same, nd_leave_native, draw_tri_cpu, CPU clipping. Citro3d keeps frame begin/split/end, render targets, clear, display transfer.
END-1 OPS: count hits per opcode over the full catalog once (epic merge rule); delete each opcode handler with 0 hits and no src macro caller (likely G_TEXRECTFLIP, G_MODIFYVTX, line/quad forms, dd_* drawlog, verts[] dd_on). Gate full catalog G2. Debug tools that stay: ndref capture/compare, the census (os_report, cen_add).

## Risks
Several agents edit pm_gpu.c and pm_gbi.c at once: own index, hunk staging, merge order. Census attribution goes to the op under the cursor at flush, so the why code matters more than the producer name. Azahar is not the console: one console push per stream end. RECT-1 may touch nd_native_submit: PM5-2 waits. The 400-vs-320 width and wide-edge rules live in 3 places (CPU rect, wide_edge_k, bg_texrect copies): TRI-2 uses the CPU rule exactly.
