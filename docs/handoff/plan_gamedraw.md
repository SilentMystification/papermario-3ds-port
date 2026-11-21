# plan_gamedraw.md: GAMEDRAW, less per-step work in the game's own draw code (oracle NATIVE18; user task 20 Nov: design, pages only)
Tags: FACT (console capture, Azahar log or code read), GUESS, OPEN. Rows GD0 to GD8, GDW, DO3F. plan_drawobj.md (oracle NATIVE17) owns DO0 to DO4: this plan does not change them.

## 0. Base numbers (FACT build3ds/out/live_cap1c_170.txt, O3DS .170, main 96307bf5fa, mac_01 0 0 200, prof, window f1589)
- fps 47.4, ft 41.46. Core 1 busy 32.33 ms (lgms 40.06 - bw 7.73), logic 2.40. Timed recorder: snap 2.12, rmdl 5.22, rnr 3.70, alt 1.86, rent 1.02.
- c1g (exclusive, sampled steps): ui 7.98, task 5.19, mdl 0.24, wrk 0.11, ent 0.10, npc 0.02, ply 0, fx 0, thud 0. Walk: cls rest 5.87, nd 22.79.
- Per step: 693 units (rnr), 477 alt units, 76 snap builds (c1n). Per frame: 95 model records, 11 entity draws, 6 sprite draws, 33 texture rects (ndc).
- fps60 is on (FACT `disp w1 s0 f60`, kf 300). Capture words: plan_60fps.md section 5 (prof mode, never phase=on: pm_phase.c:839).

## 1. What the game does per step (FACT code read)
### 1.1 fps60 interpolation record (all classes)
- Each FI_OPEN calls pm_interp_open (3ds/src/pm_interp.c:59-67). It scans all nodes of the step for the same (parent, a, b) (:63-64): O(n^2).
  392 nodes per step mean, 448 peak (FACT Azahar mac_00, build3ds/azt/logs/mapchange_hud_after.txt). About 77K compares per step.
- Callers: active models (model.c:2250), render tasks (model.c:4260), entities (entity.c:356), sprite components (sprite.c:510), effects (effects.c:128),
  HUD elements (hud_element.c:2003). Cost GUESS 1.5 to 3.5 ms. GD1 (gated in its worktree, f12805d958).
### 1.2 Models, entities, player, NPCs (c1g mdl 0.24, ent 0.10, ply 0, npc 0.02: FACT small)
- A static model builds no matrix (model.c:2267-2268 finalMtx = savedMtx). Cull and depth per model: model.c:2390-2405, :2525. Entities: entity.c:326-412,
  entity_model.c:517-553. Player: 77480.c:1530-1590. No row: GD3 (cull memo) and GD5 (sprite matrix memo) are dropped by these numbers.
### 1.3 execute_render_tasks (c1g task 5.19 = exclusive: the time less rmdl, rent, rspr, rnr, snap, alt, lsc inside it)
- Sort: 3 exchange sorts, O(n^2), n about 110 (model.c:4171-4222): GUESS 0.2 to 0.4. One FI_OPEN per task (:4260), each a full node scan: part of GD1.
- appendGfx code: record fill (model.c:954-1508), group model copies (model.c:2611, :2622), entity setters (entity_model.c:356-465).
- Untimed recorder in it: nd_xf_push and nd_xf_pop (nd_xf.c xf_rec: nd_ds_unit decode plus a record). GD0B times it (rxf).
- Timer cost: a sampled step runs about 1385 child timer pairs (FACT counts: 693 rnr, 477 alt, 95 rmdl, 76 snap, 27 lsc, 17 rent and rspr). One pair
  costs 1.15 us inside (cal 307 ticks); a similar part falls into the parent class. GUESS 0.7 to 1.5 ms of task is timer cost, not game work.
