# System audit of the 3DS runtime (row AUDIT, 2026-11-08)

Scope: nds1 at 53e5a553e (after the GREY and CFG merges). Read-only pass over 3ds/src nd_*.c and pm_*.c, 3ds/include, the three vertex shaders and the walker entry points. Code sizes come from `arm-none-eabi-nm -S` of the lane-audit build of 53e5a553e. Struct sizes come from gdb `sizeof` on build 2870, and the struct definitions have not changed since that build. Line numbers are for 53e5a553e.

## 1. Summary

- The systems are coupled through pm_gpu.c globals. The walker (nd_*.c) writes draw state through about 20 pm_gpu_set_* setters. Each batch close rebuilds 3 keys from those globals, hashes them, and copies the result into a 588-byte NdDraw.
- The biggest cross-system costs are per batch and come from three sources: a debug census that runs with no switch (A1), two keys built for one fact (A2), and pm5 draws that skip the command shadow (A3). Together they add a hash, about 2.4 KB of memset and copies, and a full template re-emit to every pm5 batch.
- On the hot path, the batch close needs 19,288 bytes of code (A5) and the L1I is 16 KB. A texture miss scans up to 139,264 bytes of slot records (A6).
- One latent correctness fault: at 30 fps, ring chunks 1 to 7 are never reused. The 8th ring wrap of a run fails and drops draws (B1).
- Two seam risks have numbers. Prim depth uses different formulas on the rectangle and triangle paths: they differ by one prim unit (B2). The texrect uv quantization step depends on the magnitude of s (B3).
- Maintainability: 1 counter with no reader (tex_gen), unreachable code in the walker, a dead shader in the build, 66 debug words that nothing uses, and 524 row-code comment tags in the draw files.
- Threads: 8 threadCreate sites, as expected. No lock is held across the game thread's draw path. All logging is lock-free on the caller side.

## 2. System map (A0)

Threads (`grep threadCreate`): game pm_main.c:804 (core 0); audio pm_audio.c:112/115 (core 1, or the system core with audiocore0); audio render pm_au2.c:177; flash pm_flash.c:86/87; log writer pm_log.c:956; log step pm_log.c:1031; log sink pm_log.c:1191; watchdog pm_wdog.c:154.

