# plan_interp.md: INTERP, the cost of the fps60 key frame and the time order of the presented pictures (oracle NATIVE23; edit in place)
Tags: FACT (log line or code read), GUESS (estimate), OPEN (not measured). Lines: main 09bb4a8727. Data: build3ds/hw_live_log_170.txt f4585 (main b066452693,
O3DS .170, mac_00, prof, fps60), capture 3 (plan_walk.md:3, mac_01). libctru and citro3d headers: docker image pm3ds-build:1, /opt/devkitpro/libctru/include.
Counter: no row of this plan moves C1..C12 (perf and presentation rows). Row metric: the census lines below and the console capture.

## 0. Result
1. The two frames of a step: L shows t = 0.5 between step N-1 and step N (pm_nusys.c:172 `pm_interp_lerp(0.5f)`). K shows step N itself. FACT.
2. K costs little CPU. K is a copy of the L command words plus a patch list (kc_build pm_gpu.c:3150-3193). There is no second walk. FACT.
3. K costs the same GPU time as L. The K copy draws every draw of the scene again: gpuk 7.73 ms per step (FACT f4585). The L lists are the same words (GUESS L about 7.7 ms).
4. K is not cheaper on the GPU in any scene today. Only a skip of K (section 5, KSKIP) removes GPU work, and only when nothing moved.
5. A static object never gives an equal K. L uses the float matrix of the leaf, K uses the s15.16 Mtx (FACT section 1.4). So the patch list is never empty.
6. The presentation has a time fault (section 6): the L and K swaps of a step come about 8 ms apart with no vblank wait. A picture can be dropped. GUESS share: half of the steps.

## 1. Core 1 side (pm_interp.c). It runs once per step, not per K frame.
- Record: every guMtxF2L inside the record adds a leaf (node id, leaf index, dest Mtx*, float mf, 80 B) (pm_gu.c:38-39, pm_interp.c:198-208). 55 node sites (grep FI_OPEN in src).
  pm_interp_open hashes (parent, a, b, occurrence) with the GD1 table otab (pm_interp.c:64-76). pm_interp_stop copies leaves and nodes to the previous set (:178-185).
- Lerp (pm_interp.c:136-176): memset of rtab 16 KB, ptab 4 KB, ntab 4 KB (FACT arithmetic RT_SIZE 2048 x 8 B, 2048 x 2 B), two hash builds over the previous nodes and leaves,
  then per leaf: node probe, leaf probe, 16 lerps (`(1-t)*o + t*l`, :164), one rtab insert. Only matrices are lerped. Positions, colours and uv are not.
- Rejects: no node, a node with another leaf count (:157-158), no leaf with this index (:160-162). A rejected leaf has no rtab entry: L takes the K value.
- Leaf counts: f06 rec leaves 27 to 49, peak 141 to 153 (FACT hw_tidal1_live.txt, hw_tidal2_live.txt, debug level). mac_01 OPEN. Cost per step: OPEN (no timer
  around pm_nusys.c:172). GUESS 0.3 to 0.7 ms core 1 (300 leaves x 250 cycles plus 24 KB of memset). lsc in c1t is the snap collector, not the lerp (nd_ds.c:23).
- Slot lock (SLOTL): pm_wq_frame_slot (nd_wq.c:167-169) holds core 1 before the lerp until the last L walk is done, because the walker reads rtab and repl
  through pm_interp_find (nd_mtx.c:47-52). K reads no rtab (a copy). FACT `core2 wait fw 0.00`, `slotl race 0` (f4585): the lock costs 0 now. GD8 has no value now.
### 1.4 Static leaves
- For a leaf with old mf = new mf, L gets 0.5 x mf + 0.5 x mf = mf, the float value (exact halves, FACT arithmetic). K gets nd_mtx_unpack of the Mtx (nd_mtx_set.c:151-160),
  and guMtxF2L truncates mf x 65536 (pm_gu.c:44-45). So K differs from L by up to 1/65536 per entry. Each such vector gets a patch (u5row_k pm_gpu.c:1011-1018).