- Rest GUESS 2 to 3 ms: game code plus I-cache refills after each recorder call (core2 ic1 67387 now, 33606 in the old build, FACT).
### 1.4 ui (c1g ui 7.98): mostly not UI work
- In a sampled step the logic runs inline (pm_phase.c:715 pm_lg_quiesce). nuGfxTaskStart (main_loop.c:291) is inside the ui region (:239-294). It pushes the
  frame: pm_wq_frame (nd_wq.c:148) is not on the logic thread, so wq_main_direct (:155-156) walks the frame there. The walk's untimed part (cls rest 5.87)
  lands in ui. UI work GUESS 7.98 - 5.87 = about 2.1 ms. In a normal step core 1 waits there instead (pm_wq_frame_slot, nd_wq.c:162-163; fw 6.99 ms).
- UI work in mac_01 idle: the status bar. state_drawUI_world (state_world.c:21-25) -> draw_status_ui -> update_status_bar (inventory.c:571): 2 draw_box
  windows (:756-757), about 12 hud_element_draw_next and 2 status_bar_draw_number (:782-901). Each HUD element: hud_element_draw_rect (hud_element.c:637):
  tile loop with double math (:761, :796, :861, :870), hud_rect_direct (:472), hud_rect_emit (:285, a unit buffer), nd_snap_pso and nd_rec_alt (:873-879).
  Messages and windows: none in mac_01 idle (GUESS). render_hud_elements_backUI and frontUI (:1454, :1578): a scan of all slots and a small sort.
- Unchanged between steps while the bar stands: every NdHudRec (flags, image, palette, scales, pieces), the prim and env words, so every HUD rect record.

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

## 3. Rows, ranked (mac_01, fps60 on; all ms GUESS)
| # | row | model | what | needs | gate | ms |
|---|---|---|---|---|---|---|
| 1 | GDW | Haiku | census c0i: idle, pace, gx waits of the game thread | none | spot_mac01_fps60_det, capture_check | 0 (core 0 work known) |
| 2 | DO3F | Haiku | uni_native with the key patch: K values by nd_kp_vec as kp_scan, no early return | none | spot_mac01_fps60_stereo_det, spot_mac01_fps60_det | core 0 -0.6 to -0.9 |
| 3 | GD8 | Sonnet | double buffer rtab and repl (pm_interp.c), so pm_wq_frame_slot need not wait for the walker | GDW (core 0 work under 33 ms) | 2 fps60 det benches | ft 0 to -5 |
| 4 | GD7 | Sonnet | status bar draw memo: draw_box, HUD rect, number copy their last stream range | user, NATIVE17 | hudfb_drec, spot_mac01_drec | core 1 -2 to -4 |
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
- DO5 (skip the model record fill): dropped by NATIVE17 (0b7a5aea85).

## 5. Memory and cache
- Static margin 402 KB (FACT `mem boot` line). Heap free 3080 KB after the pause picture (FACT live_cap1_170.txt:5). Every GD table is heap: 0 static.
- Heap: GD1 +6 KB, GD2 +2 KB, GD4 +4 KB, GD6 50 to 100 KB, GD7 about 80 KB, GD8 +80 KB (GUESS). Total under 280 KB heap.
- L1: GD1 replaces a 7.8 KB scan per open by 1 to 2 lines. GD2 compares a 1 KB key array. GD7 reads one key and copies one block per hit (sequential).

## 6. Order and dependencies
1. GDW and DO3F now (Haiku, disjoint files: nd_wq.c, pm_gpu.c frame begin, pm_phase.c; pm_gpu.c uni_native). Then a capture (plan_60fps.md section 5 words).
2. GD8 page when the GDW line shows core 0 work under 33 ms per step. GD7 on the user's Sonnet yes and NATIVE17's yes. GD2 any time (model.c only).

## 7. Risks and fallbacks
- GD1: the iocc host test checks ids against the old scan (both caps in). Fallback: a per-parent chain, about half the gain.
- GD2: moving NPCs can make the memo miss every step. Fallback: keep only the key array (exact, about half the gain).
- GD7: a missed key field or side effect gives a stale draw. The census lists it (3 rounds per site); a site that keeps a diff stays on the full path.
- GD8: the walker reads the wrong set: the fps60 det benches show it. Fallback: keep the lock step.
- DO3F: a wrong K patch shows in the stereo fps60 bench. Fallback: keep the early return (DO3 as now).
- GD6: a block whose output depends on state it does not compare: that block id stays on the unit path.
- Behaviour: every row is an exact cache of a pure function of the game state. Tables reset at a map load.