| Interface | Producer -> consumer | Data, owner | Copies per crossing | Frequency | Thread crossing | Duplicated state | Order dependency | Verdict |
|---|---|---|---|---|---|---|---|---|
| Game loop, pacing | pm_frame_loop -> pm_gpu_begin/end | sched_next, step_vb (pm_main.c) | none | per step | none on the draw path; C3D_FrameBegin(0) at 30 fps (pm_gpu.c:2358) | 3 pacing clocks (sched_next, step_vb, fps_vb0) | begin before retrace (pm_main.c pm_gpu_begin(1) then pm_gfx_retrace) | OK. The 30 fps begin passes no SYNCDRAW, so ring_done is never set (B1). |
| Producer -> walker | src/ records (G_PM_REC/SPR/ENT/ND) -> pm_nusys.c gbi_walk -> nd_walk_run | Gfx lists, game owns | none (walked in place) | per list; stereo walks the list twice (pm_nusys.c:94) | none | none | the record must precede its vertex word (nd_walk.c G_PM_SPR spr_at) | OK. The second stereo walk doubles walker CPU (row R10). |
| Walker -> geometry -> ring | nd_geom_vtx/gv_tri -> pm_gpu_gtrip/gappend | ring Vtx (pm_gpu.c gring), vert_* SoA (nd_geom.c:18-27) | 1 Vtx copy per load; 1 more copy per palette change (gv_recopy_n) | per vertex and per triangle | none | ep_p params (nd_geom.c) vs g_uvso/g_vp/g_cmul/g_cadd/g_zmix (pm_gpu.c:138) compared by memcmp each triangle | ep_open_gen must equal flush_gen (nd_geom.c gv_tri) | Palette search is done 3 times per triangle (A8). |
| Batch close -> GPU | flush_draw -> nd_raw_native -> nd_tmpl_build -> nd_native_submit -> nd_emit_draw | NdTmplKey, TevKey, NdDraw (588 B), NdShadow (1,848 B) | key 2x, NdDraw memset + 1 to 3 copies | per batch | none | combine fact in 5 places (A9); TevKey vs NdTmplKey (A2) | C3Di_UpdateContext must run before the raw words (pm_gpu.c:1015) | Highest per-batch cost (A1 to A5). |
| Texture cache | nd_tex_set.c tile ops -> pm_tex_load -> slot -> stage -> upload -> pm_tex_native_unit0 | Slot[1024] x 136 B (pm_tex.c:22-56) | texel expand 1x; stage copy 1x | per texture op | uploads before C3D_FrameBegin (pm_gpu.c pm_tex_upload_vram_before_frame) | tex_gen is never read (C1) | a load before the bind (pm_tex_bind) | A full scan on a miss (A6). UB1 suspect 1 is known: see row_UB1.md. |
| Audio | pm_audio_step -> pm_audio_kick -> au_main (core 1) -> pm_au2 render | ticks and the AU2 ring, pm_audio.c | none | per step: 1 kick, 1 to 4 joins (pm_main.c:599, pm_nusys.c:157/163, pm_phase.c:754) | LightEvent au_start/au_done; the wait is untimed until the first sleep (pm_audio.c:217) | none | join before game code that touches audio (AU1d comments) | OK. pm_debug_has("audiostat") runs on every kick (pm_audio.c:198). |
| Input | pm_pad_scan (every loop pass) -> game pad and menu | pad state, pm_input.c | not checked | per loop pass | none | not checked | not checked | Not checked beyond the call site. |
| Menu, options | pm_options_ui.c <-> pm_opt, pm_log levels | pm_opt (pm_options.c:14) | none | at change | menu present (GREY, merged) | PMD caches a switch forever, but PMDL follows pm_dbg_gen (C5) | none | OK. The draw code reads pm_opt once (pm_gpu.c). |
| Logging | callers -> vlog lock-free queue -> writer / sink / step threads | slots[], pm_log.c | args copied once (lf_copy) | per call | CAS only on the caller (pm_log.c:433-449); fb_lock and con_lock on the sink | none | none | OK. When off, a call costs one qon test and one level compare (pm_log.c:427-428). MENUB races: row_MENUB.md. |
| File I/O, map load | ROM read once at boot (pm_os.c:266); map change -> pm_dlc_gen -> lazy dlc_reset/ent_reset | ROM block in memory | none per map | per map | flash thread (pm_flash.c:24-25 two locks) | the same gen check in 4 places (nd_ent.c:308,341; nd_shape.c:408,461) | the reset must run before the next dlc_try (lazy) | OK. No blocking read on the game thread after boot. |
| Memory | linearAlloc at init (pm_gpu.c:1464); ring chunks (pm_gpu.c:2140); malloc at compile time (nd_dlc.c:85-94, nd_ent.c:190,213) | owners per module | none | at init or at map load; no per-frame malloc | none | none | none | Ring chunks are never reused at 30 fps (B1). |

## 3. Findings

Ranked by how much they cut cross-system cost or coupling. Tag: the interface row, or local. Size: S, M or L. Status: open until a row marks it done here.

### A. Console performance (per-op cross-module work, duplicated state)

**[done W1] A1 (batch close, S). The draw census runs on every native batch with no switch.**
pm_gpu.c:441 says "The accumulation has no switch test". `cen_add(&tk, ...)` at pm_gpu.c:1244 runs on every native raw batch. For each batch it copies a 76-byte TevKey, hashes 18 words with FNV (pm_gpu.c:456-465), calls pm_tex_census_id, and probes the 256 x 20 B table (5,120 B, pm_gpu.c:444-454). cen_frame_end clears the 5,120 bytes every frame (`if (cen_nt) memset(cen_tab, 0, sizeof cen_tab)`, pm_gpu.c:595). The table only prints at the frame given by drawcensus=<frame> (pm_gpu.c:589).
Fix: read drawcensus once at boot, and call cen_add and the memset only when it is set.

**[done W2] A2 (batch close, M). One draw state becomes three keys per batch.**
nd_raw_native builds NdTmplKey (44 B: memset and 30 stores, pm_gpu.c:1215-1225). It then builds TevKey from the same globals (76 B: memset, 19 stores and a 76-byte memcmp, pm_gpu.c:1233-1237). TevKey adds only prim, cconst, mixcol and plan_gen. nd_tmpl_build copies the key again (`NdTmplKey kk = *in`) and hashes it one byte at a time (44 FNV steps, nd_tmpl.c:309-313). cen_add hashes TevKey a third time (A1). flush_tev_rebuild builds the same two keys (pm_gpu.c:734-747).
Fix: keep one key (NdTmplKey plus the 3 colour words) in pm_gpu state. The setters update it and mark it dirty. nd_raw_native rebuilds and looks it up only when it is dirty. Hash by word.

