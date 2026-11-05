# plan_source_opts: source-level and display-list cuts after C1 (O3DS yard 58 ms to 33.4 ms)
(Opus plan, saved by the orchestrator. Tags: [M] measured, [C] read from code at HEAD a1a3ced7d, [I] inference. All rows follow docs/handoff/hw_guidelines.md. Tests use the azt catalog scenarios, not golden.py.)

## 0. Facts
- Fn keys named from build3ds/lane-w1/pm_3ds.map [M]: 0015fee8 = appendGfx_model (1206 ops); 00163914 = appendGfx_model_group (837, recurses into appendGfx_model); 00214320 = appendGfx_animator (300); 00211608 = appendGfx_entity_model (172).
- "WorkerBackUI" (549) is mis-attributed: P1 credits every op until the next mark to the last mark. It also holds render_hud_elements_backUI, render_effects_UI and render_game_mode_backUI (= draw_status_ui, the status bar) [C main_loop.c:204-211, state_world.c:22].
- "bg" (358) = appendGfx_background_texture [C background.c:250-301]. The 296x200 CI8 sky is drawn in 34 strips of 6 rows: each strip is LoadTextureTile (7 ops) plus 2 texrects, a separate texture source, a cache lookup and a close.
- Model ops per object [C model.c:1473-2020]: about 30 interpreted ops per map model, about 68 models per frame. Texture DL about 17 ops (TLUT 6, LoadBlock 7, SETCOMBINE, TEXTURE, othermode): C1 removes these. Render-mode DL 6 ops (G_DL, SetRenderMode, SetCycleType, Clear/SetGeometryMode, ENDDL). 2 PipeSync, MTX, POPMTX, G_DL to geometry (dlc), plus fog/tint ops.
- W1 calibrated: class sums 11.8 ms against taskpl 12.2 ms. Console = Azahar x 2.86. Console us per op by class: sync 4.1, geo 3.7, dl 4.7, cc 8.9, mtx 9.1, tex 12 (about 5 without flush closes), oth (texrects) 40.
- PipeSync, LoadSync, TileSync, FullSync and NOOP have an empty handler [C pm_gbi.c:3489-3497], yet each runs the full prologue: two streak calls, the W1 hook, the gv_epoch++ that breaks batch append, and the pdl clears.

## 1. Producers above 100 ops per frame (yard [M])
| # | Producer / pattern | ops/frame | why | Restructuring |
|---|---|---|---|---|
| 1 | appendGfx_model + _group texture DL | about 1109 | 17 per model: full TMEM load re-emitted every frame | C1 |
| 2 | RDP syncs | 819 (about 560 after C1) | no-ops on PICA | skip before the prologue (G-SYNC) |
| 3 | appendGfx_model render mode | 438 (73 x 6) | same 4 state ops per model, mostly equal to the last | native render-mode op with an equality test (C2 = RM) |
| 4 | status bar via hud_element_draw_rect | 549 | about 20 per element [C hud_element.c:366-611] | inline texture-bind op (C1i); generic filter dedups mode/combine/filter |
| 5 | bg sky strips | 358 | 34 x (7 load + 2 rects) + 18 setup | TARGET_3DS: one full-image bind, 2 rects |
| 6 | appendGfx_animator | 300 | G_DL to Gfx_RM1/RM3 per node + 3 PipeSync + LOD/LUT/combine [C animator.c:787-968] | reuse RM op and G-SYNC |
| 7 | appendGfx_entity_model | 172 | own texture setup + 9 PipeSync | G-SYNC; C1i swap if above 100 |
| 8 | sprite_component | 104 | flat quad | battle row only, after measuring |
| P | Pause (no P1 data) | about 2366 console ops | per glyph LoadTextureBlock_4b 7 + texrect [C msg_draw.c:1754-1797]; draw_box; partner portrait 18 tile loads [C pause_partners.c:418] | glyph C1i swap |
| F | File select (no data) | 1864 | glyphs, draw_box, file-card HUD | same |
| B | Battle (no data) | 2113 | actor sprites, battle HUD, models, effects | same rows |
Measure where P1 has no data: add three marks FI_OPEN("hud_backUI") before render_hud_elements_backUI, FI_OPEN("fx_UI") before render_effects_UI, FI_OPEN("gm_backUI") before render_game_mode_backUI (phase only, no picture effect). Scenarios: battle (`preset=toad battle`), pause (`preset=yard padscript=START,wait60`), file select (boot, no preset). One `gbiprof` yard and pause run, counts only, for the `same` equal rates (G-EQ).

