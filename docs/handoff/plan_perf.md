# plan_perf.md: sanity check and performance plan of the native draw path (oracles NATIVE15, NATIVE16; edit in place)
Base: main 1472130686. Tags: FACT (file:line or log line), GUESS, OPEN. Core 1 = the logic thread (pm_logic.c). Core 0 = the walker.
Line numbers are main 1472130686. Pages: ws_PERF0 to ws_PERF3. Gate base is always `--commit $(git merge-base HEAD main)`.

## 0. Numbers (FACT build3ds/hw_o3ds_log.txt, build 9f6be238aa, mac_01 0 0 200, windows f10889, f11189, f11489)
- ft 46.05 to 51.41 ms per step (:730, :749). fps counts presents: (steps + key frames) * 600 / vblanks (pm_phase.c:578-579), kf 300 per window (:743).
  So 290 steps take about 14 s: the game runs about 21 steps per second (FACT arithmetic). fps 37.7 to 42.6 is with interpolation.
- Core 1: lgms 44.50 to 50.77 ms per step, cyc1 11.75M to 13.28M cycles, ic1 84712 to 97203 I misses, dc1 55308 to 60556 D read misses per step (:724, :743, :762).
- Core 0: wk 35.25 to 39.60, wwait 15.40 to 17.14, so load 18 to 25 ms. pmu m: 39057 to 55525 I misses, 25542 to 30344 D read misses per clean frame (:733, :752).
- c1n per step: snb 125 to 173 snap builds, snn 123 to 171 new, snm 117 to 164 table misses (95 percent), altu 2253 to 2326 alt units, lse 13 to 15 lists (:726, :745, :764).
- c1t snap 7.01 to 8.90 ms, alt 6.63 to 7.80 ms, lsc 0.65 to 0.75 ms, cal 303 ticks per empty timer pair (:735, :754).
  The alt timer runs per unit (nd_rec.c:509-514): 2300 units x 303 ticks = 2.6 ms of timer cost in a sampled step. Real alt cost is about 4.0 to 5.2 ms (GUESS).