**[done W3] A3 (batch close, M). pm5 draws clear the command shadow on every draw.**
nd_native_submit calls `nd_shadow_reset(&ndn_sh)` for every use5 draw, even when prog5 is already bound (pm_gpu.c:1001-1003). The shadow is 1,848 B, and the reset clears all of it. After a reset, nd_emit_draw writes every template register (up to 64) and every unit block again (nd_emit.c:37-77). The pm5 uniforms go through C3D_FVUnifSet (pm5_upload pm_gpu.c:1155-1170, pm5_upload_draw :1050-1062), and citro3d sends them again at C3Di_UpdateContext. A model pm5 draw also copies its 588-byte NdDraw into d5 (pm_gpu.c:995).
Fix: reset the shadow only when the program changes. Write the pm5 uniforms with nd_emit `uni()` against the shadow, at the u5_* locations.

**[done W3] A4 (batch close, S). pm5 raw batches copy palettes that the draw then discards.**
pm_gpu.c:1248-1252 copies up to 21 x 64 B of pcomp into palk. pm_gpu.c:1268 then sets `d.npal = 0` for g_use5, and pm5_upload computes and uploads the same pcomp entries again.
Fix: skip the palk loop when g_use5 is set.

**[done A5] A5 (batch close, M). The hot code is larger than the L1I.**
The batch close path is 19,288 B of code: flush_draw 1,524 + nd_raw_native 8,996 + nd_native_submit 4,184 + nd_emit_draw 2,040 + C3Di_UpdateContext 1,256 + nd_tmpl_build 996 + ndunit_after 200 + GPUCMD_AddRawCommands 92. The L1I is 16,384 B (hw_guidelines rule 1). The per-triangle walker path is another 11,228 B: nd_walk_run 3,268, draw_tri 604, gv_tri 1,788, gappend 676, gtrip 3,108, nd_geom_vtx 1,784.
nd_raw_native contains debug blocks inline: the wl_on wdraw/wsurf logs (pm_gpu.c:1274-1287, a 40-argument pm_log and a vertex loop), orb_log (:1230), ndplan_check (:1229), the ndm_cmp call (:1273) and grablog (:1204).
Fix: move each debug block into a cold, noinline function behind one flag test. Together with A2, which removes TevKey, nd_raw_native should fall under 4 KB.

**[done W7] A6 (texture cache, M). A texture miss scans the whole slot table.**
In load_inner, a load that misses the 256-entry direct index (pm_tex.c:1631) and the recent list (:1643) scans every slot below slot_hi (pm_tex.c:1656). That is up to 1,024 x 136 B = 139,264 B, before load_miss. A true miss always pays the full scan. Every call also hashes the full CI palette, hits included (pm_tex.c:1608-1625).
Fix: use an open-addressing index keyed by (img, tlut, w, h, fmt, siz, stride, dkey), in the same form as gm_idx (nd_mtx_set.c:47) and tmpl_idx (nd_tmpl.c:300). The table is then never scanned.

**[done W5] A7 (walker -> ring, S). gappend runs the wide check 3 times per triangle.**
In pm_gpu_gappend, the `if (pm_wide && ...)` block is inside the per-vertex loop (pm_gpu.c:2502-2506). It calls wide_edge_k(pal_tab[g0], t3, 3) 3 times per appended triangle. That is 9 vertex transforms instead of 3, and wide_c.ortho counts 3 times too high.
Fix: move the block above the loop.

**[partly done W5] A8 (walker -> ring, M). The palette search runs 3 times per triangle.**
pm_gpu_gtrip searches pal_g in gtrip_batch_open (pm_gpu.c:2552-2556), again in the copy of the overflow count (:2650-2655), and again in the assignment loop (:2676-2680). gtrip_batch_open also runs 5 memcmp of draw parameters on each call, plus a 304-byte PmFx memcmp for pm5 (:2547-2551). gv_tri already knows that the parameters are unchanged (ep_pepoch, nd_geom.c gv_tri).
Fix: pass the epoch id instead of the parameters, and search the palette once.