## 2. Core 0 side: how K is made
- L walk with twins (pm_tw_on, pm_nusys.c:127): each G_MTX load makes a twin K matrix (nd_mtx_set.c:151-162), a twin stack product (nd_mtx.c:80-95),
  combined_k (nd_mtx_set.c:51), pcomp_k and pal_k per palette (pm_gpu.c:1449, :1746), nrm5_k (:4061). The twin runs for every load, moved or not.
- Patches: u5row_k stages K where K differs from L (pm_gpu.c:1011-1018). uni_native (DO3F, :1348), kp_scan after C3Di_UpdateContext (:1272-1285, :1404) and the
  palette emit (nd_emit.c:86-89) write NdKPatch entries (24 B, nd.h:290-295). The list is already a dirty list: only vectors with K different from L get a patch.
- kc_build (:3150-3193) at the L frame end (:3340): checks, nd_kccmd_decode over the used words, memcpy of the used words (gpu w 5194 words = 20.8 KB FACT f4585)
  to kc_buf, the patches, GSPGPU_FlushDataCache. Key show (:3196-3227): sync60_pace, C3D_FrameBegin (waits for GPU L), the recorded entries, the tail list, FrameEnd.
- FACT f4585: kpace 0.00, kgx 2.34 (key show waits for GPU L), nk 300 copies in 300 steps (no key walk), kf 300. CPU of kc_build and key show: OPEN (IK0).
  GUESS: kc_build 0.15 to 0.4 ms, key show 0.05 to 0.2 ms, twins in the L walk 0.3 to 1.0 ms. The L walk itself: nd 22.13 ms sampled (FACT).

## 3. GPU side
- gpuk 7.73 (FACT) = C3D_GetDrawingTime of the K frame (read at the next L begin, pm_gpu.c:3037). gpul 2.74 (FACT) = only the tail after the L FrameEnd,
  because KICK3 runs part 1 early (:867). Full L GPU time: OPEN. Read it as gpu1 (KICK3 run to the first queue drain, PRES0 line `pres1`) plus gpul.
- Per frame: gpu d 91 draws, 5194 words (FACT f4585, mac_00), 113 draws in mac_01 (capture 2). Target 240 x 400 RGBA8 + D24S8 (pm_gpu.c:1896). Triangles: OPEN.
- 96000 pixels per layer. GUESS 0.2 to 0.4 ms per full-screen layer, so 7.73 ms is not plain fill: texture fetch, blending, small draws (OPEN, no GPU counters).
- In stereo the K copy draws both eyes (GUESS 2 x gpuk).

## 4. Minimum K cost against now (per step)
| part | now | minimum | gap removed by |
|---|---|---|---|
| core 1 lerp, record, stop | GUESS 0.3 to 0.7 | lerp of moved leaves only | IK1, IK3 |
| core 0 twins in the L walk | GUESS 0.3 to 1.0 | twins of moved matrices only | IK2 |
| kc_build + key show CPU | GUESS 0.2 to 0.6 | patch apply + entries, about 0.05 | IK4 (after PIPE) |
| key show wait for GPU L | 2.34 FACT | 0 | PIPE4 (plan_pipe.md), KSKIP at idle |
| GPU K | 7.73 FACT | 0 when nothing moved, else = L | KSKIP. No exact partial redraw (section 5) |

## 5. Design options (mechanism)
- IK1 static leaves to K: in pm_interp_lerp, a leaf with `memcmp(o->mf, l->mf, 64) == 0` gets no rtab entry. L then takes the Mtx value, equal to K.
  The patch list holds only moved vectors. The L picture changes by up to 1/65536 per matrix entry for static leaves (sub-pixel GUESS): user decision, default yes.
  Gate: K shots and the icmp K words byte-equal to the base, L shots an expected diff. It removes a 60 Hz float/fixed alternation of every static leaf.
