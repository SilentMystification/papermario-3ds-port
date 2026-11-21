# plan_l1.md: L1 working sets and phase batching (oracle L1, code read, edit in place)
Base: main dfb703014f. Tags: FACT (source), GUESS, OPEN. Core 1 = logic thread (recorder). Core 0 = game thread (walk).
Console data: build3ds/out/live_xfer3_170.txt window f15344 (build d3641712cc, O3DS, mac_00 600 0 0, fps60 on). Sizes: build3ds/pm_3ds.map (21 Nov 02:01) via 3ds/tools/hotset.py.

## 0. Answer (user question: can each phase run inside L1, then the next chunk?)
1. Yes for one split: game draw code and the per-unit recorder. The game writes its setter units into a 1 KB queue, the recorder runs the queue in one loop (row L1Q1). Output stays byte-exact: the queue keeps the unit order and every other recorder entry drains it first.
2. No for the game code itself (logic and draw callbacks): the game owns the order of its calls and their side effects.
3. No across model records, entity copies and snaps: each one hashes or copies nd_ds, and the one before changes nd_ds (serial chain, section 3).
4. No for the core 0 walk today: the GPU word order is the output, and the model hit path touches shared LRU caches and flushes inside (pm_gpu.c:4364, :4377-4380, :4387). A resolve pass and a submit pass need a design pass (Opus, user yes).
5. kc_build, key show and the interp lerp already run as one pass over their items. Tiling gives nothing there.
6. First the data: row L1PH0 measures I-miss and D-miss per class. No batching row starts before that data (section 5).

## 1. Facts per step (f15344)
- Core 1: ic1 82382 I misses, dc1 43479 D read misses, cyc1 11194060, lgms 42.15 ms (FACT core2 line). 82382 x 32 B = 2.64 MB code fetched per step (FACT arithmetic).
- Core 0: pmu m 72384 I, 35088 D read misses, cyc 8223545 per clean frame (FACT). gpu d 133 draws, w 7485 words (FACT c1a line). 72384 / 133 = 544 I misses = 17.4 KB code per draw (FACT arithmetic).
- Miss cost: about 40 cycles per I miss (GUESS, plan_critpath 2.1 istall/imiss 40.7 on the XL, Nov 11 build). Not measured on this build (OPEN).
- .text 3414396 B in 12250 functions: game 2608620 B, port about 614 KB, devkitARM and libctru 191884 B (FACT map).
- L1: 16 KB I + 16 KB D, 4-way, 32 B lines, no L2 (FACT hw_guidelines.md).

## 2. Phases, code and data (sampled ms: c1t, c1d, c1g, cls lines of f15344, timer bias included)
| core | phase | code (file:line) | hot .text (FACT hotset M1hot or map) | data per step | ms | batch |
|---|---|---|---|---|---|---|
| 1 | game logic | step_game_loop main_loop.c | 624 B root, total OPEN | game state | c1s logic 4.01 | no: game order |
| 1 | game draw code | cam_main.c:224-235, main_loop.c:221-292 | appendGfx_model 4524, render_models 2680, draw_box 7880, update_status_bar 5260 | game objects | c1g task 4.99, mode 1.70 | no: game order |
| 1 | recorder per unit | nd_rec.c:611-638 | class J 15936 B | nd_ds 1164 B (FACT mmemo log), stream writes | rnr 5.49 (975 calls, FACT c1d n / 290) | yes: L1Q1 |
| 1 | snap build | nd_snap.c:50-83 | class K 5288 B | si_t 4 KB, sd_t about 66 KB (nd_snap.c:26, :28) | snap 2.40 (100 per step) | inside L1Q1 (TRI units, nd_rec.c:615) |
| 1 | model record | nd_rec.c:349-374 | class L 18416 B | mm_t 58368 B, 228 B per entry (FACT mmemo log) | rmdl 7.82 (111 per frame) | no: serial |
| 1 | entity copy | nd_rec.c:396-417 | in L | 2 NdDs copies per entity (:392, :410) | rent 3.14 (24 per frame) | no: serial |
| 1 | alt units | nd_rec.c:642-682 | in J | copy words e 3490 c 268 o 4199 (FACT c1a) | alt 2.77 | already loops |
| 0 | walk | nd_rec.c:876-896 stream_run | class A 4972 B | stream 145 KB per context (FACT ndc kb) | cls rest 10.63 | no today |
| 0 | model hit | nd_recs.c:265-294, pm_gpu.c:4350-4404 | class C 6636 B | model cache entries | c0r key 1.06, mtx 0.75, hit 3.62 | no today |
| 0 | submit, emit | pm_gpu.c:1440, nd_emit.c | class D 10660 B | command buffer 30 KB per frame | sub 4.44, emit 2.10 | no today |
| 0 | key frame | pm_gpu.c:3213 kc_build | 1316 B | L buffer copy | kb 1.80, ks 1.54 | already one pass |
- One model draw on core 0 runs C + D = 17.3 KB hot code, more than 16 KB (FACT sum). The unit replay loop runs J + K = 21.2 KB, more than 16 KB (FACT sum). The M3 census (L1PH0 step 3) gives the bytes that really run.

