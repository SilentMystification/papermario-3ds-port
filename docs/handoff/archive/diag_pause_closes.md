# Diag: pause and file select closes (build 1301-ui, read-only)

Data: 3ds/logs/perf/ui_menu/tables.txt, steady_split.txt, docs/handoff/diag_menu_lag2.md. Raw log is not in the repo: no per-site counts, no cc/geo/tri/oth split.

## Numbers
- Pause steady (f105, run C): closes 217 (= 217 gpu draws). Classed: tex 213 (151 real, 62 empty), rect 205 (18 real, 187 empty). Real classed 169 of 217; 48 unclassed.
- Pause real close time (us counters): tex 3564 us / 151 = 23.6 us; rect 406 us / 18 = 22.6 us. Timed classes 3.97 ms of GBI 16.1 ms.
- File select steady (f3650): closes 188. tex 141 (111 real, 30 empty), 2602 us. rect 176 (39 real, 137 empty), 1232 us.
- Empty closes per steady frame: pause 249, file select 167. Empty closes have no timer (pm_gpu.c:2158).
- The 97 us premise fails: 217 x 97 us = 21.0 ms, above GBI total 16.1 ms. GBI per close = 74 us (all GBI / closes).
- Marginal, f73-89 to f100-113: +145 closes, GBI +4.3 ms, ops -2145. Not separable from the op drop.

## (1) Cause class and call sites
Class = the op running when the flush runs (pm_gbi.c:25-34, pm_gbi_flush_class).
- tex (class 0): pm_gbi.c:2329 in ensure_tile (only if tile_bound fails, :2313-2318: src, pal, fmt, siz, wrap, size); :3067 G_SETTILE tile 0 (wrap change); :3116 LOADTLUT palette change; :3222 SETTILESIZE pattern path; :1164 dual_bind (aux tile).
- rect (class 3): texrect op (pm_gbi.c ~2949-3036) calls sync_render, ensure_tile(rtile), dual_bind; :2407 box_pattern_rect.
- Texture changes in pause: each label = gSPDisplayList(PauseGfxLabels[i]) + pause_draw_rect (src/pause/pause_gfx.c:644-661). Each message glyph = LoadTextureBlock_4b + rect (src/msg_draw.c:1754-1797). draw_box texture loads at src/draw_box.c:507-516, 812.
- Rotscale windows render to off-screen slots: off_screen_check (pm_gbi.c:220-227). SETCIMG flushes would be class 5 (oth), which is not in the data.

## (2) Closes that change no state
- Repeat bind of the same source is already skipped by tile_bound (pm_gbi.c:2313). Those closes do not exist in tex.
- Count per site: not measured. Use the gbiprof switch (pm_gpu.c:298-325: top 6 sites x100 per frame, and fl_consec = rects with no other flush before them). Not run.

## (3) Partner tab (src/pause/pause_partners.c)
- Per frame in pause_partners_draw_contents (:312): viewport, matrices, one sprite per partner (spr_draw_npc_sprite), portrait loop: 2 banks x 9 rows = 18 gDPLoadTextureTile + pause_draw_rect + PipeSync (:418-438), 2 TLUT loads, 2 DL calls, draw_box 166x121 (:455).
- Close count: not measured. Source gives at least 36 texture-state changes per frame.
- Reach: pause opens with START. Tab column changes while R or stick right is held (src/pause/pause_tabs.c:417-424, one column per game frame). A selects the tab (:440-444).
- Padscript (pm_input.c:33): `padscript=START,wait60,R*2,wait30,R*2,wait30,R*2,wait30,A*2,wait60`. R*2 = 2 scans = 1 game frame. Verify column index of partners in gPauseTabsPanelIDs.
- Partner init runs at pause open (pause_main.c:645). The "partner menu open" line does not prove the page shows. A tab-entry ui line (debug only) is needed for the test.

## (4) Ranked reductions (ms = closes x 0.097 ms, the user figure; empties are not timed)
1. Empty flush calls at texrect and tex sites: guard before pm_gpu_flush_at. Saves 249 (pause) and 167 (file select) calls per frame, 0 real closes. ms: not measured. Test: count pm_ec_n per frame (pm_gpu.c:2160) with the change; the quantity must show the change in the picture.
2. Merge consecutive texrects with the same tile (rect batch path, flush_impl). Saves up to 18 real rect closes (pause) = 1.75 ms, and 39 (file select) = 3.78 ms. Test: fl_consec per frame from gbiprof (must fall in the picture).
3. Pause label and glyph textures into one atlas. Saves up to 151 real tex closes (pause) = 14.6 ms, and 111 (file select) = 10.8 ms. At the measured 23.6 us per close: 3.6 ms. Large change, asset build needed. Test: real tex closes per frame from gbiprof on the label set.

## Next
- Run gbiprof on the 1301-ui build for items 2 and 3 (per-site counts). Run the partner padscript above for item 3 after the tab-entry line.
