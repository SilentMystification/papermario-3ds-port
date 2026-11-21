# plan_maptrans.md: map transitions without hitches (oracle NATIVE22; code read; edit in place)
Tags: FACT (log line or code read), GUESS, OPEN. Line numbers: main a79f8ce548. The user reads sections 2 to 7 before a mechanism row (MTX1 and up) starts.
Console data (FACT, xfer lines, mac_01 <-> mac_00): A = build3ds/out/push_o_20261120-230244.txt (build 7827, main b066452693, after ROMDR),
B = build3ds/out/live_play2_170.txt (main a05f62c545), C = build3ds/hw_tidal1_live.txt (16 Nov, before MAPLAG), D = build3ds/hw_xl_log.txt (11 Nov).

## 0. Result
1. The longest step of a map change is the load step: 240 to 266 ms (FACT A max). It runs under the black cover. 11 Nov it was 163 ms (FACT D).
2. The load step holds 3 blocks. Walker drains at each ROM read (rest 121 ms, GUESS mostly drains). Romfs file reads of the seam and .dlc
   files (seam 29 to 49 + dlcp 58 to 67 ms, FACT A xfer2). The drain before the load plus the audio join (au 25 to 49 ms, FACT A).
3. Visible steps: OPEN. The xfer line mixes covered and visible steps (fade 56 to 92 ms, o34 8 to 12, FACT A). MTX0 splits them by phase.
4. Root cause (mechanism): resource lifetime is kept by a blanket drain of the walker queue, not by the frame that last reads the resource.
   The load runs as one synchronous block on the logic thread. First-sight cache work runs inline in the first drawn frames.

## 1. Timeline of one map change (game code src/state_map_transitions.c; L = logic thread core 1, G = game thread core 0, P = prefetch thread core 1 prio 0x3F)
| # | stage | code | thread | blocks | ms |
|---|---|---|---|---|---|
| 1 | exit volume, exit script, GotoMap: set_game_mode(CHANGE_MAP) | pm_map.c:367-389: drain "mode", xfer begin, map_prefetch | L | 1 full drain | OPEN |
| 2 | P reads <map>_shape.nds (romfs), Yay0 of <map>_hit | pm_prefetch.c:39-48 | P | nothing (runs in L idle) | yay0 7.8 (FACT A) |
| 3 | CHANGE_MAP_INIT: fade out, rate 20: 13 steps, old map drawn | state_map_transitions.c:134-143, screen_overlays.c:460 | L, G | visible | OPEN |
| 4 | DELAY: draw off; 4 idle steps (the N64 wait for its RSP) | :144-149, :155-157; main_loop.c:194-197 no frame push | L | covered | small (GUESS) |
| 5 | load_map_by_IDs: drain "map", audio join, heap create, clears | pm_map.c:393-418, world.c:83-242 | L | covered | load 250 to 272 (FACT A) |
| 5a | map statics restore (MS01) | pm_mapdata.c:108-126 memcpy data, memset bss | L | - | OPEN (GUESS under 1) |
| 5b | shape (.nds take), hit, bg Yay0; seam and .dlc file reads | pm_map.c:136-154, nd_shape.c:177-188, :210-256 | L | FS waits | seam+dlcp 88 to 115 (FACT A) |
| 5c | texture copies: 1 to 2 dma_copy per texture, 8 KB chunks | model.c:1573, :1585, 43F0.c:432-447 | L | 1 drain per chunk | maxstep rom 115 reads (FACT A) |
| 6 | AWAIT_MAIN: main script: NPCs, entities, music | :171-183; 101b90_len_8f0.c:179, :201; entity.c:881 | L | covered, drains | OPEN |
| 7 | FADE_IN: draw on, cover held 3 steps (MAPLAG), 13 fade steps | :184-198, pm_maplag.h:6 | L, G | first sight | init 50 to 150 (FACT A) |
| 8 | first sight on G: model misses, tex misses, list compiles | nd_recs.c:190, pm_tex.c:1399, nd_shape.c:759 | G | warm 64 misses per covered frame (pm_xfer.c:160) | mdl 282, tex 53/25 ms, dlc 156/24 ms (FACT A) |
- Audio: control job and mixer on core 1 (pm_audio.c:118). The map song samples decode at first play on the render thread (BTLWIPE did this only for battles, pm_prefetch.c:9-11).
- The picture: while the draw is off, no frame is pushed (main_loop.c:194-197). The screen keeps the black fade frame. The watchdog limit is 10 s (pm_wdog.c:32).
- The N64 hides the same load the same way: fade out, draw off, 4 idle steps so the RSP ends, load, fade in. The port keeps this order (FACT code).