**[not done] A9 (batch close, M). One combine fact lives in 5 places.**
The same fact is kept as nd_ws.comb_w0/w1, nd_comb cur_plan (192 B), ScMemo.plan x 16 (244 B each, nd_comb.c:42), pm_gpu plan + plan_k + plan_gen (pm_gpu_set_plan does a 192-byte memcmp and copy, pm_gpu.c:3380-3398), and NdTmplKey w0/w1 (nd_tmpl_build rebuilds the plan from the words, nd_tmpl.c pm_comb_build). The owner is nd_comb.c.
Fix: pm_gpu keeps a plan id (memo index) and the words only.

**[partly done W6] A10 (walker, S). sync_combine hashes its inputs twice and scans the memo.**
sync_combine hashes 11 words (sc_sig_now, nd_comb.c:467-473), then 10 more words (scm_sig, :436-442). scm_find then scans the 16 x 244 B memo by key (:444-447). The sc_sig hash is also copied inline at :582-585 instead of calling sc_sig_now.
Fix: compute one hash and index the memo by its low bits.

**[not done] A11 (walker, S). t1_fold_tri repeats the uv formula per triangle, with divides.**
t1_fold_tri (nd_comb.c:696-716) calls pm_tex_size and does 6 float divides per vertex on every triangle when ep_t1f is set. It recomputes uv with nd_gv_params's formula in a different operation order (nd_model.c:31-36).
Fix: use ep_p.uvso (u = tc * uvso.xy + uvso.zw).

**[done W6] A12 (game loop, S). Switch lookups run per frame and per kick.**
pm_debug_has runs every frame for "pacestat" (pm_main.c:472), "fpslog" (:614) and "logstat" (:639), for "prevcpu", "stereo" and "grabtest" (pm_gpu.c:2374, 2377, 2402), and on every audio kick for "audiostat" (pm_audio.c:198). Each lookup costs a hash plus a strcmp (pm_log.c:171-185).
Fix: use PMDL (pm_port.h:68), which costs one compare.

### B. Correctness and seams

**[done W4] B1 (memory, ring; S). At 30 fps, ring chunks are never reused.**
ring_done is set only when the frame begin waited with SYNCDRAW: `if (fb_flags) ring_done = ring_fr` (pm_gpu.c:2361). fb_flags is SYNCDRAW only when pm_interp_on > 0 (fps60; pm_gpu.c:2358), and the default is fps60 = 0 (pm_options.c:14). ring_pick skips any used chunk with stamp >= ring_done (pm_gpu.c:2134), and ring_done stays 0. So each ring wrap allocates a new 384 KB linear chunk (16384 x 16 + 65536 x 2, pm_gpu.c:122) that is never freed. After 7 extra chunks (RING_CHUNK_MAX 8, :131), ring_pick returns -1, ring_need returns 0, and rectangles and triangles are refused (rr_why[5], pm_gpu.c:3635, 3781).
Class: dropped draws, and up to 2.7 MB of linear memory held.
Fix: set ring_done = ring_fr after any frame begin that waited for the GX queue, or stamp chunks against pm_gx_done().

**[done B2] B2 (seam, S). Prim depth has two formulas: rectangle path vs triangle path.**
Triangles use nd_prim_ndc_z = 2(p-1)/32767 - 1 (nd_model.c:14). The `- 1` is PRIMZFIX, one unit nearer, and nd_target.c:22 gives one unit as 512 of the 24-bit depth. The G_ZS_PRIM texrect and the fill use prim_z / 32767 without the unit (nd_rect.c:236, 312), and they go through another matrix (the RECT_GM palette, pcomp_calc pm_gpu.c:803-807).
Class: edge (depth). A prim-depth decal drawn as a texrect and one drawn as triangles land 1 unit apart.
Fix: one helper for both paths, with a host test that compares the PICA depth of both paths for p in {1, 16384, 32734}.

**[open: no visible case] B3 (seam, S). The texrect uv step depends on |s|, not on the span.**
tex_raw_piece uses the base form only when mx >= 127 (pm_gpu.c:3735). The quantization step is therefore sc = 4096, 1024 or 256, chosen by the magnitude of s (:3741-3744). s and t are in texture units (nd_rect.c:196-197 divide by tw), so the rounding error is 0.5/sc of the texture. At |s| in [31.9, 127) that is 0.5 x tw/256 texels: 0.5 texel for tw 256. Neighbouring pieces of one split (pm_gpu.c:3783-3790), and neighbouring strips, can get different sc and bs.
Class: texel. One edge can sample positions that differ by up to 0.5/sc.
Fix: always use the base form (bs = floor(min s)), so that sc depends only on the span.

