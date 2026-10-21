# PACE-INV findings: HUD FPS not a locked 30, and frame time spikes

Data used: 3ds/logs/perf/hw_pull2/hw_o3ds_log.txt (build 902, O3DS, switches pacestat logstat phase phasecsv loglevel=debug).
3ds/logs/perf/hw_pull3/ is empty: no build 926 data. All numbers below are from build 902. Build 902 has no logstat2 line and no AU2.
Build 902 ran with debug logging (1.5 to 2.5 lines per frame, 629 to 1207 dropped lines) and two long scenes where the gbi phase was 45 to 65 ms.
Only steps 340 to 804 (465 steps) are near 30 FPS. Use them for pacing. The two long segments (steps 2434 to 5202 and 6742 to 9006) run at 15 FPS (p50 66 and 74 ms) and are GPU/gbi bound.

## 1. Why the HUD is not a locked 30

What the HUD measures (3ds/src/pm_main.c:383-390): `presents` counts game steps (drawn steps and FS1 skipped steps, not idle iterations).
The window closes at the end of the first step after 1 s of system ticks. fps = presents * 268111856 / elapsed ticks. It is printed with %4.1f (line 431).
No smoothing. The window length is 1000 ms plus up to one step.

Reasons for 29.x:
- The pacing is tied to the retrace, not to a timer. The loop waits with gspWaitForVBlank (pm_main.c:311) and C3D_FrameCounter, and FrameBegin(SYNCDRAW) waits for one more vblank. svcSleepThread is used only by the test switch fs1delay (pm_main.c:337).
- The 3DS LCD retrace is 59.83 Hz, not 60. Two vblanks are 33.43 ms. A perfect run gives 29.92 steps/s. Proof: Azahar log 3ds/logs/perf/full/cur_fast.txt line 89: 30 steps, 60 vblanks, wall 1002.7 ms, steps/s 29.92. So the best HUD value is 29.9, never 30.0.
- The HUD counts whole steps in a window that is at least 1 s. One late step (3 vblanks, 50 ms) makes the window hold 29 steps: the HUD shows 28.x to 29.0 for one second. A catch-up makes the next window 30 or 31.
- In hw_pull2, steps 340 to 804: 292 steps took 2 vblanks (33.4 ms), 163 took 3 (50 ms), 8 took 4, 2 took 7. p50 34.0 ms, p95 51.9, p99 67.9, mean 40.05 ms. 35 percent of steps were late there, so the HUD showed 25 to 29.9. Step time is a multiple of 16.7 ms (vblank steps): that is the "single digits to high double digits" scatter.
- The audio tick count per step follows the vblank count: 2 ticks = 33.9 ms mean (292 steps), 3 ticks = 49.3 ms (161 steps), 4 ticks = 64.5 ms. So a late step is a real 3-vblank step, not a counter error.
- The bottom number "frame_ms" (pm_main.c:365, 436-439) is the time of the drawn iteration only (step_start to step end). It does not contain the idle iteration. It shows 8 to 20 ms for a step whose period is 33 ms. It is a work time, not a period. Its update is skipped when the log writer holds the console lock (pm_gpu.c:882, trylock), so it can also be stale.

Fix, cheapest (pm_main.c:383-390): count vblanks. Use a window of 120 vblanks (C3D_FrameCounter(0)) and show `steps * 60.0 / vblanks_in_window`. This gives 30.0 when locked, whatever the exact LCD rate. Show the late share on the same line: `late = steps with vblank interval > PM_VBLANKS_PER_STEP` (pace_iv[2] + pace_iv[3] exist, pm_main.c:248-250) as "L12%". Cost: about 8 changed lines. Also show real steps/s only in the log (pace line already has it).

## 2. Spikes and fluctuation (build 902)

Steady distribution (steps >= 340, all three segments, 5835 steps): p50 64.0 ms, p95 96.0, p99 129.9. Per segment: seg 340-804 p50 34.0, p95 51.9, p99 67.9, max 122.2. seg 2434-5202 p50 65.9, p95 88.0, p99 149.9, max 240.2. seg 6742-9006 p50 74.2.
Dominant phase of the 3546 of 5835 steps over 50 ms: gbi 3546 (all of them are a slow scene, not a spike), au 140, tx 52, fb 36, lo 7, ot 2, ro 1.

Alternation: after a 50 ms step the next step is NOT short. Mean next step 47.2 ms (min 29.7, 150 cases). A 50 ms step plus the next step average 97 ms for 2 steps. So there is no catch-up pair. The late steps come in runs (steps 492 to 496, 802 to 803: 62 to 70 ms in a row). A normal alternation is 33 ms and 50 ms with the pair sum 83 ms: the loop gets 2 audio ticks and then 3 ticks (pm_main.c:245 pace_ticks). Not seen as a pair here, because the scene cost (gbi 13 to 19 ms + gpu + audio + fbegin wait) is near the 33 ms limit. A true spike is a step > 100 ms with one dominant phase.

