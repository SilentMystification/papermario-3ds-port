# plan_perf.md: sanity check and performance plan of the native draw path (oracle NATIVE15, edit in place)
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
## 6. Rows (merged up to c4648e7561: PERF0, PERF1, PERF3, MDLBLK, SNAPD, DUPMS, ALTU, CNTFIX, C1MIC, NC0A. All Haiku unless marked)
Console capture (FACT, build 7569 = main 0b1ba0f322, O3DS .170, mac_01, 17 steady windows, build3ds/hw_o3ds_log.txt):
core 0 24.0 ms, core 1 44.4 ms, ratio 1.86 (was 2.0 to 2.45), wwait 11.2 (was 15 to 17), ft 46.5, fps 42.2 (was 37.7). c1t snap 5.6 (was 7 to 9), alt 8.2.
ic1 92K, dc1 51K per step. c1s logic 2.69, dl 43.9. c1a m 10975 e 2035 c 194 o 4171 words per step; core 0 runs m 0, e 996. gpu d 113 w 6574 per frame.
Core 0 cls: rec 6.15, sub 6.09, rest 5.76, raw 1.93, ent 1.63, list 0.63.
Reading (GUESS from the code read): the 30 ms of list build with no timer is the game draw code (old build 7079: all of core 1 was 19 ms), the untimed
recorder (nd_rec_nr per unit, nd_ds_apply, nd_rec_model with nd_ds_model, msnap, nd_snap_now, nd_mkey_a; the entity copies) and the I-cache misses
(92K per step x about 40 cycles is about 14 ms). PROF2 splits it at the next capture.
| rank | row | what | files and regions (main c4648e7561) | proxy (Azahar) / console number | gate | page |
|---|---|---|---|---|---|---|
| 1 | C1HOT | fresh hot group from a census; core 1 recorder classes J, K, L contiguous | 3ds/tools/hotset.json, 3ds/ld/hot_order.ld | hotset M2 stale 0, J-L bytes / ic1 92K, lgms 44.4 | spot_mac01_drec, entcape_kmr03_drec | ws_C1HOT |
| 2 | PROF2 | timer classes: core 1 rmdl, rent, rspr, rnr; core 0 key, mtx, hit | nd.h:471, nd_rec.c bodies, nd_recs.c:258-270, pm_phase.c:608, :631 | byte-equal / new c1d and c0r lines | spot_mac01_drec, entcape_kmr03_drec | ws_PROF2 |
| 3 | MCOPY (Sonnet) | delete the model copy ranges (core 0 runs 0 of 10975 words per step) | see the design below | c1a m to 0, d / lgms, c1t alt | all 5 drec, expected-diff in miss frames | after NC0C |
| 4 | NC0B (Sonnet) | vertex load reads geom and tss from its VTX record; triangles read the snap geom | nd_rec.c VTX writers, nd_walk.c nd_vtx_run, nd_geom.c:115-132, :195-252, :373-511 | nd_c0 size | spot_mac01_drec, fxparts_drec | next pass |
| 5 | NC0R (Sonnet) | the target role (nd_img_role, 14 callers) from the snap role; omode_l goes | nd_tgt.h:38-42 and its callers | nd_c0 size | hudfb_drec, wscmb_pause_drec | next pass |
| 6 | NC0C to NC0G (Sonnet) | tile binds from nd_ap->b; rectangles; direct draws; segment pre-resolve; NdC0 and the hit delta go | nd_tex_set.c, nd_rect.c, nd_direct.c, nd_ds.c:36-47, nd_walk.c:435-443, nd_recs.c:86-151 | c0gate state 1 to 0 | all 5 drec | one per pass |
| 7 | MDLHIT2 | model hit path cuts, from the PROF2 c0r split | pm_gpu.c:4183-4240, nd_recs.c:251-276 | c0r / cls rec 6.15 | spot_mac01_drec, hudfb_drec | after PROF2 data |
| - | GPUTRL | the 30-word draw trailer (nd_emit.c:120-135) | nd_emit.c | gw / gb | needs a console check (PICA hangs) | user |
Parallel now: C1HOT and PROF2 (no shared file).
Why Sonnet for NC0B to NC0G and MCOPY (FACT code read): core 0 reads geom at vertex load (nd_geom.c:480, :508-511) and at the triangle (:195-252), so the
value must travel in the VTX record of every copy path (stream, alt copies, model ranges, entity slices); the target role has 14 callers, some with no live
snap (nd_target.c:78, nd_shape.c:714, :767). Each row changes a record format across several writers and needs an expected-diff decision. Not closed edits.
## 6a. MCOPY design (user approved the design pass, 20 Nov)
- FACT: core 1 writes 2 copy ranges per model record (nd_ds.c:463-492, 10975 words per step); core 0 runs them only in rec_apply on a cache miss (nd_recs.c:42-50);
  the console shows c0 m 0 in steady windows.
- What the copy gives a miss: (1) the scalar state (omode, geom, prim, env, fog, blend, combine, texture scale): hit_delta already gives the same end state
  (nd_recs.c:61-67 HC6 rule), so the miss can call hit_apply instead; (2) the lights (MOVEMEM, NUMLIGHT): core 1 records them after the record as a light
  block (NdDs lt[], lt_n) next to the model snap; (3) the tile and load units (R13): the miss draw reads tile 0 and 1 from the model snap b[2] (nd_msnap.h:13)
  once NC0C makes every bind read a snap; (4) the epoch cuts of the marks (cp_unit gv_epoch++): batch splits differ only in miss frames.
- Steps: MCOPY1 (after NC0C): the miss path takes hit_delta, the light block and the msnap binds; MCOPY2: nd_ds_model writes no copy (dm_on goes), nd_msnap_ab
  and the 2 range words go. Gate: all 5 drec; a DIFF passes only in a frame that logs a model miss (new marker `mmiss f<n>`, expected-diff rule).
- Expected (GUESS): core 1 -2 to -4 ms (c1t alt and the copy writes), stream -44 KB per step.
## 7. nd_c0 removal (user goal: core 0 decides nothing)
Each class B reader of section 1 must read a recorded value. Core 1 has every value: the snap carries geom, tss, oml, fill, primz and the tile binds b[2];
core 1 NdDs.w has om_kh and om_kl. NC0A to NC0G in section 6, one closed page per pass (each row needs its own code read of the readers it moves).
The texture cache, the model cache and the GPU memos stay on core 0 (class C). c0gate state reaches 0 at NC0G.
## 7a. Console rows
- C1HOT, PROF2 and MDLHIT2 gate on Azahar. The console confirms later: ic1 and lgms (C1HOT), the c1d and c0r lines (PROF2), cls rec (MDLHIT2).
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
- Q1 (open, for the user): accept an expected-diff gate for MCOPY in model-miss frames (marker mmiss)? Default yes.
- Q2 (open): one capture after C1HOT and PROF2 merge, to read ic1, lgms, c1d, c0r. Default: at the next wake.

## Decision (user, 20 Nov)
PERF2 (audio threads on core 0) and capture run B (audiocore0) are dropped: audio stays on core 1. The balance comes from PERF1, deleting duplicate work and moving small decisions to core 0.