- KSKIP: kc_build sets kc_same when the copy is valid and `np == 0` (armed) or plain. Then K words = L words (kc_build is copy + patches, FACT). The key show then
  keeps sync60_pace (the step stays 2 periods: without it the next L begin comes one period early, :2986), noise_next x 2, dump_shot, pm_lg_key_walked,
  and skips C3D_FrameBegin, the entries and FrameEnd. The target keeps the L picture, equal to the K picture. Gain at idle: GPU -7.73, core 0 -2.34 wait,
  -kc_build copy. ft at mac_00 idle: about 0 (c0i pace 5.68 FACT: the grid holds the step). Heavy static scenes: ft -2 to -7 (GUESS). Needs IK1 to fire.
  Gate: spot_mac01_fps60_det shots byte-equal (the shot reads the target, pm_gpu.c:2200), census `ksk` above 0, seeded fault `kskipall` (skip every K) gives a DIFF.
- IK2 twin skip: a flag per stack level says twin = main. A load with no replacement and an equal twin skips the twin product, combined_k = combined,
  pcomp_k = pcomp. Same bits (same inputs, same operations): byte-exact. GUESS core 0 -0.2 to -0.6.
- IK3 core 1: a generation stamp in rtab instead of the 16 KB memset, skip static leaves (IK1). GUESS core 1 -0.05 to -0.2.
- IK4 K in place (no memcpy, no decode): patch the L buffer after GPU L, submit it again. Conflicts with PIPE3 (the next L walk writes while K draws): needs 2 list regions by parity.
- Dirty-rectangle K redraw on the L picture: not exact in general. A moved object needs the pixels behind its old place: every draw that covers the old and
  new screen boxes, in order, from a reset depth and colour inside the box, no frame grab, per eye. While the camera moves, every pixel moves: no gain. Not planned.
- Input latency: KSKIP and IK1 to IK4 change none. Pace grid: KSKIP keeps it. PIPE3 and PIPE4 change the key show: KSKIP and PRES1 go after them or get WAIT lines.

## 6. Presentation time order (user report: wobbly when moving fast; FACT code read unless marked)
1. L_N: begin on the sync60 grid (pm_gpu.c:3032, :2976-2987). The grid is ticks (period 4481228 ticks = 59.83 Hz, :2977), not locked to the vblank. It holds
   while a begin is less than one period late, else it starts again (:2986). C3D_FrameEnd (:3342) queues the display transfer, citro3d swaps in its queue
   callback when the GPU is done (renderqueue.c:227-230, coordinator FACT). The latest present is applied at the next vblank (gspgpu.h:133).
2. K_N: the key show begin is always late (kpace 0.00), it waits for GPU L (kgx 2.34), then K draws 7.73 ms (gpuk) and swaps. No frame begin waits for a vblank (fb_flags 0, :3030).
3. So the L and K swaps of a step are about 8 ms apart, the K to next L gap about ft - 8 = 26 ms (GUESS from FACT gpuk, ft 34.33).
   libctru: one swap per screen per frame, no triple buffering, else glitches (gfx.h:150-161). With no vblank in the 8 ms, L is never shown and K's transfer
   can write the buffer on screen (GUESS: libctru gfx.c not read). GUESS share (16.7 - 8) / 16.7 = 52 percent of steps. Free tick grid: the share drifts.
4. Late steps: iv bins (pm_main.c:718: 1 or less, 2, 3, 4+): 9 / 265 / 14 / 12 (FACT f4585), 15 / 236 / 34 / 15 (capture 3). 632 vblanks for 300 steps (FACT).
5. Weight: t is always 0.5 (pm_nusys.c:172), not from time. With fps60 the step has no accumulator (pm_main.c:683 sched_next = 0): a late step slows the world.
   A 3-vblank step shows L and K for 1 + 2 or 2 + 1 vblanks: uneven speed. Dynamic mode (pm_fpsmode.h:24) drops lerp and K after a late or heavy step
   (pm_nusys.c:171): a 30 / 60 switch. FS1 (fskip on by default, pm_options.c:18; pm_main.c:693-694) can drop a late step's draws (0 in f4585: iv bins sum to 300 steps). OPEN: the user's fps60 mode.
6. Not interpolated (FACT): the sky background scroll comes from cam->curBoomYaw and lookAt_obj per step (background.c:285-291), a texture rect with no matrix:
   the sky moves at 30 Hz under a 60 Hz world while the camera pans (GUESS a main wobble source). Screen-space rects, HUD slide (inventory.c:737-746),
   messages, effects with own vertex positions: OPEN (census IMOV). A rejected leaf (section 1) moves at 30 Hz. Camera cuts: camera_epoch has no reader (pm_interp.c:84-86).