## 3. Constraints (why "no")
- Game code: the order of setter calls is the stream order. The game reads back the stream: nd_rec_pos (msg_draw.c:334, filemenu_msg.c:183), nd_rec_mark (model.c:3525, :4138), nd_rec_data returns a buffer the game fills (14 sites, for example effects/firework.c:262).
- Model record: mm_hash reads nd_ds (nd_rec.c:278-285), mm_apply writes nd_ds (:342), nd_ds_model writes nd_ds (:370). Record i+1 reads the result of record i.
- Snap: snap_build0 reads nd_ds and the memo sn_lp, sn_cmds of the last snap (nd_snap.c:61). Entity: ent_s0 = nd_ds (nd_rec.c:410), then nd_ds_list writes nd_ds (:412).
- Core 0 model hit: nd_tex_get (pm_gpu.c:4364) and nd_tmpl_build, nd_tmpl_touch (:4377-4380) move LRU state that later draws read. pm_gpu_flush (:4387) and the submit loop (:4392-4402) write GPU words. A resolve pass over N records before their submits changes the LRU order (OPEN: a pin per batch).
- List draws record their call site (nd_gtab.c:139 __builtin_return_address): nd_draw_list cannot go into a queue.
- Pointers in units (VTX, MTX, lights, lists) are read again by core 0 in the walk (nd_xf.c:24): the data is stable for the whole step, so a queue inside the step is safe (FACT code read).

## 4. Measurement row L1PH0 (Haiku, start now, page ws_L1PH0.md)
- Switch profph. The NDT class stack (pm_gpu.c:1301-1313) also sums PMN0 and PMN1 per class, exclusive as the ticks. Sampled steps run serially on core 0 (pm_logic.c:7-8, :138), so one counter set covers both the recorder and the walk.
- Lines per window: `l1m set <c> <class> <e0> <e1> <pushes>` x 3, `l1t set <c> s <n> e0 e1 cal`, `l1q u cut xf dl r <7 buckets>` (unit runs, Azahar-exact).
- Step 3: M3 of the fncov lane (hotset.py --fncov) gives the bytes that run per frame per class: the compulsory miss floor is M3 / 32.
- Bias: 2 svc per class edge. The cal pair gives the misses of one empty edge pair. Real class misses = e0 - pushes x cal e0 (GUESS: the hot cal is a low bound).
- Capture after the merge (orchestrator, user yes): profile prof, words `warp=mac_01:0 pos=0:0:200 profdisk=1 profph profrot`, 120 s. capture_check proof `l1m,l1t,l1q`. profrot gives sets m (imiss, drmiss) and s (istall, dstall).

## 5. Rows and start rules (a rule uses the L1PH0 numbers, set m and set s windows)
| row | model | status | what | counter | gate |
|---|---|---|---|---|---|
| L1PH0 | Haiku | start now | class PMU counts, unit run census, M3 | none (debug lines) | build, host, c0gate, spot_mac01_drec byte-equal, capture_check OK |
| L1Q1 | Haiku | WAIT rule Q | setter unit queue in front of nd_rec_nr | ic1, lgms (console), l1q u / cut (Azahar) | host uqunit with seed, spot_mac01_drec and hudfb_drec byte-equal, 0 uqhole lines |
| L1Q2 | - | no page | transform units (nd_xf_push, nd_xf_pop) into the same queue | ic1 | after L1Q1 |
| L1C0 | Opus | no page, user yes | core 0 resolve pass and submit pass for runs of model records | pmu m per draw | design pass |
- Rule Q (start L1Q1): (a) l1m e0 of rnr + snap + alt is at least 3 x (M3 J + M3 K) / 32, and (b) l1q buckets 8-15 and up hold at least 40% of u. If (a) fails: drop L1Q1. If (b) fails and l1q xf is at least 0.3 x cut: write L1Q2 first.
- Rule C0 (ask the user for L1C0): l1m e0 of mkey + mmtx + mhit + sub + supd + semit is at least 50% of the l1t e0 walk share (pmxph gbi phase).
- RESULT (FACT, console f389 set m, build 7933 -> 8016): core 0 walk pmxph gbi 50957 -> 46971 I-misses (-3986, -7.8 %: L1C0B, sub -2628 mhit -1814 mkey -911 mmtx -427). Core 1 list build pmxph dl 41124 -> 41050 (-74: L1Q1 gives nothing). Cause: 527 units in 55 runs per step, about 9.6 units per drain; rec_open, snap and nd_rec_pos sync the queue (nd_rec.c:224, :760, nd_snap.c:135-136), and J + K (21.2 KB) exceed 16 KB L1. The rnr class push count stays 466 because rec_nr0 pushes per unit (nd_rec.c:621), not a sign the queue is off. L1Q2 is dropped (same short runs). A core 1 gain needs fewer sync points or a smaller per-unit path (design, OPEN).
- Expected (GUESS until the data): L1Q1 ic1 -5000 to -15000 per step, lgms -1 to -3 ms. Core 1 is off the critical chain today (plan_critpath.md 0 item 5): L1Q1 gives core 1 margin, not frame time, until core 0 shrinks.
