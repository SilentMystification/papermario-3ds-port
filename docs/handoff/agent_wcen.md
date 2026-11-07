# agent_wcen: walker draw census by why code (CEN-1a)

## STATE
CEN-1a done: c53711ba2 (why codes, wcen4 lines), 7c9c078a7 (scenario fixes). 8 scenes censused, lane cen, 300-frame window from boot. Every line: sum of why counts = wd.
Walker wd left at the 8 scenes: CPU rect (all but sun clip at sbk_56 and nr8 unit 0 at the pause and text menus).
CEN-1b done: 10528125d (one refusal line per reason). spot_mac01_pic shots 0 to 2 byte-equal to the CEN-1a build, nrawwhy unchanged.
Open: R13 aux forms and R11 cases (nd_model why 2, 3) not censused (ndm_why prints only under ndmcheck). Forms not decoded.
Budget: CEN-1a 8 Azahar starts (cap). CEN-1b 6 starts, 4 of them GPU memory fault 0xC000001D on a slot another agent kept (env).

## Method
- Walker draw (wd): every C3D_Draw of the walker (pm_gpu.c flush_draw_cpu, the raw walker draw). Counted per producer in pm_gbi.c pm_gbi_draw_note, window 300 frames (phase mode, os_report).
- Why code (pm_gpu.c CW_*, set per batch in flush_draw; cen_fb marks CPU triangles from pm_gpu_vert_clip (clip), pm_gpu_tri3d (seam), gtrip wide refusal (wide), draw_tri CPU (ring)):
  rect = CPU rectangle batch; clip = CPU triangle from the clip route; seam = CPU triangle from the seam route; wide = edge-only ortho refusal; ring = CPU triangle the ring did not take; pm5 = pm5 batch (g_use5) on the walker; other = none of these; nr1..nr9 = nd_raw_native refusal (nraw_why index 1..9: 1 cmdbuf, 2 off, 3 grab, 4 zmark, 5 nozt, 6 tex1, 7 noise, 8 unit0, 9 tmpl).
- Lines: `wcen4 f<n> <producer> wd <n> why <code>:<n>...` (one per producer with wd > 0), end line `wcen4 end f<n> wd <n> nd <n>`. Producer = label or the appendGfx address (non-main list: display context list address). Window totals.
- Scenes (lane cen, build of c53711ba2): wcen_mac01, wcen_iwa10, wcen_battle, wcen_sbk56, wcen_pause, wcen_text, wcen_golden, wcen_nok11 (azt_scenarios.json, bench_defs.json).
- Azahar numbers: direction only. Window labels in the table are the printed frame (f324 = first window, f908 = golden tour).

## Table: wd by why code per scene (window totals; nd = native draws in the same window)

| scene | wd | why codes | producers (wd) | nd |
|---|---|---|---|---|
| mac_01 (pos 0 0 200) | 607 | rect 607 | list 0160e3d8 145, list 01258c58 10, list 0156a1a8 144, gfx_game_mode_front_ui 19, camera_render 289 | 24857 |
| iwa_10 (-586 1 530) | 593 | rect 593 | list 0160e3d8 141, list 01258c58 16, list 0156a1a8 142, gfx_game_mode_front_ui 11, camera_render 283 | 37954 |
| battle (kmr_03, battle from frame 100) | 2401 | rect 2401 | list 0160e3d8 149, list 01258c58 14, list 0156a1a8 140, gfx_game_mode_front_ui 81, effect_renderUI 1736, camera_render 281 | 16258 |
| sbk_56 (20 0 440) | 1487 | rect 611, clip 876 | list 0160e3d8 147, list 01258c58 6, list 0156a1a8 146, gfx_game_mode_front_ui 19, camera_render 293, 001e4170 (sun_appendGfx, src/effects/sun.c) clip 876 | 22380 |
| pause (mac_00 4, START at 96) | 40482 | rect 40272, nr8 210 | list 0160e3d8 9346, draw_msg 19738 (rect 19528, nr8 210), gfx_window_root 1767, list 01258c58 12, list 0156a1a8 9101, gfx_game_mode_front_ui 12, camera_render 506 | 5754 |
| text (as pause, to frame 300) | 40746 | rect 40536, nr8 210 | list 0160e3d8 9261, draw_msg 19806 (rect 19606, nr8 200), gfx_window_root 1803 (rect 1793, nr8 10), list 01258c58 12, list 0156a1a8 9346, gfx_game_mode_front_ui 10, camera_render 508 | 5556 |
| golden tour (kmr_20 4 first, window f908) | 600 | rect 600 | list 0160e3d8 148, list 0156a1a8 146, gfx_game_mode_front_ui 12, camera_render 294 | 23693 |
| nok_11 field (entry 0, no splash in window) | 605 | rect 605 | list 0160e3d8 144, list 01258c58 12, list 0156a1a8 143, gfx_game_mode_front_ui 19, camera_render 287 | 19026 |

Also seen: wcen_mac01 second window (f624): WorkerBackUI wd 2136 rect (not in the table, first window only).
nrawwhy (cumulative, prof line): unit0 210 at pause and text, 56 at golden (f300 line), others 0. Matches the nr8 counts of the draw_msg lines.

## Findings
1. The walker is now CPU rects in every censused scene: rect is the RECT stream (pm_gpu_fill_rect, pm_gpu_tex_rect; camera_render and the two background lists are rects).
2. Text menus (pause, text) are the big walker scenes: about 40k wd per 300 frames, nearly all rect (draw_msg glyphs, the menu panels). The nr8 part is unit 0 refused by pm_tex_native_unit0 (offscreen image or debug override), 200 to 210 per window at draw_msg.
3. sbk_56: 876 wd clip draws from sun_appendGfx (TRI-3 CLIP, the CPU clip route of pm_gpu_vert_clip). Sun is an S8 effect.
4. Not seen in any of the 8 scenes: wide, ring, seam, pm5, nr1 to nr7, nr9, other. The old WCEN3 entity_model (iwa_10 928 wd), appendGfx_model (1790 wd), sprite_component (171 wd) and walking_dust, stars_shimmer draws are native at HEAD 7515ccc42 (no walker wd). The old WCEN3 tables are removed (stale).
5. walking_dust, stars_shimmer, WorkerBackUI (plan RECT-2): not walker in these scenes at this HEAD except WorkerBackUI in the mac_01 second window (2136 rect). The RECT-2 cause list must be checked again after CEN-2.

## Gaps
- R13 aux forms (nd_model why 2) and R11 cases (why 3): not in the census. ndm_why prints under the ndmcheck switch only. Forms are not decoded. Needs a run with ndmcheck and a form dump (MDL-1 owner).
- Two of 8 census windows are the first 300 frames from boot (field, pause open from frame 96). Not a 300-frame window inside the pause alone.
- Azahar budget: CEN-1a 8 starts (mac_01 1, list of 7), at cap. No more CEN-1a runs.
