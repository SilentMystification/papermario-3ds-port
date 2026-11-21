# plan_interp.md: INTERP, the cost of the fps60 key frame and the time order of the presented pictures (oracle NATIVE23, edit in place)
Tags: FACT (log line or code read), GUESS, OPEN. Lines: main 09bb4a8727. Data: build3ds/hw_live_log_170.txt f4585 (main b066452693, O3DS .170, mac_00, prof,
fps60), capture 3 (plan_walk.md:3, mac_01). libctru and citro3d headers: docker image pm3ds-build:1, /opt/devkitpro/libctru/include. Counter: no row moves C1..C12.
## 0. Result
1. L shows t = 0.5 between step N-1 and step N (pm_nusys.c:172 `pm_interp_lerp(0.5f)`). K shows step N. K is a copy of the L command words plus a patch list
   (kc_build pm_gpu.c:3150-3193): no second walk, little CPU. FACT.
2. K costs the same GPU time as L: the copy draws the whole scene again. gpuk 7.73 ms per step (FACT f4585). L has the same words (GUESS L about 7.7 ms).
3. Only a skip of K (KSKIP, section 5) removes GPU work, and only when nothing moved. Today no K copy is equal to L: a static leaf gets a patch (1.4).
4. Presentation fault (section 6): the L and K swaps of a step come about 8 ms apart, with no vblank wait. A picture can be dropped (GUESS half of the steps).
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
## 6. Presentation time order (user report: wobbly when moving fast. FACT code read unless marked)
1. L_N begins on the sync60 grid (pm_gpu.c:3032, :2976-2987): ticks, period 4481228 = 59.83 Hz (:2977), not locked to the vblank. It holds while a begin is
   less than one period late, else it starts again (:2986). C3D_FrameEnd (:3342) queues the display transfer. citro3d swaps in its queue callback when the
   GPU is done (renderqueue.c:227-230, coordinator FACT. libcitro3d.a onQueueFinish calls gfxScreenSwapBuffers, C3D_FrameEnd calls C3D_FrameBufTransfer
   then gxCmdQueueRun: FACT objdump). The latest present is applied at the next vblank (gspgpu.h:133).
2. K_N: the key begin is always late (kpace 0.00), waits for GPU L (kgx 2.34), draws 7.73 ms (gpuk), swaps. No begin waits for a vblank (fb_flags 0, :3030).
3. So the L and K swaps of a step are about 8 ms apart, K to the next L about ft - 8 = 26 ms (GUESS from FACT gpuk and ft 34.33). libctru: one swap per
   screen per frame, no triple buffering, else glitches (gfx.h:150-161). gfxGetFramebuffer returns buffer[gfxCurBuf ^ gfxIsDoubleBuf], the swap flips
   gfxCurBuf (FACT objdump of libctru gfx.o). No vblank in the 8 ms: L is never shown, and K's transfer writes the buffer that is still on screen (tear).
   GUESS share (16.7 - 8) / 16.7 = 52 percent of steps. The free tick grid makes the share drift: a 30 / 60 mix.
4. Late steps: iv bins (pm_main.c:718: 1 or less, 2, 3, 4+): 9 / 265 / 14 / 12 (FACT f4585), 15 / 236 / 34 / 15 (capture 3). 632 vblanks for 300 steps (FACT).
5. Weight: t is always 0.5, never from time. fps60 steps have no accumulator (pm_main.c:683): a late step slows the world, and shows L and K for 1 + 2 or 2 + 1
   vblanks (uneven). Dynamic mode (pm_fpsmode.h:24) drops lerp and K after a late or heavy step (pm_nusys.c:171): a 30 / 60 switch. FS1 (fskip on by default,
   pm_options.c:18, pm_main.c:693-694) can drop a late step's draws (0 in f4585: the iv bins sum to 300 steps). OPEN: the user's fps60 mode.