## 2. Root cause as mechanisms (FACT code read)
- M1 lifetime by blanket drain. Every nuPiReadRom outside audio calls pm_walk_drain (pm_os.c:500). pm_wq_idle waits until the queue is empty
  (nd_wq.c:178-183). The queue holds entries that read no game RAM: the step begin (pace and GPU wait, pm_gpu.c:3087, :3032, :3035), step ends, key frames,
  texture invalidations. Each ROM read posts an invalidation (pm_os.c:435 -> pm_tex.c:1325, no kick). So the next read always waits one cross-core round trip.
  During the load no frame is in the queue at all (draw off since stage 4): every wait in the load step protects nothing.
- M2 synchronous load. load_map_by_IDs runs in one step. Port work was added inside it: romfs seam and .dlc reads (MAPLAG, nd_shape.c:258-268), the drains.
- M3 inline first sight. The new map's lists, textures and model records build in the frames that draw them. MAPX-3 and DOORHITCH spread part of it
  over covered frames (pm_xfer.c:154-180, :198-236). While the draw is off (stages 4 to 6) core 0 has no frame to walk and does nothing.
- Display list era: build 3542 (92e01310) had the same drain (pm_os.c:462-466) and the same per-read invalidation (pm_os.c:435) (FACT git show).
  Before CORE2-3b (c0ab8296f3, 8 Nov) the step ran serially on core 0: a ROM read was a plain copy. The drain put back the serial guarantee at every write.
- What made it matter: core 1 now runs ahead with key frames and the next begin in the queue (plan_walk.md 1), so a full drain waits for GPU and pace.
  The .seam and .dlc romfs files (S7MAP 12 Nov, DLCRD 15 Nov) added FS waits. No transition data of build 3542 exists (OPEN).

## 3. Design A: resource lifetime by reader stamps (rows MTX1, MTX5)
- A reader entry is a queue entry that reads game RAM: a frame walk (pm_wq_frame nd_wq.c:153) and a hidden list prebuild (pm_xfer.c:232). All other
  entries read port memory only (begin: staged texels pm_tex.c:699-705; end: command buffer; invalidation: slot table; menu present; shots). FACT code read.
- Each reader entry gets the next number r (wq_r_push at the post). The walker sets wq_r_done = r after it ran. A resource carries a stamp s: the
  wq_r_push value when the resource was last reachable by the recorder. A writer waits until wq_r_done >= s, or not at all when it has passed.
- Default stamp for an unknown destination: wq_r_push now (every reader posted so far). It replaces the blanket drain in rom_read, decode_yay0, the
  "map" and "mode" drains. It waits only for queued frames. In the load step it never waits (no frame since stage 4).
- Narrow stamps fall out of the model: ROMDR (a free raster entry, lazyDeleteTime 0) = stamp 0 (nuPiReadRomOwn). PAUSEDR (msg.c:1405 memcpy) = stamp 0.
  HUD and item icon append (hud_element.c:198, :234; item_entity.c:312, :345) = the stamp of the last cache clear. A general heap block = the stamp of
  the last general_heap_free. NPC sprite headers (101b90_len_8f0.c:179) = stamp 0 (logic-private). These are row MTX5 (normal play, not transitions).