## 2. Rows (real = console ms estimate; Az = calibrated Azahar)
| Row | Ops removed/frame | real / Az ms | Lines | Risk | Azahar quantity | Picture check |
|---|---|---|---|---|---|---|
| G-SYNC: at the loop top, right after the w0/op decode, `if (op in {E6,E7,E8,E9,00,E0}) { pc++; skip_n++; continue; }`; P1/W1 count the skips as class "skip" | about 560 yard, all syncs in UI scenes | 2.3 / 0.8 | about 10 (pm_gbi.c) | low | opsplit sync n to about 0, taskpl main down 0.5 ms or more | yard, pause, file select shots |
| C2 = RM: model.c 7 sites `gSPDisplayList(ModelRenderModes[i])` become one port op (w1 = i) under TARGET_3DS; handler computes exit omode_l/h and geom from the static list, returns when all equal, else applies through the existing handler code | about 365 | 2.2 / 0.76 | about 60 (model.c, pm_gbi.c) | low-medium: cycle-type sync_combine and pm_gpu_set_mode call must stay | prod model+group ops down 300 or more | yard plus a fog map shot |
| BG: TARGET_3DS path in appendGfx_background_texture: one LoadTextureTile of the whole image (or C1i), texrects for the full height, 2 rects split at the texOffsetY wrap; wave mode keeps the strips | about 330 plus 33 binds and closes | 3 to 5.5 / 1.5 to 2.0 | about 40 (background.c) | medium: a 59 KB CI8 source as one texture (pads to 512x256); fog palette path | taskpl bg us from 2364 to 900 or less | yard sky pixel-equal, wide on and off, one fog bg map |
| C1i: C1 handler variant with an inline record (2 extra words, like texrect), filled by the producer at emit time: img, fmt/siz, w/h, rect, pal | 0 | enabler | about 50 (pm_gbi.c) | medium (TMEM records ldrec/blk) | tested by the next row | |
| HUD: hud_element.c per-file `#define` override (TARGET_3DS) of gDPLoadTLUT_pal16, gDPLoadTextureTile(_4b), gDPLoadTextureBlock to C1i | about 320 (more in pause and file select) | 1.6 / 0.6 | about 20 | medium | prod hud_backUI ops down 250 or more | status bar equal; pause icons |
| G-EQ: before the prologue skip SETPRIMCOLOR/ENV/FOG/BLEND/COMBINE/OTHERMODE_H,L/TEXTURE/GEOMETRYMODE when the live interpreter state already equals the result (compare to prim+lod, envc, fogc, blendc, comb_w0/w1 with comb_set && !box_folded && sc_ok, omode & mask, tex_on/ss/ts, (geom & clr) or set). No shadow copy, so no invalidation | yard 100 to 200 after C1/C2/HUD; pause more | 0.9 to 1.7 / 0.3 to 0.6 | about 40 | low-medium: box_folded and sc_ok side effects | new skip counter equals the gbiprof same counts and the walk drops | pause text, file select flip |
| ANIM: RM op swap at animator.c:794-837 | about 150 | 0.7 / 0.25 | about 15 | low | prod animator ops down 100 or more | yard |
| GLYPH: msg_draw.c override of LoadTextureBlock_4b/Tile_4b to C1i | 0 yard; pause about 900 | pause 4 / 1.4 | about 15 | medium | pause prod draw_msg ops | pause and file select text pixel-equal |
| SPRITE (battle) | measure first | | about 40 | medium | battle prod sprite_component | battle |
Generic dedup: skip rate in the yard: syncs 100% of 819 ops (23% of all ops), about 560 after C1; equal state 40 to 60% of the remaining cc/geo ops (history: sync_combine inputs equal 62%, SETTILE tile 0 75%). Pause and file select hold most of the value. Do not cache a "last word per opcode" shadow: compare against the live state (dlc/pdl replay and per-frame resets write it).

## 3. Game logic and effects
No rows. PH_LOGIC is 5.8 ms = 10% of 58 ms, below the 15% gate. Re-gate after lane 2 (14.5% at 40 ms, 17% at 33.4 ms); then split PH_LOGIC with fine phases. Effects are under 24 ops each in the yard. No rate cuts.

## 4. Order for lane 2 after C1 (console real ms [I])
| Step | Row | Saves | Total |
|---|---|---|---|
| start | 58.0, C1 midpoint 4 | 4 | 54.0 |
| 1 | C2 = RM | 2.2 | 51.8 |
| 2 | G-SYNC | 2.3 | 49.5 |
| 3 | BG | 3.5 | 46.0 |
| 4 | C1i + HUD | 1.6 | 44.4 |
| 5 | G-EQ (after the gbiprof census) | 1.2 | 43.2 |
| 6 | ANIM | 0.7 | 42.5 |
| 7 | W4+W2, W3, W6 (plan_walk_cost midpoints) | about 4 | about 38.5 |
| 8 | GLYPH (pause/file select) | | |
| 9 | SPRITE (battle, if above 300 ops) | | |
Stop rule: a row is kept when its Azahar quantity moves by at least half the table value and the picture is equal. A changed picture reverts the row; no third try on the same producer. Two kept rows in a row that each move the Azahar walk under 0.3 ms (about 0.85 ms console) end the lane. The final console push confirms the estimates (`opsplit`, `prod`).

