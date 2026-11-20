# plan_gamedraw.md: GAMEDRAW, less per-step work in the game's own draw code (oracle NATIVE18; user task 20 Nov: design, pages only)
Tags: FACT (console capture, Azahar log or code read), GUESS, OPEN. Rows GD0 to GD7. plan_drawobj.md (oracle NATIVE17) owns DO0 to DO4: this plan does not change them.

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

## 2. Where core 1 time goes
- The game walk itself is small (FACT c1g mdl, ent, ply, npc). Its big costs are the fps60 node scan (GD1), the HUD rect build (GD7) and the
  timer cost of sampled steps (not paid in normal steps).
- The large part is the recorder below the game code: DO1 to DO4 (plan_drawobj.md), GD6 and GD7 remove repeated records of unchanged objects.

## 3. Rows (core 1 ms per step, mac_01, fps60 on; all ms GUESS)
| row | model | what | needs | gate | core 1 |
|---|---|---|---|---|---|
| GD1 | Haiku | pm_interp_open: (parent, a, b) hash table, same node ids | none | host iocc (seeded), spot_mac01_fps60_det byte-equal | -1.5 to -3.5 |
| GD0 | Haiku | census c1G, c1u (merged ff6e9cc1ea) | none | done | 0 |
| GD0B | Haiku | census 2: frame push out of ui (gsub), ui split (hud, mode, win), rxf timer, c1v alt units per class | none | hudfb_drec byte-equal, capture_check | 0 (ui reads about 2 ms) |
| GD7 | Sonnet | HUD rect record memo: an equal NdHudRec plus equal entry state copies its last stream block and skips direct, emit, snap, alt | GD0B c1x mode, c1v mode; NATIVE17 (alt) | hudfb_drec, spot_mac01_drec byte-equal; census `hmemo diff 0` | -0.8 to -2 |
| GD2 | Haiku | execute_render_tasks: sort on a local key array; memo of the order when all dist equal | c1g task > 0.5 (FACT met) | 2 drec benches, self-check `gdsort bad 0` | -0.2 to -0.4 |
| GD4 | Haiku | group models: child render mode and texture id from a per-map table, no Model copy | GD0B task split | 2 drec benches | 0 to -0.3 |
| GD6 | Sonnet | fixed setter blocks (camera, entity, sprite setup) replayed by the recorder | GD0 c1u (NATIVE17 agreed, 4 conditions) | 4 drec benches, census `gdblk diff 0` | -1 to -2.5 |
Total GD1, GD2, GD4, GD6, GD7: -3.5 to -8.7 ms core 1 (GUESS), on top of DO1 to DO4. Core 0: 0 (GD rows keep the stream bytes).
Why Sonnet (GD6, GD7): the memo key must hold every input of the record; the census names missing fields (a design loop, as DO1).

## 4. Change detection (the retained form)
- No row adds a dirty flag or a version stamp to the game structs (model->flags, fog, tint and HUD fields have many writers). Each row compares its inputs.
- GD2: input = task count and the dist of each task in task order; the 3 passes are a pure function of it. bat1_sort_runs (:4224) always runs.
- GD4: the render mode property and the texture id of a group child are map data: fill once per map load (reset at clear_render_tasks, model.c:4030-4047).
- GD7: key = the NdHudRec bytes (64 B plus 48 B per piece, hud_element.c:271-281), nd_ds.w.prim, nd_ds.w.envc, the snap memo state and the entry state
  of GD6 condition 3. Hit: copy the last block (the other context's stream stays intact, plan_drawobj.md DO1 fact), write the nd_ds after-values.
  Step 1 is a census beside the full path (as ws_DO1 step 1). GD7 edits hud_element.c and a new 3ds/src file; it does not edit nd_rec_model or nd_rec_alt.
- GD6 conditions (oracle NATIVE17, plan_drawobj.md section 6): (1) same stream bytes and nd_ds after the block as the unit path (`gdblk diff 0`);
  (2) state setters only, no TRI or RECT unit; (3) the entry compare holds sc_lost, sc_dirty and the stream shadow; (4) no CALL or model record inside;
  (5) no nd_rec_model edit. A HUD rect holds a RECT, so it is GD7, not GD6.
- DO5 (skip the model record fill): dropped by NATIVE17 (0b7a5aea85).

## 5. Memory and cache
- Static margin 402 KB (FACT `mem boot` line). Heap free 3080 KB after the pause picture (FACT live_cap1_170.txt:5). Every GD table is heap: 0 static.
- Heap: GD1 +6 KB, GD2 +2 KB, GD4 +4 KB, GD6 50 to 100 KB, GD7 32 entries x (key 400 B + block 1 KB) = about 45 KB (GUESS). Total under 160 KB.
- L1: GD1 replaces a 7.8 KB scan per open by 1 to 2 lines. GD2 compares a 1 KB key array. GD7 reads one key and copies one block per hit (sequential).

## 6. Order and dependencies
1. GD1 (gated in its worktree), GD0 (merged) and GD0B now. GD2 now (FACT c1g task 5.19 > 0.5; Haiku page after GD0B merges: it edits model.c only).
2. A prof-mode capture after GD0B and GD1 merge: c1x gives the real ui split; c1v mode gives the HUD alt units. Then GD7 (page ready on the user's
   Sonnet yes and NATIVE17's yes for the alt record copy), GD4 by the task split, GD6 by c1u.

## 7. Risks and fallbacks
- GD1: the iocc host test checks ids against the old scan (both caps in). Fallback: a per-parent chain, about half the gain.
- GD2: moving NPCs can make the memo miss every step. Fallback: keep only the key array (exact, about half the gain).
- GD7: a field the key misses gives a stale HUD rect. The census lists it (3 rounds), the hudfb_drec bench proves the bytes. Fallback: memo only the
  game side (skip hud_rect_direct and emit on an equal NdHudRec, record as now): about a third of the gain.
- GD6: a block whose output depends on state it does not compare: that block id stays on the unit path.
- Behaviour: every row is an exact cache of a pure function of the game state. Tables reset at a map load.
