# plan_gamedraw.md: GAMEDRAW, less per-step work in the game's own draw code (oracle NATIVE18; user task 20 Nov: design, pages only)
Tags: FACT (console capture, Azahar log or code read), GUESS, OPEN. Rows GD0 to GD9, GDW, DO3F. plan_drawobj.md (oracle NATIVE17) owns DO0 to DO4: this plan does not change them.

## 0. Base numbers (FACT build3ds/out/live_cap1c_170.txt, O3DS .170, main 96307bf5fa, mac_01 0 0 200, prof, window f1589)
- fps 47.4, ft 41.46. Core 1 busy 32.33 ms (lgms 40.06 - bw 7.73), logic 2.40. Timed recorder: snap 2.12, rmdl 5.22, rnr 3.70, alt 1.86, rent 1.02.
- c1g (exclusive, sampled steps): ui 7.98, task 5.19, mdl 0.24, wrk 0.11, ent 0.10, npc 0.02, ply 0, fx 0, thud 0. Walk: cls rest 5.87, nd 22.79.
- Per step: 693 units (rnr), 477 alt units, 76 snap builds (c1n). Per frame: 95 model records, 11 entity draws, 6 sprite draws, 33 texture rects (ndc).
- fps60 is on (FACT `disp w1 s0 f60`, kf 300). Capture words: plan_60fps.md section 5 (prof mode, never phase=on: pm_phase.c:839).

## 1. What the game does per step (FACT code read)
### 1.1 fps60 record and small classes: GD1 (merged) removed the O(n^2) node scan (pm_interp.c:59-67). c1g mdl, ent, ply, npc are small (FACT).
### 1.3 execute_render_tasks (c1g task 5.19, exclusive): sort O(n^2) (model.c:4171-4222, GUESS 0.2 to 0.4), appendGfx code, xf records (rxf), the
  timer cost of sampled steps (about 1385 child pairs per step, GUESS 0.7 to 1.5 ms), I-cache refills (ic1 67387 now, 33606 old, FACT).
### 1.4 ui (c1g ui 7.98): mostly the inline frame walk of sampled steps (nuGfxTaskStart inside the ui region, main_loop.c:291; nd_wq.c:155-156).
  Real UI work is the status bar: update_status_bar (inventory.c:571): 2 draw_box (:756-757), about 12 HUD rects, 2 numbers (:782-901).

## 1.5 Capture 2 (FACT live_cap2_170.txt f11489, main 353b31c792 = capture 1 build + GD1, GD0B, DO3) and core 0
- ft 41.21 (41.46), core 1 busy 28.59 (32.33: GD1), bw 11.22 (7.73), wk 38.00 (33.01). Pace iv 2/3/4+ vblanks: 178/96/26 (172/104/24). Walk (sampled) nd 23.42 (22.79).
- c1G mode 5.50 ms, c1u mode 357 units, c1v mode 24, c1x mode 1.19: the status bar (2 draw_box, HUD rects, numbers). task 13.43 incl., 224 units, 419 alt.
- wk is not core 0 work: it spans pm_main.c steps up to :760 with the game thread's waits for the logic (pm_logic.c:166, :173 and others; wwait counts
  only :153), sync60_pace and the GX wait of C3D_FrameBegin (pm_gpu.c:2989-2991). GDW measures them. Capture 2 windows are at f11489, capture 1 at f1589.
- DO3 is inert with fps60: pm_gpu_twin_begin sets nd_kp_on = 1 each L frame (pm_gpu.c:198), uni_native returns at :1329, so fv stays 111 and upd 1.05 ms.
  DO3 adds no core 0 work in that case (one store and a branch per draw): the wk rise is not DO3 (GUESS: moved wait time; GDW decides). sub 4.20 + upd 1.05
  + emit 1.15 = 6.40 against sub 5.98: the split timers of DO3 step 1. Keep DO3; DO3F makes it act with the key patch.
- The step is a pipeline with a lock step: core 1 waits in pm_wq_frame_slot (nd_wq.c:162-163) until core 0 has walked the L and K frames of the step before,
  because pm_interp_lerp rewrites rtab and repl that the walker reads. So a faster core 1 (GD1) did not move ft; core 0 work and this lock set the period.