- ndc per frame: model 93, ent 11, spr 6, st 1418 native state records, kb 178 stream high-water (:736, :755).
- Core 0 cls: rec 5.09 to 5.39, sub 3.99 to 5.28, rest 2.91 to 5.85, ent 1.60 to 1.70, raw 1.02 to 1.92 ms per sampled frame (:733, :752).
- mx 47.09 to 51.87 ms per step is the wall time of the mixer jobs (pm_au2.c:168-176), not CPU time. OPEN: the CPU share of the audio threads on core 1.
## 1. Does core 0 still decide (code read; class B = a decision core 1 could record; C = a core 0 cache that stays)
| decision | file:line | class |
|---|---|---|
| tile and texture bind: tile window, tlut slot, tile 1 valid, glyph atlas, uv origin | nd_tex_set.c:27-32, :87-101, :103-180, :291-447, :566-594 | B + C |
| model record hit or miss, hit delta decoded from the record units | nd_recs.c:251-276, :86-104, :142-151 | B (delta) + C (cache) |
| model draw state key B from nd_c0 when the record has no model snap | pm_gpu.c:3834, :3871 | B |
| list call path: entity pools, map shape, or alt range | nd_walk.c:209-235, nd_ent.c:365-396, :435-463 | B + C |
| entity segment native or slice: lighting, fog, index room | nd_ent.c:310-326 | B |
| copy unit choice by alt_mode and ex_depth, known-bit bookkeeping om_kh, om_kl | nd_walk.c:259-274, nd_set.c:98-101 | B |
| direct draw bind: merges pso words into nd_c0, refusal | nd_direct.c:48-93, :152-159 | B |
| vertex params: geom, tex_on, tile uv, fog from nd_c0 | nd_geom.c:97-124, :195-203, :454-530 | B |
| rectangles: scissor, tile uv, fill test | nd_rect.c:61-167, :221-295, :314-346 | B |
| segment address resolve from nd_c0.segbase | nd_walk.c:435-443, nd_ent.c:361 | B |
| GPU memos: ls_gpu, nd_sv_comb flush, render signature, NdShadow | nd_snap.c:166-168, nd_set.c:61-69, :111, nd_emit.c:55 | C |
FACT every nd_c0 field is written and read on core 0 only. use_t1 is never written; fill is read only by nd_c0_to_ws (nd_state.c:46-51).
## 2. Duplicate work and duplicate code
| duplicate | file:line |
|---|---|
| Every snap is a full 760-byte NdSnapChk (GUESS size) copied 4 times on core 1 (r to o, sn_last, *rp, the stream) and once on core 0 (sc_cur) | nd_snap.c:43-44, :60-65, :70, :148, :213 |
| 95 percent of the snaps miss the pso table and run the core 1 decision again; one frame has about 165 new snaps for a few unique inputs (GUESS) | nd_snap.c:55-58, nd_decide.c:217-228 |
| Each model record runs a full snap build only to fill 5 decision words that only the miss path reads | nd_msnap.c:23, :35 |
| Two snap formats: NdSnapChk and NdMSnap carry the same prim, omh, oml, geom, cw0, cw1, fogc, blendc, envc, b[2] | nd_snap.h:15-26, nd_msnap.h:9-14 |
| A list table copies the whole NdDs (about 1.4 KB, GUESS) to give core 0 eight words | nd_snap.c:95, :197-203 |
| Model units are decoded 3 times: core 1 nd_ds_model with an alt copy, core 0 hit_delta, core 0 rec_apply alt run | nd_ds.c:463-492, nd_recs.c:86-104, :42-50 |
| Core 1 writes 2 alt ranges per model; core 0 runs them only on a cache miss | nd_ds.c:465, :487, nd_recs.c:44, :48, :188 |
| An entity list with a call is copied twice (flat and mode 1), plus core 0 compiles it again from the list | nd_rec.c:244-248, :262-265, nd_ent.c:378 |
| NdDs struct copies (1.4 KB each) per stream call, raw fallback, entity and direct draw | nd_rec.c:245, :261, :443, :491, :534, nd_snap.c:100 |
| Two fallback formats for a native record: nd_rec_alt (stream units) and nd_rec_alt_raw (copy units) | nd_rec.c:474-487, :494-504 |
| ds_pairs computes nd_ds_ep_rule twice per unit | nd_ds.c:431-432 |
| NdWs (core 1) and NdC0 (core 0) have the same 24 fields; nd_c0_to_ws copies them | nd.h:142-189, nd_state.c:46-51 |
Record formats on the stream: NDC MODEL, SPR, ENT, TEXRECT, NAPI (about 40 runners), CALL, JUMP, END (nd_rec.c:711-720). List paths: stream units, raw alt copy, alt fallback, list tables, inline state lists, static lists, custom gfx ranges, parts, device records.
## 3. Dead code and debug in hot paths (FACT grep: no caller in 3ds/src, src, 3ds/tools, 3ds/tests)
- No caller: nd_snap_cdiff and nd_cd_* (nd_snap.c:239-244), nd_lsc_end (nd_snap.c:97), tri_stats (nd_geom.c:147), cc_text, cc_uses (nd_comb.c:78-97), nd_env_vtx (nd_shape.c:122-126), stdump_on (nd_shape.c:51), pm_gbi_ptr_of (nd_walk.c:444), nd_c0_src (pm_gpu.c:601).
- Dead drec branches: nd_c0d is never written, so c0_dwho and c0diff never fire. c0_wk stays 0, so c0who logs key 0 only (pm_gpu.c:600-604, :634-635, :1336).
- mwsnap is gone. stat_call is unreachable (nd_walk.c:208, :350 comment). cpsnap logs only in drec runs (nd_snap.c:153-154).
- Hot debug: the alt timer per unit (nd_rec.c:509-514) and pm_drec_now() extern calls per record (nd_snap.c:143, :154, :185, :224, :232). Other counters are single stores.
## 4. Risks the 5 drec benches cannot see (order of risk)
1. Model cache miss path (rec_miss, alt ranges, msnap fallback): it runs at map loads, before the drec windows.
2. FACT bug: a model cache hit reads blendc of the last record that missed: pm_gpu.c:4228 reads nd_msnap_w(), set only in rec_miss (nd_recs.c:196). Shroud draws on a hit can get a wrong colour. Fix in ws_PERF3.
3. Battle scenes, entity refusal (ent_m1 copy, nd_walk.c:189), ent DYN recompiles (nd_ent.c:425-428).
4. fps60 key frame copy and replay (pm_gpu.c:3085-3133) and stereo replay (sr_replay pm_gpu.c:2534): the benches run without them (GUESS).
5. Off-screen targets and frame buffer capture (nd_tex_set.c:52-61), glyph fallback (nd_direct.c:142-220), water copies (nd_model.c nd_r11_walk).
6. Lists over 256 ranges (ls_over, nd_snap.c:92) and full intern table (PERF1): both fall back to correct but slower paths.
## 5. Where the time goes (code read, gains GUESS)
| finding | file:line | cost now |
|---|---|---|
| F1 snap per draw: build, decide, 4 copies, 760 B into the stream | nd_snap.c:40-71 | core 1 5 to 7 ms, core 0 0.5 to 1 ms |
| F2 core 1 I-cache: 85K to 97K misses per step, about 2x core 0 | log :724 | core 1 up to 15 ms (40 cycles per miss, GUESS) |
| F3 alt copies written per unit with slot, applier and 2 values (24 B), mostly not run | nd_rec.c:509-514, :505 | core 1 4 to 5 ms |
| F4 audio control (prio 0x30) and mixer (0x31) on core 1 preempt the logic thread (0x32) | pm_audio.c:116-119, pm_au2.c:191-195, pm_logic.c:93-95 | OPEN |
| F5 model hit on core 0: flush per record, tmpl touch, matrix combine, full block per draw, shadow reset | pm_gpu.c:4214-4241, :1319-1333 | core 0 rec 5.1 to 5.4 ms (FACT) |
| F6 PICA: every cached model draw sends the full state; the next draw sends it again (shadow reset) | pm_gpu.c:1331, nd_emit.c:141-145 | GPU words OPEN |
| F7 PICA: 15 single-register commands (30 words) close every draw, 0x25E twice | nd_emit.c:120-135 | about 0.1 ms CPU, GPU OPEN |
| F8 core 1 list table NdDs copy, ls_find byte-wise FNV over 760 B | nd_snap.c:79, :95 | core 1 0.3 ms |
PICA use is sound in the base: raw words straight into the command buffer (pm_gpu.c:1361-1362), a per-register shadow (nd_emit.c:55), one BufInfo (:1349),
one flush per split (:294-303), one FrameBegin and FrameEnd per frame (:2953, :3249), C3Di_UpdateContext only on dirty state (:1357-1360).
## 6. Rows (merged up to 01e0780881: PERF0, PERF1, PERF3, MDLBLK, SNAPD, DUPMS, ALTU, CNTFIX, C1MIC, NC0A, PROF2, C1HOT. All Haiku unless marked)
Console capture (FACT, build 7569 = main 0b1ba0f322, O3DS .170, mac_01, 17 steady windows, build3ds/hw_o3ds_log.txt):
core 0 24.0 ms, core 1 44.4 ms, ratio 1.86 (was 2.0 to 2.45), wwait 11.2 (was 15 to 17), ft 46.5, fps 42.2 (was 37.7). c1t snap 5.6 (was 7 to 9), alt 8.2.
ic1 92K, dc1 51K per step. c1s logic 2.69, dl 43.9. c1a m 10975 e 2035 c 194 o 4171 words per step; core 0 runs m 0, e 996. gpu d 113 w 6574 per frame.
Core 0 cls: rec 6.15, sub 6.09, rest 5.76, raw 1.93, ent 1.63, list 0.63.
Reading (GUESS from the code read): the 30 ms of list build with no timer is the game draw code (old build 7079: all of core 1 was 19 ms), the untimed
recorder (nd_rec_nr per unit, nd_ds_apply, nd_rec_model with nd_ds_model, msnap, nd_snap_now, nd_mkey_a; the entity copies) and the I-cache misses
(92K per step x about 40 cycles is about 14 ms). PROF2 splits it at the next capture.
| rank | row | what | files (main 01e0780881) | proxy (Azahar) / console number | gate | page / order |
|---|---|---|---|---|---|---|
| 1 | NC0B (Sonnet) | VTX unit 4 words (geom, tss); entry record before each static call; NdGs, nd_gs; geom, tex_ss, tex_ts leave nd_c0 | nd_rec.c, nd_walk.c, nd_geom.c, nd_shape.c, nd_ent.c, nd_set.c, nd_recs.c, nd_ds.c, nd_gtab.c, nd_model.c | geodiff 0, nd_c0 912 to 900 / none | step 1 spot_mac01, entcape_kmr03; step 2 hudfb, fxparts | ws_NC0B, now |
| 2 | NC0R (Sonnet) | target role from the recorded other mode (snap, entry, model snap); omode_l goes | nd_tgt.h, nd_snap.h:10, nd_comb.c, nd_direct.c, nd_geom.c:200, :224, nd_rect.c, nd_shape.c:714, :767, nd_target.c, pm_gpu.c:3991, :4125, nd_model_set.c | rolediff 0, nd_c0 to 896 / none | step 1 spot_mac01, hudfb; step 2 wscmb_pause, entcapd_kgr01 | ws_NC0R, WAIT NC0B |
| 2 | NC0C (Sonnet) | every bind but the glyph reads a block: armed snap, entry block (entry record unit 3), nd_ap, model snap | nd_snap.h NdGs, nd_rec.c rec_ew, nd_snap.c, nd_geom.c:210, nd_rect.c:248, pm_gpu.c:4523, nd_tex_set.c:27-32, :181, :595-599, nd_msnap.c | bindsrc 0, c0src wbind = glyph only / none | spot_mac01, entcape_kmr03 | ws_NC0C, WAIT NC0B; parallel with NC0R |
| 3 | MCOPY1 (Sonnet) | a miss takes hit_apply, record lights, model snap binds; census first; bench.py cmp --expect | nd_recs.c:42-50, :167-196, 3ds/tools/bench.py | altw c0 m 0 / none (c0 m is 0 in steady windows) | spot_mac01, entcape_kmr03, expected-diff mmiss | ws_MCOPY1, WAIT NC0C |
| 4 | MCOPY2 (Haiku) | core 1 writes no model copy ranges; the shadow resets stay | nd_ds.c:461-495, nd_rec.c:224-238, nd_msnap.c:33, nd_walk.c:413-416, nd_ds_test.c | altw c1 m 0 / c1a m 10975 to 0, c1t alt, lgms (GUESS -2 to -4 ms) | spot_mac01, fxparts | ws_MCOPY2, WAIT MCOPY1 |
| 5 | NC0D to NC0G (Sonnet) | see the section 7 table | | c0gate state 1 to 0 at NC0G | all 5 drec | next pass |
| 6 | MDLHIT2 | model hit path cuts, from the PROF2 c0r split | pm_gpu.c:4183-4240, nd_recs.c:251-276 | c0r / cls rec 6.15 | spot_mac01, hudfb | after a capture with PROF2 |
| - | GPUTRL | the 30-word draw trailer (nd_emit.c:120-135) | nd_emit.c | gw / gb | needs a console check (PICA hangs) | user |
Order: NC0B, then NC0R and NC0C in parallel (separate lines; NC0R puts ND_ROLE at nd_snap.h:10, NC0C edits NdGs), then MCOPY1, then MCOPY2.
WAIT reasons (data): NC0R and NC0C read NdGs and the entry record of NC0B, and share nd_set.c:105 and nd_recs.c:148-149 with it. MCOPY1 needs the model snap
bind getter and the snap binds of NC0C, so no later bind reads walker tiles that the copy no longer writes. MCOPY2 deletes the ranges that MCOPY1 stops reading.
Why Sonnet (FACT code read): each NC0 row changes a record format or adds a record across several writers, adds a census and moves 10 to 35 readers in
5 to 12 files; MCOPY1 has a census that picks a branch and a gate tool change. MCOPY2 is a closed deletion list: Haiku.
Gate pattern of the NC0 rows: step 1 switches the readers and keeps the old nd_c0 writes for a census line (geodiff, rolediff, bindsrc); a census line or a
DIFF stops the row (the oracle decides). Step 2 deletes the fields; its gate uses 2 other benches for more cover.
## 6a. MCOPY design (user approved the design pass and the expected-diff gate, 20 Nov; pages ws_MCOPY1, ws_MCOPY2)
- FACT: core 1 writes 2 copy ranges per model record (nd_ds.c:461-495, 10975 words per step); core 0 runs them only in rec_apply on a miss (nd_recs.c:44, :48).
- A miss after MCOPY1: hit_apply (the HC6 end state, nd_recs.c:142-151), rec_lights (the record lights R2 w0, R3 w1 through nd_light_count and nd_mv_mem,
  as nd_ds.c:472-479 and nd_walk.c:246), nd_sr_forget; the tile 0 bind from the model snap only if the census shows the copy moved the uv origin or the slot.