**[done W9] B4 (local, S). The host tests check a decoder the game does not run.**
nd_words (nd_state.c:35) decodes SETPRIMDEPTH as `w1 >> 16`. The walker (nd_set.c:59) uses `(w1 >> 16) & 0x7fff`. nd_words is "never called during the walk" (nd_state.c:2-3), so nd_gbi_test and nd_rec_test check a decoder the game does not run. The two differ when bit 31 is set.
Fix: make the walker setters and nd_words one function.

### C. Maintainability

**[done W1] C1 (local, S). tex_gen is written and never read.** It is incremented at 10 sites in 5 files (nd_comb.c:642, nd_rect.c:61,107,159,260, nd_target.c:66, nd_tex_set.c:338,366,505,555). It is declared at nd_tex_set.c:14 and nd_int.h:14. No code reads it. Fix: delete it.

**[done W1] C2 (local, S). Unreachable or no-op code:**
- nd_walk.c:193-195: `pc += w0 & 0xffffu; break;` comes after the G_PM_ENT `break`.
- nd_walk.c:202-203: the G_DL prefetch runs twice.
- nd_walk.c:326-334: the switch cases G_SPNOOP and RDP*SYNC never run, because g1_skip (nd_walk.c:32) skips them first.
- pm_gpu.c:1343: the cen_why ternary always gives CW_OTHER, because graw is 1 at that point (the !graw case returned at :1318).
- vert_sx/vert_sy are written (nd_walk.c:387-388) and never read.
- `if (lm) {}` at nd_mtx_set.c:109.

**[done W10] C3 (local, S). pm.v.pica is built and never bound.** pm.v.pica (DVLE 0) is built into pm_shaderall (3ds/CMakeLists.txt:338), but only DVLE[1] is bound (pm_gpu.c:1470). The comment at pm_gpu.c:1451 ("one DVLB ... a program switch sends no shader code") is stale, because pm4 and pm5 are in different DVLBs (:1486-1488).

**[done C4] C4 (local, S). 66 of 210 debug words are referenced by no bench, scenario, row file or tool:**
au2off au2trace audiohash audiolock battlelist battlen btlstatus camlog coreprobe crashframe crashtest crashtest_audio crashtest_audio_call crashtest_call creditsseek decalbias demobattle depthview dspaux dspflush dspfx dspgain emlog entlog entskip exitframe faulttest fillcyc fillog flashfake flashheavy flushlog fpslog fs1delay fs1old fxdx fxlist grabtest imglog lognull mapflags mashlog mashx modeldump msgtest noaudio padov padrand pauseskip pm5draw prevcpu prevtest profstall ptrcheck ptrtest rectred romstrict runaway shotskip simlog texnearest texrlog tourfx tx1check warptest wdog.
Deleting them is a user decision, because CLAUDE.md rule 7 says debug tools may stay.

**[done W6] C5 (local, S). PMD is defined 4 times and never sees a change.** The same PMD macro is defined at nd_comb.c:63, nd_rect.c:44, nd_set.c:18 and pm_main.c:14. It caches a word forever, so it never sees an options-panel change. PMDL (pm_port.h:68) does see it. Fix: use one macro, PMDL.

**[done W1] C6 (local, S). Counters print slots and names that are no longer true.** Only slots 4, 6, 8 and 9 of nraw_why are written (pm_gpu.c:1197, 1206, 1211, 1228), but nd_ent.c:288 and pm_heaphi.c:76 print slots 1, 2, 3, 5 and 7, which are always 0. rect_log prints rect_c[4] as "cpu" (pm_gpu.c:3678), but the counter now counts dropped rectangles (:3698, :3801).

**C7 (local, M). Comments name rows instead of reasons.** There are 524 row-code tags ("FB-2:", "S3N-3c:", ...) in nd_*.c, pm_gpu.c and pm_tex.c, and 1,098 in all of 3ds/src. Many of these comments give the row and not the reason. Fix: rewrite them as reasons when the file is next edited. Git history keeps the row.

**[partly done C8] C8 (local, L). pm_gpu.c holds about 12 subjects in 3,830 lines.** It holds GPU state, the ring, rectangles, sprites, the model cache, censuses, off-screen targets, grab, HUD, menu, screenshots and the fatal screen. Fix: split out pm_hud/pm_shot (UI) and nd_ring/nd_rectq (draw), along the existing seams.