## 1.6 Status bar on core 0 (FACT NATIVE20, live_play2_170.txt f10494 hidden against f11094 shown, build a05f62c545)
- Shown bar: core 0 gbi +4.15 ms (rest +2.78, raw +0.85, sub +0.50), draws 95 -> 121 per frame (about 160 us per draw), c0i gx +2.14 ms (GPU), ft +1.23.
- Same every step while it stands: the 2 draw_box windows, about 12 HUD rects and the digits. Dynamic: digits and icons when coins, HP, FP or star
  points change; the slide position while it opens or closes (inventory.c:737-746); the matrices are per context (pointer) but equal in content.
- GD9 = a retained core 0 command block (the model-cache-hit idea of DO1, one level up). GD9A (census): core 1 brackets draw_status_ui (state_world.c:22)
  with 2 NAPI records; the end record carries a key = FNV of the recorder inputs inside the bracket (unit words; matrix and vertex contents, not their
  pointers). Core 0 hashes its shadow before and the emitted words after (address registers out, as gst). Line `ublk`: key, st, w, bad.
- GD9B (replay, after GD9A shows bad 0 and key near n): at the begin record, flush citro3d and reset the native shadow (both paths), so the block starts
  from a known state. Miss: walk, then store the words, the vertex bytes (copied to a persistent linear buffer, attribute addresses patched), the shadow
  after and the citro3d values the block changed (scissor, viewport: a census lists them). Hit: copy the words (no split inside: else miss), restore
  the after state, skip to the end record. Invalidate: map load, texture cache eviction, pm_wide, stereo change. First step after a change: a miss
  (the full walk, as today), so a coin count animation costs as today.
- Memory (GUESS): words 26 draws x 60 to 100 = 6 to 10 KB heap, vertices 2 to 4 KB linear, one entry per site: under 32 KB, static 0 (margin 378 KB FACT).
- Gain (GUESS): core 0 -3 to -4 ms per step with the bar standing; GPU gx unchanged (the same words); ft -1 ms when core 0 is the limit.
- Not chosen: the bar drawn once to a texture and shown as one quad (GPU -2 ms too), because blending 26 layers in a texture first is not byte-exact
  (user decision). GD7 (core 1 memo) moves ft by about 0: core 1 is off the critical chain (plan_critpath.md).

## 3. Rows, ranked (mac_01, fps60 on; all ms GUESS)
| # | row | model | what | needs | gate | ms |
|---|---|---|---|---|---|---|
| 1 | GDW | Haiku | census c0i: idle, pace, gx waits of the game thread | none | spot_mac01_fps60_det, capture_check | 0 (core 0 work known) |
| 2 | DO3F | Haiku | uni_native with the key patch: K values by nd_kp_vec as kp_scan, no early return | none | spot_mac01_fps60_stereo_det, spot_mac01_fps60_det | core 0 -0.6 to -0.9 |
| 3 | GD8 | Sonnet | double buffer rtab and repl (pm_interp.c), so pm_wq_frame_slot need not wait for the walker | GDW (core 0 work under 33 ms) | 2 fps60 det benches | ft 0 to -5 |
| 4 | GD9A | Haiku | status bar block census (key, state, words) | none | hudfb_drec (drec, gst), capture_check | 0 |
| 4b | GD9B | Sonnet | status bar core 0 block replay | GD9A bad 0 | hudfb_drec, spot_mac01_drec; drec-frame compare `ublk diff 0` | core 0 -3 to -4 |
| 4c | GD7 | Sonnet | status bar draw memo (core 1): after GD9B | user, NATIVE17 | hudfb_drec, spot_mac01_drec | core 1 -2 to -4 |
| 5 | GD6 | Sonnet | setter blocks (camera, entity, sprite setup) replayed by the recorder | c1u (NATIVE17: 4 conditions) | 4 drec benches | core 1 -1 to -2.5 |
| 6 | GD2 | Haiku | render task sort on a key array, memo of the order | none | 2 drec benches, `gdsort bad 0` | core 1 -0.2 to -0.4 |
| 7 | GD4 | Haiku | group model child table | task split | 2 drec benches | core 1 0 to -0.3 |
Done: GD1 (core 1 -3.7 FACT), GD0, GD0B. Dropped: GD3, GD5 (c1g mdl, ply, npc small), DO5.
- Core 1 rows (4 to 7) move ft only after core 0 work and the lock step (rows 2, 3) fit the 33.3 ms step. Order: 1, 2, then 3 by the GDW line, then 4 to 7.
- Why Sonnet: GD7 and GD6 need every input of a draw in the memo key (a census loop, as DO1); GD8 changes when the walker reads a table that the logic writes.
- GD8 design: two rtab and repl sets indexed by the step parity; pm_interp_find reads the set of the frame it walks (PmFrame flags carry the parity);
  pm_wq_frame_slot then waits only for the frame 2 steps back. +80 KB heap. The K frame words stay equal (same lerp values).