- Risk (code read): the copy's TILESIZE units set the uv origin (nd_tex_set.c:127-130) that the miss draw's gv reads (nd_geom.c:117), and the cache entry keeps
  that gv for later hits. So MCOPY1 step 1 is a census (bits: scalar, lights, tiles, uv origin, slot); a scalar or light difference stops the row.
- The copy open and close reset the core 1 stream shadow (nd_rec.c:534-535 sh_all); MCOPY2 keeps these resets (nd_rec_shreset) at the same 4 places.
- Expected-diff rule (tool, MCOPY1): `bench.py cmp --expect mmiss` passes a DIFF when the tip log has `mmiss f<m>` (drec frames only) with m at or before the
  frame of the first differing draw (mapped by the `drec frame <F> seq <S>` lines, pm_gpu.c:609). Seeded self-test: `bench.py selftest-expect`.
- Expected (GUESS): core 1 -2 to -4 ms (c1t alt and the copy writes); stream -44 KB per step (FACT arithmetic 10975 x 4 B).
## 7. nd_c0 removal (user goal: core 0 decides nothing)
Each class B reader of section 1 reads a recorded value. Sources after NC0B: the snap (nd_ap) for stream, copy and list-table draws; the VTX unit words for a
vertex load; the entry record (NdGs nd_ew, core 1 values at the call) for a static call (map shapes, entity paths); the model snap for model records.
A static list's own GEOM ops change the static register nd_gs->geom (the static cache, class C). The texture cache, the model cache and the GPU memos stay.
Fields left after NC0B and NC0R (FACT grep of the nd_c0 readers, main 01e0780881) and their row:
| fields | readers (count) | row | source |
|---|---|---|---|
| td, tst1, timg, tlut, tlut_n (texture setters) | nd_tex_set.c (160), nd_rect.c (35), nd_ent.c (22), pm_gpu.c (11), nd_comb.c, nd_snap.c | NC0D rects, then NC0G | snap blocks after NC0C; the setters keep only GPU side effects (wrap, tlut_v20, loads at bind) |
| omode_h, comb_w0, comb_w1, prim, prim_lod, envc, conv_k4 | nd_set.c, nd_snap.c:175-183, :210, nd_direct.c, nd_recs.c, nd_decide.c | NC0E direct, NC0G | GPU memos (class C) move to the GPU layer as a PICA shadow; direct draws merge the pso words on core 1 (nd_snap_pso) |
| tile_x0, tile_y0, tile_fx, tile_fy, cur_tile | nd_tex_set.c, nd_comb.c, nd_direct.c, nd_rect.c, nd_geom.c:117 | NC0G | outputs of the bind cache (class C) move into the bind module (ens_ state) |
| segbase[16] | nd_walk.c:438 ptr_res, nd_ent.c, pm_gpu.c, nd_set.c:93, :110 | NC0F | core 1 resolves segment addresses in the recorded units |
| sci_x0 to sci_y1 | nd_set.c:84, :108, nd_rect.c, nd_target.c:83 | NC0D | the snap or the rect record carries the scissor |
| fog_fm, fog_fo, prim_z, tex_on | nd_geom.c:115, :121, nd_comb.c, nd_recs.c, pm_gpu.c:4568, nd_msnap.c | NC0G | snap words (add the fog factors) and model snap words |
The hit delta (nd_recs.c:86-151) goes with the last scalar field. Each of NC0D to NC0G needs its own reader read: next pass, one page each.
## 7a. Console rows
- C1HOT and PROF2 are merged; MDLHIT2 gates on Azahar. The console confirms later: ic1 and lgms (C1HOT), the c1d and c0r lines (PROF2), cls rec (MDLHIT2),
  c1a m and lgms (MCOPY2). The NC0 rows move no console number by design (watch ndc kb and lgms for a rise).
