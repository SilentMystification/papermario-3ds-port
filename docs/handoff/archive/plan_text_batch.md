# plan_text_batch: one draw batch per text run (pause, file select, dialogue)
(Opus plan, saved by the orchestrator in condensed form. Tags: [M] measured, [C] read from code at main 8362b7864, [I] inference. Rows follow docs/handoff/hw_guidelines.md.)

## 0. Facts that change the premise
- [C] Glyphs already merge into one rect batch inside a message. GA1 took consecutive texrects from 0 to 82 per frame (rect_begin, pm_gpu.c:2361, merges on the same ec and texture). The 122 tex closes come from state switches at message edges, not from glyphs.
- [M] TEXI removed 412 op prologues and saved 137 us (about 0.33 us per prologue). GA1 removed 91 closes plus wrap calls and saved 370 us (about 4 us per close). The 13 us per op average (14.2 ms / 1092 ops) does NOT come from prologues or glyph closes. It comes from a few heavy ops or per-message work not yet located.
- [C] Per-message work: msg_reset_gfx_state (msg_draw.c:1663) runs PipeSync + G_DL D_8014C500 (15 ops: RM, CC, filter AVERAGE, ...). The first glyph emits gDPLoadTLUT_pal16 (~6 ops, :1754). The atlas is per (sheet, palette) (pm_tex.c glyph_build_atlas), so a palette change is a new bind. A filter toggle flushes at pm_gbi.c:3105, the TLUT at :2875, combine/prim at set_plan (pm_gpu.c:2224): about 3 flush sites per message edge.
- [C] Per glyph today: G_PM_TEXI runs tb_inline (pm_gbi.c:2654), which runs 5 tex_op words. SETTILESIZE calls pm_tex_glyph_same (16-entry palette FNV, plus glyph_resolve with a `%` and a `/` by 128/96/72, no HW divide). G_TEXRECT (:3594) runs sync_render, off_screen_check, box_fold_sync, box_pattern_rect, ensure_tile. ensure_tile calls glyph_atlas_bound, which calls pm_tex_glyph_same again. Then pm_tex_size, 4 float divides, pm_gpu_tex_rect (6 put).
- [C] SUSPECT FOUND: pm_gbi.c:3047 `pm_log_d("glyphbind ...")` runs on every glyph with no switch gate. Measurement runs use log level 3, so every glyph pays a vsnprintf plus a queue copy (vlog, pm_log.c:309). THE CONSOLE PUSH COMMAND USES loglevel=debug, SO THE CONSOLE PAYS IT TOO. On a level 1 player build it is a call and 2 tests.
- [C] Drop shadow (msg_draw.c:1514-1541) puts PipeSync/RM/CC/prim around each shadow glyph: 2 set_plan closes per glyph, only in DROP_SHADOW messages (pause_items.c:261, pause_badges.c:491).

## 1. Time model of one glyph (Azahar; console = x2.86)
| part | ops | Azahar us [I] | counter |
|---|---|---|---|
| TEXI: 5 tex_op + glyph_same + glyphbind log | 1 | 5-20 (log 3-15 of it) | gbiprof op 0b |
| texrect handler + ensure_tile + 6 put | 1 | 5-14 | gbiprof op e4; gabsplit n |
| close in the run | 0 (merged) | ~0 | flsplit rect n/e |
| per message: G_DL D_8014C500, TLUT, filter, plan, PipeSync | ~25 per message | 3 flushes x ~4 us + handlers | gbiprof de/f0/e3/fc; flsplit tex/cc; combsplit |
G ~ 90 glyphs, M ~ 30-40 messages [I]. Glyph ops 0.9-3 ms Azahar; message edges 1-2 ms. Total about 2-5 ms of the 14.2 ms. The rest is unexplained by code reading, so R0 runs first.

## 2. Design: a text run as one op (R1)
- Producer (msg_draw.c, TARGET_3DS): msg_draw_char appends a 16 B record. If gMainGfxPos still equals the end of the open run and the header key matches, it bumps n; else a new header. Split rule = contiguity: any other Gfx between two glyphs ends the run (shadow, opacity, dither, images, scissor, SETCIMG keep their exact order). A header key change also splits (raster base, palette, texSize, dsdx/dtdy).
- Header (2 Gfx): w0 = G_PM_GLYPHS<<24 | texW<<18 | texH<<12 | n; w1 = raster base; w0' = palette (D_802F4560[pal]); w1' = dsdx<<16 | dtdy. The palette moves into the header, so the per-message TLUT ops go away.
- Record (2 Gfx = 16 B): s16 ulx, uly, lrx, lry; u16 charIndex; s16 texOffsetX, texOffsetY; u16 pad.
- Handler (pm_gbi.c, noinline), once per run: sync_render, off_screen_check, box_fold_sync, set td[0] to glyph 0, ensure_tile(0) (atlas bind through glyph_remap), 1/aw, 1/ah. Per record: cell gx = (idx & 15)*gw, gy = (idx >> 4)*gh (cols 16, shift/mask, no divide), s0 = (gx + soff/32)*raw, same for t, then pm_gpu_tex_rect. Rects merge in rect_begin: one close per run.
- Fallback: if the atlas is not bound after ensure_tile (font not in glyph_sheets), run the old per-glyph path. Exit state: tlut_pal[0] = palette, ens_clear(), st0_ok = 0, cur_tile = -1.
- Not built until numbers say so: a text TEV mode putting shadow and glyph in one batch.

