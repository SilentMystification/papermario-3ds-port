# plan_gamedraw.md: GAMEDRAW, less per-step work in the game's own draw code (oracle NATIVE18; user task 20 Nov: design, pages only)
Tags: FACT (console capture, Azahar log or code read), GUESS, OPEN. Rows GD0 to GD6. plan_drawobj.md (oracle NATIVE17) owns DO0 to DO5: this plan does not change them.

## 0. Base numbers
- Console build 7680, O3DS .170, mac_01 0 0 200 (FACT live_prof_170.txt f1289): core 1 busy 35.2 ms per step, logic 2.40, timed recorder 15.97
  (snap 4.78, rmdl 4.33, rnr 3.65, alt 1.85, rent 1.07, rspr 0.23, lsc 0.06), 693 units per step. Per frame: 95 model records, 11 entity draws, 6 sprite draws (ndc).
- fps60 is on in both console captures (FACT `disp w1 s0 f60`: live_prof_170.txt:1399, live_old_170.txt:93; kf 300 per window).
- Capture 1 (FACT live_cap1_170.txt; cap1b is the same run, its push failed): s0 and ser 300 in every window, all timers 0.00, ft 70.1. Phase mode
  starts no sampled step (pm_phase.c:839 pm_pf_arm = pm_pf_on && !ph_want), so c1g has no split yet. Rows marked "c1g" wait for a prof-mode capture.
- Old display list build 3542: core 1 busy 12.1 ms (FACT). The "9.7 ms game draw" of plan_60fps.md is that total less logic (GUESS). It holds the fps60
  record (1.1), the old word stores and the game walk.

## 1. What the game does per step (FACT code read) and what stays the same in mac_01
### 1.1 fps60 interpolation record (all subsystems)
- Each FI_OPEN calls pm_interp_open (3ds/src/pm_interp.c:59-67). It scans all nodes of the step for the same (parent, a, b) (:63-64): O(n^2).
- 392 nodes per step mean, 448 peak in mac_00 (FACT Azahar, build3ds/azt/logs/mapchange_hud_after.txt `f06 rec`). About 77K compares per step.
- Callers: each active model (model.c:2250), render task (model.c:4260), entity (entity.c:356), sprite component (sprite.c:510), effect (effects.c:128),
  HUD element (hud_element.c:2003). The cost spreads over all c1g classes and the untimed rest.
- Cost GUESS 1.5 to 3.5 ms core 1 per step (a 7.8 KB node table read 392 times). Fix GD1: a hash table (parent, a, b) -> last node. Same node ids.
### 1.2 Models (95 records per frame)
- mdl_update_transform_matrices (model.c:2236-2365, called at cam_main.c:122, outside c1g): a static model builds no matrix (finalMtx = savedMtx,
  :2267-2268). A changed model builds one (:2276-2303) and copies it for 2 steps (:2253-2266). The game already keeps the matrix: no row.
- render_models (model.c:2367-2582): per model a bounds cull (up to 8 points, 1 divide each, :2390-2405), a depth transform (:2525), a task (:2540).
  Same in mac_01 while the camera stands: camera matrix, model centers, flags.
- execute_render_tasks (model.c:4150-4273): 3 exchange sorts, O(n^2), n about 110 tasks GUESS (:4171-4222), then one appendGfx per task.
- appendGfx_model (model.c:820-1511): fills a 14-pair record (GUESS 1 us) and calls nd_rec_model (:1509; its cost is rmdl). A group model copies a Model
  by value per child (model.c:2611) and scans the property list per child (:2622).
### 1.3 Entities (11 draws, FACT ndc)
- render_entities (entity.c:326-412), draw_entity_model_A (entity_model.c:517-553): a distance test, guMtxL2F, transform_point, a task.
- appendGfx_entity_model (entity_model.c:336-515): about 8 setter units with the same values each step (:356-465), then nd_rec_ent_draw (:469).
  Game part GUESS under 0.1 ms. The units are part of rnr.
### 1.4 Player and NPCs (6 sprite draws, FACT ndc)
- appendGfx_player (77480.c:1530-1590): 4 guRotateF, 3 guTranslateF, 1 guScaleF, 7 guMtxCatF per step. spr_draw_component (sprite.c:501-542) per
  component: spr_transform_point and the component matrix chain. render_npcs (npc.c:963): a transform and a divide per NPC. GUESS 0.05 to 0.3 ms.
### 1.5 Effects, HUD, UI, camera
- render_effects_scene (effects.c:121): effects animate each step. No retained gain.
- render_transformed_hud_elements (hud_element.c:1935): an exchange sort (:1981-1992). Status bar, windows, messages in mac_01: OPEN.
- Camera setup (cam_main.c:190-210): 21 setter units per camera per step with the same values each step.

## 2. Where core 1 time goes
- The game walk (cull, sort, record fill, player and NPC matrices) is small by code read: GUESS 1 to 3 ms. Static models already keep their matrix.
- The large part is the recorder below the game code (FACT 15.97 ms timed, GUESS 8.7 ms untimed). Each unit is decoded and recorded where the old
  display list stored 2 words. DO1 to DO4 and GD6 address it.
- The largest game-side cost is the fps60 record scan (GD1). It runs every step with fps60 on (the 60 fps target).
- The native path removed the core 0 interpreter (core 0 47.9 -> 30.7 ms FACT). Core 1 grew (12.1 -> 35.2 ms FACT) by the recorder. A retained game walk
  alone does not reverse that. GD1 and the recorder memo rows (DO1, DO4, GD6) do most of it.

