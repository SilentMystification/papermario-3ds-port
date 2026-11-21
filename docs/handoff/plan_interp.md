# plan_interp.md: INTERP, the cost of the fps60 key frame and the time order of the presented pictures (oracle NATIVE23, edit in place)
Tags: FACT (log line or code read), GUESS, OPEN. Lines: main 09bb4a8727. Data: build3ds/hw_live_log_170.txt f4585 (main b066452693, O3DS .170, mac_00, prof,
fps60), capture 3 (plan_walk.md:3, mac_01). libctru and citro3d headers: docker image pm3ds-build:1, /opt/devkitpro/libctru/include. Counter: no row moves C1..C12.
## 0. Result
1. L shows t = 0.5 between step N-1 and step N (pm_nusys.c:172 `pm_interp_lerp(0.5f)`). K shows step N. K is a copy of the L command words plus a patch list
   (kc_build pm_gpu.c:3150-3193): no second walk, little CPU. FACT.
2. K costs the same GPU time as L: the copy draws the whole scene again. gpuk 7.73 ms per step (FACT f4585). L has the same words (GUESS L about 7.7 ms).
3. Only a skip of K (KSKIP, section 5) removes GPU work, and only when nothing moved. Today no K copy is equal to L: a static leaf gets a patch (1.4).
4. Presentation fault (section 6): K finishes 8.26 ms after L with no vblank wait: half of the L pictures never show, K tears (FACT drop 150 of 300). Wobble causes by rank: 6.3 drop and tear, 6.3 uneven 1 + 2 vblanks, 6.6 sky lead in L, 6.4 ft 38.7 > 33.4, 6.5 weight. Fix: PRES1, then IMOV / PRES3, then perf.
## 1. Core 1 side (pm_interp.c). It runs once per step, not per K frame.
- Record: every guMtxF2L in the record adds a leaf (node id, leaf index, dest Mtx*, float mf: 80 B) (pm_gu.c:38-39, pm_interp.c:198-208). 55 node sites (FI_OPEN in src).
  pm_interp_open hashes (parent, a, b, occurrence) with the GD1 table otab (:64-76). pm_interp_stop copies leaves and nodes to the previous set (:178-185).
- Lerp (:136-176): memset rtab 16 KB, ptab 4 KB, ntab 4 KB (FACT arithmetic), 2 hash builds over the previous nodes and leaves, per leaf: node probe, leaf probe,
  16 lerps (:164), 1 rtab insert. Only matrices are lerped: no position, colour or uv. Rejects (no node, other leaf count :157-158, no leaf :160-162): no rtab entry, L = K.
- Leaves: f06 rec 27 to 49, peak 141 to 153 (FACT hw_tidal1_live.txt, hw_tidal2_live.txt). mac_01 OPEN. Cost: OPEN (no timer at pm_nusys.c:172).
  GUESS 0.3 to 0.7 ms core 1 per step. lsc in c1t is the snap collector (nd_ds.c:23), not the lerp.
- SLOTL: pm_wq_frame_slot (nd_wq.c:167-169) holds core 1 before the lerp until the last L walk is done: the walker reads rtab and repl (nd_mtx.c:47-52). K reads no rtab.
  FACT f4585 `core2 wait fw 0.00`, `slotl race 0`: the lock costs 0 now, so GD8 (plan_gamedraw.md) has no value now.
### 1.4 Static leaves
- Old mf = new mf: L gets 0.5 mf + 0.5 mf = mf, the float (exact halves). K gets nd_mtx_unpack of the Mtx (nd_mtx_set.c:151-160), and guMtxF2L truncates mf x 65536
  (pm_gu.c:44-45). K differs from L by up to 1/65536 per entry, so each such vector gets a patch (u5row_k pm_gpu.c:1011-1018). FACT code read.
## 2. Core 0 side: how K is made
- L walk with twins (pm_tw_on, pm_nusys.c:127): every G_MTX load makes a K twin (nd_mtx_set.c:151-162), a twin stack product (nd_mtx.c:80-95), combined_k
  (nd_mtx_set.c:51), pcomp_k and pal_k (pm_gpu.c:1449, :1746), nrm5_k (:4061). The twin runs for every load, moved or not.