Steps over 100 ms: 213 in 5835 (3.7 percent). Causes by phase (a step can be in two groups):
- tx (texture decode/upload, tx >= 50 ms): 61. 63 of the 213 have 20 or more cache misses in the step. Typical: 34 misses x 8 KB per step for 8 steps in a row (steps 2594 to 2601, w 142 to 174 ms, tx 64 to 68 ms, no ROM read), then back to 70 ms. The cost is about 2 ms per miss.
- ROM read plus Yay0 (lo + ro >= 50 ms): 6. Map and NPC spawn: steps 331 (126 ms, 6946 ROM reads, 578 KB), 3281 (124 ms, 5460 reads, 508 KB), 565, 2822 (507 reads, 195 KB).
- au (audio join wait >= 10 ms): 22 steps >= 100 ms. In steps 340 to 804, 56 steps have au >= 15 ms; mean step 55.0 ms against 38.0 ms for the rest. The audio join is the second cost of the near-30 segment. AU2 (build 926) removed the join; no data yet.
- ot (other, unattributed): 3 steps: 8020 (180 ms), 8034 (152 ms), 8043 (46 ms), each next to a "menu: open" line (menu open, not a steady state). Step 1: 6956 ms is the boot.
- fb (FrameBegin wait for the previous GPU frame) 5 to 16 ms in most steps; it never dominates.
- Not found as a cause: the 300-frame stat lines (steps at 300, 600, ... have mean 59.5 ms against 62.0 ms for all steps; none over 110 ms). Log writer: producers use a lock-free queue (pm_log.c:260-262, 358). The writer is on core 1 at priority 0x3F, fsync once per second (pm_log.c:478-482). The game thread takes the console lock only in pm_con_lock callers at boot, fatal screen and menu (pm_gpu.c:795, 853, 868, 1269), and the HUD uses trylock (pm_gpu.c:882). No step with a logstat or fsync pattern was found. logstat2 (writer max us) is not in build 902, so the writer is "not proven clear" for build 926.
- Save: no save.flash event inside a steady segment (one save.flash line in the log).
- Cache flush: GSPGPU_FlushDataCache is called per vertex pool range at pm_gpu.c:193, 197, 198 (gc phase). gc was 0.1 to 1.6 ms. Not a cause.

Texture cache: the answer to the user's question "is it the cache being flushed" is no for a flush of the whole cache. The cache has a 4 MB budget (pm_tex.c:22) and drops the oldest slot when full (pm_tex.c:309-320). It drops slots when the game writes to the memory of a cached texture (pm_tex_invalidate, pm_tex.c:449; called from every ROM read pm_os.c:386, from the map load pm_map.c:375, from the CFB pm_main.c:193, 228). texstat per 300 frames: evictions 0 in 8 windows, 433 to 696 (2.5 to 5.6 MB) in 4 windows near steps 2700 and 3600 to 3900, invalidated 130 to 447 in most windows, uploaded up to 9.7 MB per 300 frames. The windows with the most evictions are the windows with the 34-miss runs. So: partly yes, as eviction and invalidation misses (re-decode of 20 to 100 textures in one step), not as a flush.

## 3. Cause ranking

1. HUD shows 29.x and not 30.0 (proven). The LCD is 59.83 Hz (29.92 steps/s), the window holds whole steps, and late steps (35 percent in the near-30 segment) change the count. Fix: pm_main.c:383-390 as in section 1. In row HWM1: the pace line (pm_main.c:253-262, switch pacestat) already prints steps, vblanks, intervals and late steps. No new log line needed.
2. Late steps in runs (proven): scene cost near the 2-vblank limit. gbi 13 to 19 ms + audio join 5 to 29 ms + fbegin 6 to 16 ms + GPU. A step slightly over 33 ms costs 50 ms (vblank quantization). FS1 does not help here (it skips only after a late step). Cheapest: the AU2 change (already in build 926) removes the audio join part; check the pace line late-step share with build 926. Covered by HWM1 pace lines. No new line.
3. Texture miss runs (likely): 34 to 100 misses per step for several steps. Capacity and invalidation churn (4 MB budget, pm_tex.c:22; "never hit before evict" up to 264 per 300 frames means textures dropped before reuse). Cheapest: raise BUDGET at pm_tex.c:22 if free memory exists (HUD shows the free region), or keep slots when only the epoch changes. Covered by texstat lines only per 300 frames. New line needed (see below) to prove which step has which misses: log at pm_tex.c:631 only when more than 20 misses in one step, with the evict and invalidate counters.
4. ROM read and Yay0 at map and NPC spawn (proven, 6 steps): 100 to 130 ms once per spawn. Expected load work. Only a streaming fix helps. Not a steady-state cause. Covered by pc and romn columns.
5. Audio join waits (proven for build 902, probably gone in 926): au 10 to 29 ms in 12 percent of the near-30 steps. Check in the build 926 log: pc au column.
6. Log writer and console lock (guess, not seen): no evidence in 902. Covered by logstat2 in build 926 (pm_log.c:242: writer max us for file, sock, console). A max over 20 ms with a spike in the same 300 frames would show it.
7. "ot" 150 ms at menu open (proven, rare): menu code path, not steady state.

New log lines needed:
- Spike line in pm_main.c after line 382 (frame_ms known): `if (frame_ms > 50.f) pm_log_hw("spike f %u ms %.1f vb %u ticks %u texmiss %u/%uKB rom %u/%uB", frame, frame_ms, C3D_FrameCounter(0) - step_vb, pace_ticks_last, tex_miss, tex_kb, rom_n, rom_b);` with the phase values of the step (pm_ph_frame_end already has them in rec r). Without the phasecsv range, pc lines are lost to queue drops (629 to 1207 drops in build 902). This line is small (about 1 per spike) and does not flood.
- Pace window: pace line at pm_main.c:253 already has what is needed; add the 59.83 Hz note only in the HUD fix.