**[done W10] C9 (local, S). Only one cold file is compiled with -Os.** Only nd_cen.c gets -Os (3ds/CMakeLists.txt:419). pm_warp.c, pm_dbg_game.c, pm_fxtest.c and pm_sweep.c are debug-only and are built at -O2 (CMakeLists.txt:9). hw_guidelines rule 1 says cold files are built -Os.

## 4. Work list (this agent, in order; ownership first)

| Id | Findings | Gate | Status |
|---|---|---|---|
| W1 | A1 census gated; C1, C2, C6 dead state | build, nd_host, 5 det byte gates | done (all 5 byte-equal to base) |
| W2 | A2 one draw key, A5 cold debug blocks out of nd_raw_native | same, plus wcen_mac01 prof=4 gbi phase | done: byte-equal; nd_raw_native 8,996 to 6,332 B, close path 19,288 to 16,832 B; TevKey built only for ndref and drawcensus; nd_tmpl_build repeats the last key with no hash |
| W3 | A3, A4 pm5 through the shadow | same | done: byte-equal (battle_start_f182 has pm5 draws); the shadow resets only at a program change (prog_use); pm5 uniforms are marked dirty only when they differ from citro3d's copy (u5set, u5mtx); no palk copy for pm5. The 588 B d5 copy stays (nd_emit needs no_uni and loc_uvx) |
| W4 | B1 ring reuse at 30 fps | same (a ring wrap scenario if one exists, else code trace) | done: byte-equal; no scenario wraps the ring, so the proof is the code trace: C3D_FrameBegin without NONBLOCK calls gxCmdQueueWait(-1) (libcitro3d renderqueue.o disassembly), so ring_done = ring_fr at every begin |
| W5 | A7, A8, A11 gtrip and gappend lookups | same | done for A7 and the palette part of A8: byte-equal; one palette search per vertex (pal_lookup), wide check once per triangle. The parameter memcmp of A8 stays (gtrip has one caller, a later epoch id is a small gain). A11 not done: t1_fold_tri runs only for a TEXEL1 window fold (ep_t1f), and its uv feeds a floor, so the other rounding order could move a fold |
| W6 | A10, A12, C5 combine hash and switch macros | same | done: byte-equal; PMD removed (PMDL everywhere, it follows option changes); the per-frame and per-kick lookups use PMDL; the inline sc_sig copy calls sc_sig_now. The two combine hashes stay: scm_sig misses keyc, keys and conv_k4, so a merged key or a hash-indexed memo would change which entry replays |
| W7 | A6 texture slot index | same | done: byte-equal; a lookup miss walks one chain of a 2,048-bucket index (hb_head, hb_next) instead of every slot below slot_hi. The CI palette hash per call stays (the key needs it) |
| W8 | B2, B3 seams (pixels change) | before and after shot of the subject, byte-equal elsewhere | not done. B2: from the matrices (Mtx_OrthoTilt row 2 = (0, 0, 1, -1), from the libcitro3d disassembly, and the view row 2 = (0, 0, 0.5, 0.5), pm_gpu.c rebuild_3d), a normal triangle and the prim-depth rectangle both give depth 1 - p/32767, while the shader zmix path gives -nd_prim_ndc_z(p) = 1 - 2(p-1)/32767. The pond blocker (p 0x7FF0) draws in Azahar, so the zmix path must be checked by a depth readback (pm_gpu_depth_at) at toad_pond before any edit. B3: s and t stay below 7.9 texture repeats in the texrects of the det scenes (sc 4096), so no shot can show the change; edit only with a scene that tiles past 7.9 |
| W9 | A9 combine fact owner, B4 decoder merge | same as W1 | B4 done (nd_words masks prim depth like the walker). A9 not done: moving the plan owner touches nd_comb, pm_gpu and nd_tmpl together, and the gain is one 192-byte compare per plan change; it needs its own row |
| W10 | C3, C8, C9 (structure); C4 and the stereo double walk wait for the user | ask first (user 2026-11-08: C4 yes, C8 yes, stereo later) | C3 and C9 done (byte-equal). pm4.v.pica now declares uvso, cmul, cadd, zmix in a row: nd_emit_draw writes them as one block, and the old shared DVLB had kept them adjacent only by chance. gv4_on now checks the layout at init. C8, C4 and the stereo walk wait for the user |