- Kept as full drains (they wait for walker output or walker state, not for readers): "dlcpre" (pm_map.c:154, the walker pm_dlc_gen bump pm_tex.c:1256),
  "zbuf" (pm_main.c:297), "cfb" (background_gfx.c:263).
- Stamps already in use for GPU reads: ring chunks (pm_gpu_rect.c:41), glyph cells (pm_tex.c:322), texture staging (pm_tex.c:795). Design A is the same rule for game RAM.

## 4. Design B: asynchronous load jobs with a completion handshake (rows MTX2, MTX4)
- Job = a file read or decode on P with a take at the consumer (the pm_prefetch_post and pm_prefetch_take pattern, pm_prefetch.c:87-124).
- MTX2: the .seam and .dlc files of the target map are read on P at the first covered step (stage 4, 5 idle steps). pm_dlc_preload takes the bytes.
  The files depend only on the map name (nd_shape.c:145, :151, :218, :224). Cut: seam+dlcp, 88 to 115 ms (FACT A) leave the load step.
- MTX4: the map song samples pre-decode on P at the song change (pm_prefetch_song, as BTLWIPE F4). Only if MTX0 shows dec in visible steps.
- Must stay synchronous (the main script reads them in its first run, world.c:241): shape, hit, collision, model tree, texture handles, map init.
  These are memcpy and Yay0 (7.8 ms, FACT A). No slicing is needed for them.
- Not done: slicing load_map_by_IDs over steps (it clears heaps and lists in one order; the cover hides it), preload of neighbor maps (heap, no gain).

## 5. Design C: staged first-sight fill under covered steps (row MTX3, design only)
- In stage 6 core 0 is idle. After the load, post bounded walker jobs per covered step: list prebuild of all models (as pm_xfer.c:209-236, XH_STEP 12)
  and texture cache fills of the map textures. A texture fill must not move the texture unit state (DOORHITCH found det shot changes, pm_xfer.c:206-207).
- Map statics restore: sliced only if MTX0 rst shows more than 2 ms (GUESS it does not).

## 6. Invariants and proof
- I1: a write into game RAM happens only after every reader entry that may read the old bytes ran (stamp due). I2: an entry that reads game RAM is a reader.
- Det benches run serially: a post runs at once (nd_wq.c:146-150), so stamps are due by construction and the gates are byte-equal (FACT code).
- The console proves the races: MTX1 counts norseq (a texture source read on the walker outside a reader entry, pm_tex.c:1384) and keeps romrace.
  Both must stay 0 in the capture. A count above 0 prints one log_e line with the caller.
- Gates: xfer_map_shots (covered and first drawn frames) and spot_mac01_drec, byte-exact against `--commit $(git merge-base HEAD main)`.

## 7. Options for the user (expected cmax = longest covered step, vmax = longest visible step; all ms GUESS until MTX0)
| option | rows | model | cost | risk | load step |
|---|---|---|---|---|---|
| A stamps | MTX1 | Haiku | nd_wq.c, nd_walk.c, 4 call sites | a missed reader entry: norseq census | 250-272 -> 130-190 |
| A+B | + MTX2 | Haiku | pm_prefetch.c, nd_shape.c | heap for the .dlc bytes (OPEN size) | -> 40-100 |
| A+B+C | + MTX3, MTX4 | Sonnet (user yes) | walker job budget, texture fill | texture unit state | vmax at the normal step |
- Target: vmax at or under the normal step of the scene now (normal ft 41 ms, plan_critpath.md 0), 33.4 ms after the frame rate rows. cmax under 100 ms.