Honest outcome [I]: about 38 to 43 ms in the yard on O3DS (23 to 26 fps); this does not reach 33.4. About 5 to 9 ms remain: 30 to 40 real closes at about 97 us, dlc replay 2.7 ms, PH_GPU + GCACHE + GDRAW 8.4 ms, logic 5.8 ms, frame begin/end 3.2 ms.
Architecture options for the rest: (a) a retained per-model draw: one op per map model carrying the texture bind, render mode, matrix and dlc handle; takes the model producers to about 1 op each and merges closes for consecutive models with the same texture; about 3 to 6 ms [I], about 150 lines, high risk. (b) Walk on N3DS core 2: 0 ms on O3DS. (c) Accept a stable 25 fps (40 ms) on O3DS. The user's target is 30 fps, so (a) is the next row after step 9 if the measured total is still above 38 ms.

Files: pm_gbi.c (gbi_run_body loop 2589-2660, handlers 2920-3497), src/model.c (appendGfx_model 1369-2021: render-mode sites 1636/1700/1764/1773/1899/1937, texture DL 1523/1527), src/background.c (250-301), src/hud_element.c (265-611), src/main_loop.c (gfx_back_ui 204-211), src/animator.c 787-968, src/msg_draw.c 1754-1797.

## Addendum (planner v2, with hardware rows; this order replaces section 4)
Facts [M map]: gbi_run_body is 0x5e04 = 24.0 KB, flush_draw (pm_gpu.c) 14.8 KB, sync_combine 1.7 KB, ensure_tile 1.2 KB, load_mtx 1.1 KB; gm_tab 256 x 64 B = 16 KB plus gm_nrm 12 KB = 28 KB of linear-scan data. Hot code is far over the 16 KB I-cache.
Generic dedup placement: one `u8 opf[256]` flag table (256 B, 8 lines, always hot) read once per op before the streak, epoch, gp and os hooks; skipped ops cost about 10 instructions. Skips are counted in opsplit as `sk n` so P1 and W1 stay exact. Extra P1 marks: hud_backUI, gm_backUI, draw_box.
Hardware rows:
- H1 (W6+): split gbi_run_body: rare cases (FILLRECT, SETCIMG, KEYR/GB, CONVERT, MOVEMEM viewport, bad-dl paths, xt/boxlog/pipelog/po3a/mtxlog/dd blocks, the ~20 pm_log calls) into `__attribute__((cold,noinline))` functions in pm_gbi_cold.c compiled -Os. Target: hot loop plus top handlers (DL/ENDDL, VTX, TRI, MTX, tex 7, cc 6) at 8 KB or less. Console 2 to 5 ms [I]. Azahar: only "not slower" and the picture.
- H2: same for flush_draw (14.8 KB): debug, dc timers and the off-screen/snap paths go cold; -Os for the pm_tex.c miss path, pm_phase.c; delete dead pdl_try/pdl_try_m in pm_dlc.inc.c. 1 to 2 ms [I].
- H3 (W3): gv_matrix_id 64-entry index (FNV of 16 words to u8 slot, 64 B) replaces the newest-first memcmp scan over gm_tab/gm_nrm. 0.5 to 2 ms.
- H4: PLD of the G_DL target at the push and of the next vertex block in the G_VTX copy. 0.2 to 0.5 ms.
- H5: divides in hot paths: texrect s0/t0/s1/t1 divide by tw/th 4 times per rect (pm_gbi.c ~3172-3187) to one reciprocal per bind cached with the tile; G_VTX `tex_ss / 65536.f` per vertex to a precomputed scale on G_TEXTURE; `/268u` timer divides in pm_gpu.c:719-724 and :456 to one divide at the report. 0.2 to 0.6 ms.
- H6: GbiState hot fields (omode, comb_w, prim, env, geom, tile0, epoch) in one 64 B struct aligned 32, used by G2 and the RM op.
Order after C1 and C2 = R1 (render-mode op): G1 sync skip (2.3), H1 cold split (3.0), R2 background single bind (4.0), G2 equality filter + H6 (0.9), H3 matrix index (1.0), R3 HUD inline bind (1.6), H2 flush_draw cold split (1.5), W4/W2 (1.0), H5 divides + H4 prefetch (0.6), R4/R5 glyphs (pause/file select only). Mid estimate: the yard ends at about 36 ms, 2.5 ms short of 33.4; it reaches it only if C1 and H1 land at their high ends. What is left: dlc replay of map geometry 2.7 ms, about 67 real closes x 97 us, logic 5.8 ms. If the final console build is still above 33.4 ms: (a) stable 30 fps on N3DS only and 20-25 on O3DS, or (b) per-model pre-baked command buffer keyed by producer identity (only remaining lever of 5 ms or more on the O3DS; high risk).
Stop rules: revert any row whose Azahar quantity misses or whose picture differs; no third try on the same producer; stop after two rows in a row that each save under 0.5 ms Azahar walk (H rows: under 1 ms on the console in the final build); stop once the yard is at or under 33.4 ms on the console.