## 3. Rows (core 1 ms per step, mac_01, fps60 on; all ms GUESS)
| row | model | what | needs | gate | core 1 |
|---|---|---|---|---|---|
| GD1 | Haiku | pm_interp_open: (parent, a, b) hash table, same node ids | none | host iocc (seeded), spot_mac01_fps60_det byte-equal | -1.5 to -3.5 |
| GD0 | Haiku | census: c1G (class time with recorder work in) and c1u (units per class) | none | spot_mac01_drec byte-equal, capture_check | 0 |
| GD2 | Haiku | execute_render_tasks: sort on a local key array; memo of the permutation when all dist equal last step | c1g task > 0.5 | 2 drec benches, drec self-check `gdsort bad 0` | -0.1 to -0.4 |
| GD3 | Haiku | render_models: cull and depth memo per model (camera matrix bytes, center, flags equal) | c1g mdl > 0.4 | 2 drec benches, `gdcull bad 0` | -0.1 to -0.3 still camera, 0 moving |
| GD4 | Haiku | group models: child render mode and texture id from a per-map table, no Model copy | c1g task after GD2 > 0.5 | 2 drec benches | 0 to -0.3 |
| GD5 | Haiku | player and sprite component matrix memo (same float inputs: reuse the Mtx) | c1g ply + npc > 0.3 | spot_mac01_drec, entcape_kmr03_drec | -0.05 to -0.3 |
| GD6 | Sonnet | fixed setter blocks (camera, entity, sprite, HUD setup) get a block id; the recorder replays a block with equal words and entry state | GD0 c1u (NATIVE17 agreed, 4 conditions) | 4 drec benches, census `gdblk diff 0` | -1 to -2.5 |
Total GD1 to GD6: -2.4 to -7.3 ms core 1 (GUESS), on top of DO1 to DO4 (core 1 -3.5 to -5, plan_drawobj.md). Core 0: 0 (GD rows keep the stream bytes).

## 4. Change detection (the retained form)
- No row adds a dirty flag or a version stamp to the game structs. model->flags and the fog, tint and panner globals have many writers (evt API, model.c);
  a missed writer is a stale draw. Each row compares its inputs directly: for these small inputs that costs about one flag test, and no writer is missed.
- GD2: input = task count and the dist of each task in task order. The 3 sort passes are a pure function of it. bat1_sort_runs (:4224) always runs.
- GD3: the 64 B of camera->mtxPerspective and pm_wide, per model the center, the DO_BOUNDS_CULLING bit and the bounding box pointer. Equal: reuse the
  visible bit and the distance. Reset at clear_render_tasks (model.c:4030-4047, map load).
- GD4: the render mode property and the texture id of a group child are map data (shape file, gCurrentModelTreeNodeInfo): fill once per map load.
- GD5: per component the float inputs of spr_transform_point and the matrix chain. GD6: the recorder compares block words and entry state (as DO1).
- GD6 conditions (agreed with oracle NATIVE17, plan_drawobj.md section 6): (1) a replayed block gives the same stream bytes and the same nd_ds after it as
  the unit path (census `gdblk diff 0`); (2) a block holds state setters only, no TRI or RECT unit; (3) the entry state compare includes sc_lost, sc_dirty
  and the stream shadow; (4) a block never spans a CALL or a model record. GD6 does not edit nd_rec_model (DO1 works there). ws_GD6 states all 5.
- DO5 (skip the record fill, plan_drawobj.md): drop. The fill is GUESS 1 us per model; a check of its 12 inputs costs the same.

## 5. Memory and cache
- Static margin 402 KB (FACT `mem boot` line). Heap free 3080 KB after the pause picture (FACT live_cap1_170.txt:5). Every GD table is heap: 0 static.
- Heap: GD1 +6 KB (inside the fps60 block), GD2 +2 KB, GD3 +4 KB, GD4 +4 KB, GD5 +2 KB, GD6 50 to 100 KB (GUESS). Total under 120 KB.
- L1: GD1 replaces a 7.8 KB scan per open by 1 to 2 lines. GD2 compares a 1 KB key array (no 2 indirect loads per compare). GD3 reads 16 B per model
  in the order of the model loop. GD6 copies one block per hit (sequential).

## 6. Order and dependencies
1. GD1 and GD0 now (no c1g dependency): ws_GD1.md, ws_GD0.md. Different files: they run in parallel.
2. A console capture in prof mode (plan_60fps.md section 5 words) after GD0 merges. Then GD2, GD3, GD4, GD5 by the thresholds in section 3.
3. GD6 after GD0 names the classes with the most units. Recorder side agreed by oracle NATIVE17 under the 4 conditions of section 4.

## 7. Risks and fallbacks
- GD1: an occurrence count that differs changes the key frame pairs. The iocc host test checks ids against the old scan (overflow and depth caps in).
  Fallback: keep the scan and stop at the first node of the same parent (a per-parent chain), about half the gain.
- GD2: in mac_01 moving NPCs change dist every step, so the memo can miss always. Fallback: keep only the key array (exact, about half the gain).
- GD3: a moving camera gives no gain. Fallback: delete the memo (a pure deletion).
- GD6: a block whose output depends on state it does not compare. The census lists it; fallback: that block id stays on the unit path.
- Behaviour: every row is an exact cache of a pure function of the game state. The drec benches prove the stream bytes. Stale data after a map load:
  each table resets at clear_render_tasks (model.c:4030-4047).