7. Lerp math: component-wise matrix lerp (:164). A rotation shrinks at the midpoint by cos(d/2) (GUESS 0.14 percent at 6 degrees per step). Camera and player use the same t.
8. Fix: PRES1 present gate: in the key show, run the queue after the K entries (as KICK3 :867), wait while gspIsPresentPending(GSP_SCREEN_TOP)
   (gspWaitForVBlank, guard 2), then C3D_FrameEnd. The same wait before the L FrameEnd (:3342). Each picture then shows 1 vblank or more, no transfer
   writes the shown buffer. Cost: core 0 waits up to 1 vblank while the GPU draws K (GUESS ft +0 to +3 ms). Shots read the target: gate byte-equal.
   PRES2 (Opus, user yes): presents from the vblank event, one picture per vblank, 3 buffers, a late step holds K, the pace grid from the vblank tick,
   t for L from the predicted present vblank. PRES3: the sky scroll as an interpolated scalar with a K patch (after IMOV).
9. Measure (no pictures): PRES0 line `pres`: per picture the vblank gap after an L picture (lk 0, 1, 2, 3+) and after a K or plain picture (kl),
   drops (present still pending at a swap), wob = sum of |dh - g| (dh = phase step in half steps: L = 2s+1, K = 2s+2; g = vblank gap). wob is the
   lerp weight error by time, in half steps. Ideal: lk 1 and kl 1 only, drop 0, wob 0. Azahar: counts valid, gaps not. The console decides.

## 7. Rows (ranked; ms GUESS unless marked)
| # | row | model | what | needs | gate | gain |
|---|---|---|---|---|---|---|
| 1 | IK0 | Haiku | census `kcost`, `kcost1` (page ws_IK0) | none | spot_mac01_fps60_det byte-equal, capture_check | 0 (decides 4 to 8) |
| 2 | PRES0 | Haiku | census `pres`, `pres1` (page ws_PRES0) | after IK0 (same lines) | the same | 0 (decides 3, 9) |
| 3 | PRES1 | Haiku | present gate (6.8) | PRES0 drop above 0 | fps60_det byte-equal, console pres drop 0 | wobble, ft +0 to +3 |
| 4 | IK1 | Haiku | static leaves to K (5) | IK0 st, user yes | K shots byte-equal, L expected diff | patches -st share |
| 5 | KSKIP | Haiku | skip K when K = L (5) | IK1 | fps60_det byte-equal, ksk above 0, kskipall DIFF | idle GPU -7.73 FACT, core 0 -2.34 FACT |
| 6 | IK2 | Haiku | twin skip (5) | IK0 tw | fps60_det, stereo_det byte-equal | core 0 -0.2 to -0.6 |
| 7 | IMOV, PRES3 | Haiku, Sonnet | 30 Hz movers census, sky scroll lerp | PRES0 | drec census, then K patch gate | wobble |
| 8 | IK3 | Haiku | rtab stamp | IK1 | host iocc, fps60_det | core 1 -0.05 to -0.2 |
| 9 | PRES2 | Opus | vblank present scheduler | PRES1 data, user yes | fps60_det, console pres | wobble 0 |
| 10 | IK4 | Sonnet | K in place | PIPE3 | fps60_det, stereo_det | core 0 -0.1 to -0.3 |
- Capture (user's word, after IK0 and PRES0 merge): `--profile prof fps60 warp=mac_01:0 pos=0:0:200 profdisk=1`. The user stands 2 windows, then runs fast
  and turns the camera for 3 windows. Read kcost, kcost1, pres, pres1, c0k, c0i, at. capture_check first with the same words.
- Fallback per row: IK0, PRES0 delete the lines. PRES1 revert (as now). IK1 revert. KSKIP revert. IK2, IK3, IK4 revert.
- Memory: IK0 and PRES0 under 200 B .bss (GUESS), no heap. Static margin 382 KB (FACT mem boot). A 4 KB page step is possible (hw_guidelines.md).