## 8. Why worse now (FACT xfer lines, mac_01 -> mac_00 first trip)
| data | max | load | init | seam+dlcp | rdw (drain ms per transition) |
|---|---|---|---|---|---|
| D 11 Nov | 163.3 | 139.7 | 163.3 | none (device compile, dlc 121 ms) | 562.1 |
| C 16 Nov | 267.4 | 186.5 | 267.4 (seam, dlcp in the first drawn step) | 98.8 | 1111.7 |
| B 20 Nov | 239.3 | 220.6 | 213.2 | 121.4 | 520.5 |
| A 20 Nov | 240.3 | 270.3 | 49.8 | 91.8 | 126.0 |
- The growth since 11 Nov is the romfs file waits (S7MAP, DLCRD), moved into the load step by MAPLAG (16 Nov). ROMDR cut rdw (B -> A).
- Rows NC0B to GD9A (34 merges, `git diff ed3ae2039d a79f8ce548`): none edits the load path (world.c, pm_map.c, pm_prefetch.c, pm_mapdata.c, nd_shape.c load code).
  ROM read: ROMDR (raster reads without drain), STEPX, STEPX2, DRAINC (counters). Queue: SLOTL (slot waits count L frames), GDW (prof timers).
  Caches: DO1 (core 1 memo, cleared at a texture generation change nd_rec.c:310), MSNAP2, MCOPY1, MCOPY2, GD1 (per frame). The max did not grow (C -> A).
- OPEN: the user may feel the visible steps (o34 8 to 34 includes normal steps over 34 ms: idle 68 percent slow, plan_walk.md 6). MTX0 decides.
- Other maps: pre 0 transitions decode Yay0 on L: 66 to 147 ms (FACT build3ds/hw_menu1_live.txt #1622, #1712). rdw up to 1909.6 ms (#1624).

## 9. Measurement
- MTX0 (page ws_MTX0): per transition, steps by phase (o fade out, c covered, n fade in, t tail) with max and over-34 counts, the 10 longest steps
  with drains (calls, waited, ms), ROM reads and KB, Yay0, texture misses, model misses, list compiles, heap allocs and frees, restore KB, audio join ms,
  sample decodes, and the top 4 drain callers. Azahar counts all counts deterministically (det is serial: waits are 0, times are not valid).
- Console capture after MTX0: capture_check, then `--profile prof warp=mac_01:0 story=ch0_toad_town profdisk=1`, -Seconds 150. The user walks
  mac_01 -> mac_00 -> mac_01 -> mac_00 (south exit and back) 3 times. Read xfer4, xfer5, xfer6, xfer7 of each transition.

## 10. Rows (ranked; memory against the static margin 378 KB, FACT mem boot B)
| # | row | page | model | gate | effect | memory | fallback |
|---|---|---|---|---|---|---|---|
| 1 | MTX0 | ws_MTX0 | Haiku | xfer_map_shots, xfer_map_steps | 0 (decides) | about 1 KB bss | delete the census |
| 2 | MTX1 | ws_MTX1 | Haiku | xfer_map_shots, spot_mac01_fps60_det | load step -60 to -115 (GUESS) | 256 B | the 4 sites back to pm_walk_drain |
| 3 | MTX2 | ws_MTX2 | Haiku | xfer_map_shots, spot_mac01_drec | load step -88 to -115 (FACT A sizes) | heap only | read on L as now |
| 4 | MTX3 | design | Sonnet | xfer_map_shots | init and fade-in first sight | 0 | warm frames as now |
| 5 | MTX4 | design | Haiku | aud bench | visible dec | heap | render decode as now |
| 6 | MTX5 | design | Haiku | per caller | normal play waits | 0 | default stamp |
- Why Sonnet for MTX3: a texture fill outside a draw must keep every texture unit word the walker reads (pm_xfer.c:206-207).

## 11. Order
1. MTX0 now (census, no behaviour change). 2. Console capture (section 9). 3. With the user's yes on section 7: MTX1, then MTX2 (disjoint files).
4. Capture again. 5. MTX3 and MTX4 pages only if xfer4 vmax or n steps still exceed the target.