- Patches: u5row_k stages K where K differs from L. uni_native (DO3F :1348), kp_scan (:1272-1285, :1404) and the palette emit (nd_emit.c:86-89) write NdKPatch
  (24 B, nd.h:290-295). The patch list is a dirty list already: only vectors with K different from L.
- kc_build (:3340): checks, nd_kccmd_decode over the used words, memcpy of the used words (gpu w 5194 = 20.8 KB, FACT f4585), patches, GSPGPU_FlushDataCache.
  Key show (:3196-3227): sync60_pace, C3D_FrameBegin (waits for GPU L), the recorded entries, the tail list, FrameEnd.
- FACT f4585: kpace 0.00, kgx 2.34, nk 300 copies in 300 steps (no key walk). CPU of kc_build and key show: OPEN (IK0). GUESS kc_build 0.15 to 0.4,
  key show 0.05 to 0.2, twins in the L walk 0.3 to 1.0 ms. The L walk: nd 22.13 ms sampled (FACT).
## 3. GPU side
- gpuk 7.73 = C3D_GetDrawingTime of K, read at the next L begin (pm_gpu.c:3037). gpul 2.74 = only the tail after the L FrameEnd: KICK3 runs part 1 early (:867). FACT.
  Full L GPU time: OPEN. Read it as gpu1 (KICK3 run to the first queue drain, PRES0 line `pres1`) plus gpul.
- Per frame: 91 draws, 5194 words (FACT f4585, mac_00), 113 draws in mac_01 (capture 2). Target 240 x 400 RGBA8 + D24S8 (pm_gpu.c:1896). Triangles OPEN.
- GUESS 0.2 to 0.4 ms per full-screen layer: 7.73 ms is not plain fill (texture fetch, blending, small draws: OPEN, no GPU counters). Stereo: K draws both eyes (GUESS 2 x).
## 4. Minimum K cost against now (per step)
| part | now | minimum | row |
|---|---|---|---|
| core 1 lerp, record, stop | GUESS 0.3 to 0.7 | moved leaves only | IK1, IK3 |
| core 0 twins in the L walk | GUESS 0.3 to 1.0 | moved matrices only | IK2 |
| kc_build + key show CPU | GUESS 0.2 to 0.6 | patch apply + entries, GUESS 0.05 | IK4 |
| key show wait for GPU L | 2.34 FACT | 0 | PIPE4 (plan_pipe.md), KSKIP at idle |
| GPU K | 7.73 FACT | 0 when nothing moved, else = L | KSKIP. No exact partial redraw |
## 5. Design options (mechanism)
- IK1: in pm_interp_lerp a leaf with `memcmp(o->mf, l->mf, 64) == 0` gets no rtab entry. L takes the Mtx value = K. The patch list then holds only moved
  vectors. The L picture changes by up to 1/65536 per entry of a static leaf (sub-pixel GUESS): user decision, default yes (it ends a 60 Hz float / fixed
  alternation of every static leaf). Gate: K shots and icmp K words byte-equal to the base, L shots an expected diff.
- KSKIP: kc_build sets kc_same when the copy is valid and np = 0 (armed) or plain: K words = L words (copy + patches, FACT). The key show keeps sync60_pace
  (else the next L begin comes one period early, :2986), noise_next x 2, dump_shot, and skips C3D_FrameBegin, entries and FrameEnd. The target keeps the L
  picture = the K picture. Idle gain: GPU -7.73, core 0 -2.34 wait, -kc_build copy. ft at mac_00 idle about 0 (c0i pace 5.68 FACT: the grid holds the step),
  heavy static scenes ft -2 to -7 (GUESS). Needs IK1. Gate: fps60_det shots byte-equal (shots read the target, pm_gpu.c:2200), `ksk` above 0, seeded `kskipall` DIFF.
- IK2: a flag per stack level says twin = main. A load with no replacement and an equal twin skips the twin product, combined_k = combined, pcomp_k = pcomp.
  Same inputs and operations give the same bits: byte-exact. GUESS core 0 -0.2 to -0.6.
- IK3: a generation stamp in rtab instead of the 16 KB memset. IK4: patch the L buffer in place after GPU L (no memcpy, no decode): needs 2 list regions with PIPE3.
- Dirty-rectangle K redraw: not exact in general (the pixels behind an old place need every covering draw in order from a reset depth and colour, no frame grab,
  per eye). While the camera moves every pixel moves: no gain. Not planned.