## 4. Change detection (the retained form)
- No row adds a dirty flag or a version stamp to the game structs (model->flags, fog, tint and HUD fields have many writers). Each row compares its inputs.
- GD2: input = task count and the dist of each task in task order; the 3 passes are a pure function of it. bat1_sort_runs (:4224) always runs.
- GD4: the render mode property and the texture id of a group child are map data: fill once per map load (reset at clear_render_tasks, model.c:4030-4047).
- GD7: key = the call arguments (draw_box), the NdHudRec (hud_element.c:271-281), plus the NdDs before; the hit repeats side effects (matrix stack slots,
  gMatrixListPos: draw_box.c:472, :497, :789). Census first (ws_GD7 step 1). No edit of nd_rec_model or nd_rec_alt.
- GD6 conditions (oracle NATIVE17, plan_drawobj.md section 6): (1) same stream bytes and nd_ds after the block as the unit path (`gdblk diff 0`);
  (2) state setters only, no TRI or RECT unit; (3) the entry compare holds sc_lost, sc_dirty and the stream shadow; (4) no CALL or model record inside;
  (5) no nd_rec_model edit. A HUD rect holds a RECT, so it is GD7, not GD6.

## 5. Memory and cache
- Static margin 378 KB (FACT, coordinator 20 Nov; was 402 KB). Heap free 3080 KB after the pause picture (FACT live_cap1_170.txt:5). Every GD table is heap: 0 static.
- Heap: GD1 +6 KB, GD2 +2 KB, GD4 +4 KB, GD6 50 to 100 KB, GD7 about 80 KB, GD8 +80 KB, GD9 under 32 KB (GUESS). Total under 280 KB heap.
- L1: GD1 replaces a 7.8 KB scan per open by 1 to 2 lines. GD2 compares a 1 KB key array. GD7 reads one key and copies one block per hit (sequential).

## 6. Order and dependencies
1. GD9A gated (8785b78c74): merge it. GD9A2 (census 2: steady window, dup walks, nd_ds entry hash, first differing unit) next. GDW and DO3F are merged. GD8 page when the GDW line shows core 0 work under 33 ms per step.
2. GD9B page after the GD9A line (bad 0, key near n). GD7 and GD6 after GD9B (core 1 rows), on the user's Sonnet yes and NATIVE17's yes.

## 7. Risks and fallbacks
- GD1: the iocc host test checks ids against the old scan (both caps in). Fallback: a per-parent chain, about half the gain.
- GD2: moving NPCs can make the memo miss every step. Fallback: keep only the key array (exact, about half the gain).
- GD7: a missed key field or side effect gives a stale draw. The census lists it (3 rounds per site); a site that keeps a diff stays on the full path.
- GD8: the walker reads the wrong set: the fps60 det benches show it. Fallback: keep the lock step.
- DO3F: a wrong K patch shows in the stereo fps60 bench. Fallback: keep the early return (DO3 as now).
- GD6: a block whose output depends on state it does not compare: that block id stays on the unit path.
- Behaviour: every row is an exact cache of a pure function of the game state. Tables reset at a map load.