## 3. Rows
| Row | Lines | Change | Azahar quantity (pause f599, phase on) | Gate |
|---|---|---|---|---|
| R0 | ~3 | Gate glyphbind (pm_gbi.c:3047) behind a static pm_debug_has("imglog") like settdbg. Run azt pause_menu with gbiprof + phase. | gbiprof top ops (0b, e4, de, f0, e3, fc) ns/call and count; flsplit; combsplit; logstat lines per frame | draw_msg us drops by the log share; the section 1 table is filled with numbers |
| R1 | ~70 | G_PM_GLYPHS run op | draw_msg ops 1092 to 750 or less; gbiprof 0b+e4+f0 time down >= 80%; tex closes 122 to M+20 or less; glnsplit fallbacks = 0 | walk down >= half the R0-measured glyph+TLUT time; pictures equal |
| R2 | ~35, only if R0 shows filter/plan edges >= 1 ms | skip the D_8014C500 G_DL when the run left the state equal; filter in the run bind | flsplit cc+tex closes; gbiprof de | >= 0.5 ms Azahar |
| R3 | ~40, conditional | text TEV mode for shadow + opacity in one run | combsplit plan/prim per frame | >= 0.3 ms |
Picture checks for every row, byte-equal to the R0 baseline: pause status, items (shadowed selection), badges, partners, map pages; text_menu; file select; dialogue. The spinning coin at x 90-101, y 192-202 is known noise. Stop rule: a missed gate or changed picture reverts the row; stop after two rows under 0.5 ms walk each.

## 4. Hardware notes
16 B records, 8 B aligned in the Gfx stream (2 per 32 B line), written once in order; no side buffer, no malloc. Handler reads in order and PLDs 2 lines ahead. No divide per glyph: 2 reciprocals per run. pm_tex_glyph_same hashes once per run, not per glyph. The handler stays small (about 0.5 KB), noinline, outside gbi_run_body. Confirm on the console with opsplit oth and tex.

## 5. Expected result and risks
Azahar pause walk [I]: R0 -0.3 to -1.3 ms (log share), R1 -1.5 to -3 ms, R2/R3 -0.5 to -1 ms: total about -2 to -4.5 ms of 14.2 ms; console x2.86: about -6 to -13 ms. Honest: TEXI and GA1 moved the walk only 0.14 and 0.37 ms, so the glyph path may be smaller than the model; if R0 shows 0b+e4+f0 under 1 ms Azahar, drop R1 and look elsewhere (a heavy op gbiprof names, or tick-read overhead). Risks: text colors (palette in the header, new run per COLOR change; rainbow gives 1 glyph per run), drop shadow (state ops split runs; correct, no gain until R3), window scissor and rotscale off-screen (Gfx between runs), alpha/dither fade (ops split the run), size effects RISE/GROW/JITTER (runs of 1), clip offsets carried per record, exit-state invalidation (at most 1 extra bind per message), iQue old path.

## 1a. R0 measured (dl_r0, Azahar, azt pause_menu args + phase gbiprof loglevel=debug, shot frame 180)
Window: last 300 frames of each log. Base = main 91ed30966 (glyphbind ungated). R0 = same plus the imglog gate on pm_gbi.c glyphbind.

| op | count/frame | base ns/call | base ms/frame | R0 ns/call | R0 ms/frame |
|---|---|---|---|---|---|
| 0b (TEXI) | 103 | 61107 | 6.2 | 11212 | 1.1 |
| e4 (texrect) | 212 | 19408 | 4.1 | 19412 | 4.1 |
| de | 39 | 4395 | 0.1 | 4414 | 0.1 |
| f0 | 54 | 12479 | 0.6 | 12587 | 0.6 |
| e3 | 78 | 13429 | 1.0 | 13435 | 1.0 |
| fc | 68 | 12853 | 0.9 | 12887 | 0.9 |

- Main walk, last 300 frames: base 15789 us, R0 9833 us (-5956 us, -38 percent). bg 2647 us both. Azahar direction only.
- Base log: 959549 glyphbind lines in one run. R0 log: 0 glyphbind lines.
- opsplit f: ops 1471 both. combsplit plan 66 mode 16 prim 11 env 2 lay 53 con 13 both. flsplit tex 122/50 both; tex third field 1929 base, 3649 R0 (field not identified).
- Picture: pause shots 0000 to 0003 byte-equal (base vs R0).
- Gate for R1 (0b+e4+f0 at 1 ms or more): 1.1 + 4.1 + 0.6 = 5.8 ms per frame. Met.
- Other ungated per-op logs in pm_gbi.c, pm_tex.c, pm_gpu.c: none found. Checked and behind a switch or a cached flag: pipelog, mtxv, mtxlog, xt fill and texrect, box texrect, pipemtx, flushlog, rectdbg, glyphmap, glyphmiss, tex ptr, box raw (flush_boxlog).