- Latency: KSKIP and IK1 to IK4 change none. PIPE3 and PIPE4 change the key show: KSKIP and PRES1 go after them, or get WAIT lines.
## 6. Presentation time order (user report: wobble when moving fast. FACT code read unless marked. Lines: main 3a29e12504. Console: build 7933, .170, f2189-f3089)
1. L_N begins on the sync60 grid (pm_gpu.c:3045-3056, :3102): ticks, period 4481228 = 59.83 Hz, not locked to the vblank. It holds while a begin is less than
   one period late, else it starts again (:3055). C3D_FrameEnd (:3417) queues the display transfer (C3D_FrameBufTransfer, nm renderqueue.o). citro3d swaps in
   its queue callback onQueueFinish (gfxScreenSwapBuffers), PRES0 wraps it (pres_cb :876-885). The latest present is applied at the next vblank (gspgpu.h:138-141).
2. K_N (pm_gpu_key_show :3267-3301): sync60_pace never waits (kpace 0.00 FACT), C3D_FrameBegin waits for GPU L (kgx 2.32-2.42 FACT), GPU K 8.0 ms (gpuk FACT).
3. FACT console: glk 8.22-8.29 ms (L finish to K finish), drop 140-157 of 300 = lk bin 0 in every window, kl 2 vblanks 201-220 of 300, wob 400-434.
   No vblank in 8.26 ms in (16.7 - 8.26) / 16.7 = 51 percent of steps (FACT arithmetic, = drop/300). Then K's transfer goes to buffer[gfxCurBuf ^ 1] = the buffer
   on screen (tear, FACT objdump gfx.o), and K's swap replaces L's pending present (L never shown). libctru has 2 buffers per screen (gfx.h:150-161).
4. Step time: ft 38.69-38.94 ms (FACT) = 2.32 vblanks per step. 2 pictures per step cannot fill 2.32 vblanks evenly: after PRES1 K holds 2 vblanks in about
   32 percent of steps (GUESS: uniform phase, (38.7 - 33.4) / 16.7). Only ft <= 33.4 ms (perf rows) removes it.
5. Weight: t is always 0.5 (pm_nusys.c:172). Today half the step motion falls into 8.26 ms (21 percent of the step), the other half into 30.4 ms; on drop steps the
   whole step comes in one picture. After PRES1 L shows exactly 1 vblank before K: even motion needs t = 1 - 16.7 / ft = 0.57 at ft 38.7, 0.50 at ft <= 33.4
   (GUESS arithmetic). The error of t = 0.5 is then 0.07 step per L picture, small against item 4. Default: t stays 0.5 (det runs stay byte-exact).
   fps60 steps have no accumulator (pm_main.c:683). Dynamic mode drops K after a late step (pm_nusys.c:171); FACT nk 300 of 300: no drop in these windows.
6. Not interpolated: the sky scroll comes from cam->curBoomYaw and lookAt_obj per step (src/background.c:285-291), a texture rect with no matrix. In L the world
   is at N - 0.5, the sky at N: the sky leads by half a step of pan in every L picture (FACT mechanism, size OPEN). PRES1 shows every L: this shows more often.
   Screen-space rects, the HUD slide (inventory.c:737-746), effects with own vertex positions, rejected leaves: OPEN (IMOV). camera_epoch has no reader (pm_interp.c:85).
7. Lerp math: component-wise. A rotation shrinks at the midpoint by cos(d/2) (GUESS 0.14 percent at 6 degrees per step). Camera and player use one t.
8. PRES1 (ws_PRES1.md, Haiku): pres_gate before both fps60 FrameEnds (:3296, :3416) polls until pres_sw = pres_tx (the last swap ran) and no present is pending,
   0.2 ms steps, 40 ms cap. The swap counter is needed: libctru clears isRunning before it calls the queue callback (FACT objdump gxqueue.o +0x6c, +0x90), so
   C3D_FrameBegin can return before the swap. The key show runs the K entries at once (gxCmdQueueRun, as KICK3 :914): the GPU draws K during the wait.
   Result: L shows 1 vblank, K from the next vblank, no transfer into the shown buffer. Shots read the target: fps60_det byte-equal.
   Latency (GUESS, uniform phase): the first changed picture of a step comes at the same vblank; the full-step picture (K) comes 1 vblank later on the 51 percent
   drop steps: +8.3 ms average. Core 0 waits in the key show until the vblank (GUESS 4 to 7 ms per step), part of it in today's pace wait (c0i pace 3.85): ft +0 to +2.