## 8. Balance (user: spread the work, judge by frame time)
- The step time is max(core 1, core 0) plus the queue wait. FACT now: core 1 is the long pole by 20 to 30 ms.
- After PERF1, SNAPD, ALTU and C1HOT core 1 falls 6 to 12 ms (GUESS). Then core 1 is about 32 to 40 ms and core 0 about 18 to 25 ms.
- The pure decision left on core 1 is the combine decision per unique snap. After PERF1 it runs a few times per frame: no move pays.
- The balance lever: the snap input build moves to core 0 if core 1 stays the long pole after a capture (the user asks for it).
## 9. Measurement plan
- Counts come from Azahar gate runs at no extra cost: `altw` (PERF0, drec end) and `snapi` (PERF1, every 100 frames).
- Capture recipe (when the user asks): `-GameDebug "warp=mac_01:0 pos=0:0:200 profdisk=1"`, capture_check first with `--proof "core2 lgms,c1s logic,c1a m"`.
  Read: lgms, c1s logic and dl, c1t snap, c1a, wk - wwait, gpu words.
- Captures only when the user asks (user, 20 Nov). Rows give Azahar proxies: altw words, snapi new, hit, dec, GPU words. C1HOT and MDLHIT2 need a capture.
- Real alt ms from a sampled step = c1t alt - altu per step x cal / 268112 (FACT timer per unit, nd_rec.c:509).
## 10. User decisions (defaults)
- UP1: no push until the user wakes (20 Nov). UP2: Sonnet when a page marks the row and says why (20 Nov). UP3: MCOPY design pass yes (20 Nov).
- UP4: expected-diff gate for MCOPY in model-miss frames, marker mmiss (20 Nov).
- Q2 (open): one capture after MCOPY2 merges, to read ic1, lgms, c1d, c0r, c1a m. Default: at the next wake.
- Q3 (open): a static call (map shape, entity list) keeps a register (NdGs) that holds only core 1 entry values and the static list's own GEOM ops. Is that
  "core 0 decides nothing"? Default yes (the static cache is class C).
- Q4 (open): to reach c0gate state 0, the GPU memos (comb, prim, env, k4) move to the GPU layer and the bind outputs (uv origin, cur_tile) to the bind module.
  These are caches, not decisions. Accept the move (the names list then drops nd_c0)? Default yes.

## Decision (user, 20 Nov)
PERF2 (audio threads on core 0) and capture run B (audiocore0) are dropped: audio stays on core 1. The balance comes from PERF1, deleting duplicate work and moving small decisions to core 0.