6. Not interpolated: the sky scroll comes from cam->curBoomYaw and lookAt_obj per step (background.c:285-291), a texture rect with no matrix: it moves at 30 Hz
   under a 60 Hz world while the camera pans (GUESS a main wobble source). Screen-space rects, the HUD slide (inventory.c:737-746), messages, effects with own
   vertex positions: OPEN (IMOV). A rejected leaf moves at 30 Hz. camera_epoch has no reader (pm_interp.c:84-86): a camera cut gives one blended L picture.
7. Lerp math: component-wise (:164). A rotation shrinks at the midpoint by cos(d/2) (GUESS 0.14 percent at 6 degrees per step). Camera and player use one t.
8. PRES1 present gate: in the key show run the queue after the K entries (as KICK3 :867), wait until the PRES0 callback has done the L swap and
   gspIsPresentPending(GSP_SCREEN_TOP) is 0 (gspWaitForVBlank, guard 2), then C3D_FrameEnd. The same wait before the L FrameEnd (:3342). Each picture then shows 1 vblank or more, and no transfer writes the shown
   buffer. Cost: core 0 waits up to 1 vblank while the GPU draws K (GUESS ft +0 to +3). Shots read the target: gate byte-equal.
   PRES2 (Opus, user yes): presents from the vblank event, one picture per vblank, 3 buffers, a late step holds K, the pace grid on the vblank tick, L's t
   from the predicted present vblank. PRES3: the sky scroll as an interpolated scalar with a K patch (after IMOV).
9. Measure (PRES0, no pictures): `pres lk` = vblank gaps after an L picture (0, 1, 2, 3+), `kl` = gaps after a K or plain picture, drop = a swap while the
   last present is pending, wob = sum of |dh - g| (dh: phase step in half steps, L = 2s+1, K = 2s+2, g: vblank gap) = the lerp weight error by time.
   Ideal: lk 1 and kl 1 only, drop 0, wob 0. Azahar: counts valid, gaps not. The console decides.
## 7. Rows (ranked, ms GUESS unless marked)
| # | row | model | what | needs | gate | gain |
|---|---|---|---|---|---|---|
| 1 | IK0 | Haiku | census `kcost`, `kcost1` (ws_IK0) | none | spot_mac01_fps60_det byte-equal, capture_check | 0 (decides 4 to 8) |
| 2 | PRES0 | Haiku | census `pres`, `pres1` (ws_PRES0) | after IK0 (same files) | the same | 0 (decides 3, 9) |
| 3 | PRES1 | Haiku | present gate (6.8) | PRES0 drop above 0 | fps60_det byte-equal, console drop 0 | wobble, ft +0 to +3 |
| 4 | IK1 | Haiku | static leaves to K | IK0 st, user yes | K shots byte-equal, L expected diff | patches -st share |
| 5 | KSKIP | Haiku | skip K when K = L | IK1 | fps60_det byte-equal, ksk, kskipall DIFF | idle GPU -7.73, core 0 -2.34 FACT |
| 6 | IK2 | Haiku | twin skip | IK0 tw | fps60_det, stereo_det byte-equal | core 0 -0.2 to -0.6 |
| 7 | IMOV, PRES3 | Haiku, Sonnet | 30 Hz movers census, sky lerp | PRES0 | census, then K patch gate | wobble |
| 8 | IK3 | Haiku | rtab stamp | IK1 | host iocc, fps60_det | core 1 -0.05 to -0.2 |
| 9 | PRES2 | Opus | vblank present scheduler | PRES1 data, user yes | fps60_det, console pres | wobble 0 |
| 10 | IK4 | Sonnet | K in place | PIPE3 | fps60_det, stereo_det | core 0 -0.1 to -0.3 |
- Capture (user's word, after IK0 and PRES0 merge): `--profile prof fps60 warp=mac_01:0 pos=0:0:200 profdisk=1`. The user stands 2 windows, then runs fast
  and turns the camera 3 windows. Read kcost, kcost1, pres, pres1, c0k, c0i, at. capture_check first with the same words.
- Fallback: every row reverts to now. Memory: IK0, PRES0 under 200 B .bss (GUESS). Static margin 382 KB (FACT mem boot). A 4 KB page step is possible.