9. PRES2 decision: not needed for the wobble. A 3-buffer scheduler presents at the same vblanks as PRES1 (L at V1, K at V2) and cannot fix item 4. Its only gain is
   the core 0 wait of item 8. Cheaper path: PRES2 = after PIPE3, the K C3D_FrameEnd moves into pm_pipe_open (plan_pipe.md 2): core 0 walks L while K waits for
   the vblank. Start rule: PIPE3 merged and the PRES1 console capture shows gms 2 or more and ft up 1 ms or more. Page after PIPE3 (its lines do not exist yet).
   Rejected: own vblank presents. citro3d holds the gsp event callbacks (gspSetEventCallback import, nm renderqueue.o; GUESS VBlank0 and VBlank1, C3D_FrameCounter),
   a third buffer costs 288 KB (mono) to 576 KB (stereo) linear (GUESS RGB8 400 x 240), and the menu presents through libctru (pm_gpu.c:2128, pm_main.c:90).
   PRES3: the sky scroll as an interpolated scalar with a K patch (after IMOV).
10. Measure: `pres lk` = vblank gaps after an L picture (0, 1, 2, 3+), `kl` = after a K or plain one, drop = a swap while the last present is pending, wob = sum of
   |dh - g| (dh: phase step in half steps, g: vblank gap), glk = L to K finish ms. PRES1 adds `pres1 ... gw` (gate waits), `gms` (ms per step), `gto` (timeouts).
   Ideal: lk 1 and kl 1 only, drop 0, wob 0. Azahar: counts valid, gaps not. The console decides.
## 7. Rows (ranked, ms GUESS unless marked)
| # | row | model | what | needs | gate | gain |
|---|---|---|---|---|---|---|
| 1 | IK0 | Haiku | census `kcost`, `kcost1` (ws_IK0) | none | spot_mac01_fps60_det byte-equal, capture_check | 0 (decides 4 to 8) |
| 2 | PRES0 | Haiku | census `pres`, `pres1` (ws_PRES0) | after IK0 (same files) | the same | 0 (decides 3, 9) |
| 3 | PRES1 | Haiku | present gate (6.8), ws_PRES1.md ready | PRES0 drop above 0 (FACT 150) | fps60_det byte-equal, Azahar drop 0, console drop 0 | drop 150 -> 0, ft +0 to +2 |
| 4 | IK1 | Haiku | static leaves to K | IK0 st, user yes | K shots byte-equal, L expected diff | patches -st share |
| 5 | KSKIP | Haiku | skip K when K = L | IK1 | fps60_det byte-equal, ksk, kskipall DIFF | idle GPU -7.73, core 0 -2.34 FACT |
| 6 | IK2 | Haiku | twin skip | IK0 tw | fps60_det, stereo_det byte-equal | core 0 -0.2 to -0.6 |
| 7 | IMOV, PRES3 | Haiku, Sonnet | 30 Hz movers census, sky lerp | PRES0 | census, then K patch gate | wobble |
| 8 | IK3 | Haiku | rtab stamp | IK1 | host iocc, fps60_det | core 1 -0.05 to -0.2 |
| 9 | PRES2 | Opus | K FrameEnd into pm_pipe_open (6.9) | PIPE3, PRES1 gms 2+ and ft +1, user yes | fps60_det, stereo_det, console pres | ft back by gms |
| 10 | IK4 | Sonnet | K in place | PIPE3 | fps60_det, stereo_det | core 0 -0.1 to -0.3 |
- Capture (user's word, after IK0 and PRES0 merge): `--profile prof fps60 warp=mac_01:0 pos=0:0:200 profdisk=1`. The user stands 2 windows, then runs fast
  and turns the camera 3 windows. Read kcost, kcost1, pres, pres1, c0k, c0i, at. capture_check first with the same words.
- Fallback: every row reverts to now. Memory: IK0, PRES0 under 200 B .bss (GUESS). Static margin 382 KB (FACT mem boot). A 4 KB page step is possible.