Speed check (Azahar, direction only): wcen_mac01 gbi phase avg/p95 is 5.9/15.0 ms on base 53e5a553e (2 runs) and 5.9/14.9 ms on W1-W10 (3 runs), with walker_ops 595 on both builds. Azahar has no cache model, so the cut in per-batch work and data does not show there. Open: one console_prof pair (base vs W1-W10) on the O3DS at spot_mac01_prof needs the user.

C4 (user decision, D9): 61 of the 64 words that no scenario, bench, open row or plan names are deleted with the code they gated. Kept: depthview (a key byte of NdTmplKey, the ndref TevKey format and 4 host tests replay it), ptrcheck (the PTRCHECK macro sits in src/msg.c, a file with the user's uncommitted edits), and the new CORE2 audiohash path. Gates: build, nd_host PASS, spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det 0000-0015 byte-equal to base b234c3b3d; pause_spin_grab differs only in the known grab block (x 115, y 33 to 40).

B2 result: the shader zmix path took the N64 NDC value 2(p-1)/32767-1 as the PICA clip z / w. The full palette (OrthoTilt row 2 = z - w after the view row 0.5 z + 0.5 w) needs (p-1)/32767-1, the value the rectangle path already gives (depth 1 - p/32767). Every G_ZS_PRIM triangle with p above 16384 was outside the clip volume: the mac_00 pond blocker (prim 0x7FF0, Z_CMP, colour only; wdraw zs 1 in pond_mac00_wlog) never drew, and the pond showed the hedge behind it. After nd_prim_clip_z the shot pond_mac00_wlog frame 204 shows the pond colour (rows 94 to 400 of the left 38 columns, 8,340 pixels change; build3ds/audit_shots/b2_pond_before_after.png). The 4 byte gates are unchanged (none has a G_ZS_PRIM triangle). Console check of the pond is open (POND row).

C8 result (D12, pure move): the bottom screen text (menu lines, HUD rows, footer, log frame: 180 lines) is pm_gpu_ui.c, with the shared state in pm_gpu_int.h; pm_gpu.c is 3,595 lines. The menu present, the grey pass and the shots stay in pm_gpu.c: they use target, gpu_ok, the stereo state and flush. The rectangle queue and the ring stay too: rect_raw_open alone writes about 30 batch variables of the close (gni, pal_n, pal_g, pal_gen, g_uvso, g_vp, g_cmul, g_cadd, g_zmix, g_ct, g_cullm, g_use5, rect_*, rq_*, gi_kind, draw_3d, pcomp_gen). A file split there needs a batch-state struct first, which changes code and is its own row.

A5 result: the batch close path is 13,424 B (flush_draw 1,496, nd_raw_native 5,072, nd_native_submit 2,048, nd_emit_draw 2,040, C3Di_UpdateContext 1,256, nd_tmpl_build 1,220, ndunit_after 200, GPUCMD_AddRawCommands 92), was 16,832 B. The decal pass A and zmark pass 3 of a raw batch, pm5_upload and pm5_upload_draw are out of line; t1log_draw, pm5tex1_log and stc_set are cold. 3dsx.ld.in places the close after the walker span in call order (map: 0x1000ac to 0x1066e8). Azahar gbi phase 5.9 ms before and after (no cache model); the I-cache effect needs the console pair.

## 5. Known open items (links only)

- Console regression a3c5ec093 -> 7be88729b (runs 25158, 25044): left open by the user.
- MENUB B1/B2: row_MENUB.md. UB1 suspects 2 and 3, and the mac_00 row-58 layout effect: row_UB1.md. UB1 suspect 1 (texture tile state): row_UB1.md.
- CFG and GREY: merged (61c952e64, ae8176ad6).

## 6. Not checked (reason)

- Audio DSP internals (pm_dspaud.c, pm_vadpcm.h): out of scope, and no lock finding led there.
- pm_input.c Select and stale rules, and pm_options_ui.c: one pass only, and MENUB owns the open questions.
- TARGET_3DS producer blocks in src/ (49 files): checked only through the walker dispatch (nd_walk.c G_PM_* cases).
- 3ds/ld/3dsx.ld.in, and the output shape of gen_map_restore.py: no finding led there.
- Console cache behaviour: Azahar has no cache model. The A5 and A6 numbers are code and data sizes, not measured misses.
