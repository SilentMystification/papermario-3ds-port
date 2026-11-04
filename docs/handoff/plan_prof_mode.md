# plan_prof_mode: a low-cost profiling switch (`prof`)

Scope: plan only. No source, Azahar or git change was made to write this file.
Inputs: CLAUDE.md, inv_pmu_events.md, agent_cap2.md, agent_split0c2.md, hw_guidelines.md, memory project_console-fps-logging-off.md,
3ds/src/pm_phase.c, 3ds/src/pm_log.c, 3ds/src/pm_main.c (pace and frame loop), 3ds/src/pm_nusys.c (gbi_task), 3ds/src/pm_gbi.c (sp0, end frame).

## 1. Problem

- Logging off: 24 to 28 fps on the O3DS. Every breakdown (phase, sp0c, fsplit, pmu, pace) runs with `loglevel=debug` and timers on, so it is slower than play.
- The `pace` line (pm_main.c:305) and `pm_log_hw` (pm_log.c:351) are debug level (3). They need `loglevel=debug`, which also lets every other debug line through.
- The `phase` switch reads a tick at each PH_PUSH and PH_POP (pm_phase.c:331-346) and at many per-op sites (sp0_kind at pm_gbi.c:3570, flush sites at 2901 and 3092). About 1.3 us per tick read on Azahar.
- The `pmu` switch toggles the counter set every 300 frames (pm_phase.c:259-272). Window A (I miss, D read miss) and window B (TLB miss, stall) never cover the same frames.

## 2. Design

One switch `prof`. It runs three parts:

1. Every drawn frame (clean frames): zero new tick reads, zero svc calls. The main loop already reads the tick twice per iteration (pm_main.c:366 `now`, kept as `step_start` at 406, and pm_main.c:489 `now` after the frame). `pm_pf_frame(step_start, now)` takes both values. Wall time of a drawn frame = `step_start` minus the `step_start` of the last drawn frame. Work time = `now - step_start` (no idle wait).
2. One drawn frame in N (sampled frame, default N = 30): `pm_ph_on` is set to 1 for that frame only. All the existing phase timers run in that frame (PH stack, gbi_task tick pair, sp0 tex/dlc/rest split, texture timers). No new timer site in the hot code. The other N-1 frames run with `pm_ph_on` 0, the same as a log-off build.
3. PMU: one fixed counter set for the whole run (no window toggle). The counters are read only at the two edges of each sampled frame. So the clean frames and the sampled frames get separate counts, and the phase code does not pollute the clean I-cache counts.

The prof line prints at warn level (`pm_log_w`, level 1). `loglevel=debug` is not needed. The push uses only `prof`: no debug line, no other stat switch.

### 2.1 How often the counters can be read (requirement 3)

- `svcControlPerformanceCounter(&out, PERFCOUNTEROP_GET_VALUE, reg, 0)` is one SVC per register. The API has no rate limit. A read of PMN0, PMN1 and CCNT is 3 SVC calls.
- A read does not reset the counter. Repo evidence: CAP2 `pm_pmu_sp0` (pm_phase.c:275-283) reads in the middle of a pmu window, and `pmu_frame_end` (pm_phase.c:259-272) reads the same window later. Only `PERFCOUNTEROP_SET_VALUE 0` resets a counter (pm_phase.c:246-248).
- Not checked: the libctru header is not on disk (inv_pmu_events.md line 11). The no-reset rule is from repo use and the libctru API, not from the header.
- Counter width is not checked (32 bit raw or 64 bit kernel value). 300 frames at about 40 ms are 3.2G cycles, close to 2^32 (4.29G). The plan takes each delta as u32 over one interval (at most N frames, about 1.3 s, far below 16 s). It adds each delta to a u64 window sum. So a 32-bit counter is safe.
- The counters are per core. The svc reads the core that runs the game thread (core 0). The audio job runs on core 1 (AU1d) and is not counted.
- The hardware allows a read every frame (3 SVC, estimated 3 to 6 us per frame on the console). Per-frame reads give the same window sums. The plan reads 2 times per N frames to split clean and sampled frames. A per-frame read is an open decision (section 8).

### 2.2 Counter set (two event counters only)

PMN0 and PMN1 are the only event counters. CCNT counts cycles. The user asks for three quantities (stall share, I miss, D miss), so one run cannot hold all three.

- `prof` (set `m`): PMN0 = CORE_INST_CACHE_MISS (ev 0), PMN1 = CORE_DATA_CACHE_READ_MISS (ev 11).
- `prof` with `profstall` (set `s`): PMN0 = CORE_STALL_BY_LACK_OF_INST (ev 1), PMN1 = CORE_DATA_CACHE_READ_MISS (ev 11). `pm0` is then the instruction-fetch stall share in permille.
- `prof` turns the `pmu` switch off (`pm_pmu_on` 0), so the two never change the set under each other. The sp0c pmu fields then print 0 (existing behavior with pmu off).

### 2.3 Switch (requirement 5)

- Words: `prof` (N = 30) or `prof=<N>` (4 to 255). `profstall` picks set `s`. Default off.
- Parsed once in `pm_ph_init` with the existing `pm_debug_has` and `pm_debug_value` pattern. `pm_debug_has("prof")` does not match `prof=16` (debug_scan needs a word edge, pm_log.c:105-108), so both calls are needed.
- `pm_pf_on = want && pm_log_file()`. `pm_ph_log_gate` (pm_phase.c:288) sets it again when the options panel turns the log on or off.
- The main loop reads `pm_pf_on` once per iteration: `if (pm_pf_on && !last_step_skipped) pm_pf_frame(step_start, now);`. No per-op read of any prof flag.

### 2.4 No picture change (requirement 7)

- The mode reads ticks, the PMU and the existing phase sums. It writes only its own statics and `pm_ph_on`.
- `pm_ph_on` gates measurement only (timers, counters, log lines). The phase switch already runs it on for every frame with the same pictures.
- The PMU writes (ENABLE, SET_EVENT, SET_VALUE 0) happen once at init. They are CPU monitor registers, not game state.

### 2.5 Safety of the pm_ph_on flip

- The flip is at one point: `pm_pf_frame`, after pm_main.c:490. That is after `pm_audio_join_at(3)` (pm_main.c:484), so the core 1 audio job is not running. `pm_au_tk()` (pm_os.c:151) reads `pm_ph_on` live, and its caller caches it per job (pm_os.c:158).
- Sites of the form `t0 = pm_ph_on ? pm_ticks() : 0; ... if (pm_ph_on) x += pm_ticks() - t0;` (pm_main.c:240, 260, 582, pm_gpu.c:549, 2220, pm_tex.c:553, 985, pm_depth.c:14) open and close inside one iteration, never across pm_main.c:490. The HUD site (pm_main.c:582-584) comes after the flip and reads the new value both times.
- At arm time the phase stack is reset: `last = pm_ticks()`, `acc[]` cleared, `depth = 0`, `cur = PH_OTHER`, `frame_t0 = last`. Without this, the first push adds the time since the last sample to `acc[PH_OTHER]`.
- `pm_tex.c:815` caches `tc_on` from `pm_ph_on` on its first call. The first texture call is at boot, before the first sample (frame N), so `tc_on` stays 0. Row PF2 checks this with one grep. If the first call can fall in a sample, the row starts sampling at drawn frame 300.
- pm_gbi.c:2390 runs `os_calibrate`, `mdc_frame_end` and the 300-frame reports when `pm_ph_on` is 1. In prof mode this guard gets `&& !pm_pf_on`. So no calibration cost lands in a sample and no opsplit, mops, mdc or sp0c line prints. `pmk_n` resets itself at a new list (pm_gbi.c:1959). `mdcm_n` stops at MDC_MARKS (pm_gbi.c:2131). Row PF2 checks that the os_ per-op sampling needs no `os_calibrate` value to run.
- pm_gbi.c:2389 (dcsum frame) stays. It clears `pm_dc_c` each sampled frame and prints one dcsum line per 300 sampled frames (about one per 5 minutes at N = 30).

## 3. The prof line (exact)

One line per window of 300 drawn frames, at warn level:

```
pm_log_w("prof f%u w%u fps %.1f | ft %.2f max %.1f o34 %u wk %.2f | s%u fts %.2f wks %.2f oh %.2f amort %u | gbi %.2f dlc %.2f tex %.2f gpu %.2f | pmu %c %u %u cyc %u pm %u %u | st %u vb %u iv %u %u %u %u", ...);
```

Example (made-up numbers, for the shape only):

```
prof f9000 w300 fps 26.4 | ft 37.80 max 61.2 o34 141 wk 35.10 | s10 fts 46.20 wks 43.90 oh 8.80 amort 293 | gbi 14.20 dlc 2.10 tex 3.40 gpu 5.10 | pmu m 150863 54478 cyc 9900000 pm 15 5 | st 330 vb 682 iv 0 241 52 7
```

| Field | Meaning | Source |
|---|---|---|
| f | drawn frames since boot at the window end | prof counter |
| w | drawn frames in the window (300) | prof counter |
| fps | w x 60 / vb | window end, cold |
| ft | mean wall ms of a clean drawn frame (step_start to next drawn step_start) | main loop ticks |
| max | largest clean wall ms | main loop ticks |
| o34 | clean frames with wall over 34 ms | main loop ticks |
| wk | mean work ms of a clean frame (step_start to pm_main.c:489, no idle) | main loop ticks |
| s | sampled frames in the window (about w / N) | prof counter |
| fts, wks | mean wall and work ms of a sampled frame | main loop ticks |
| oh | wks - wk: ms that the full timers add to one sampled frame (self-check) | window end |
| amort | prof cost in us per frame: oh x 1000 / N + base (base from the cal line) | window end |
| gbi, tex, gpu | mean exclusive ms per sampled frame of PH_GBI, PH_TEX, PH_GPU | Rec r->ms at pm_phase.c:502 |
| dlc | mean ms per sampled frame of dlc replay inside G_PM_MODEL groups (sp0 kind 2, a part of gbi time) | delta of sp0_T[2] |
| pmu c | counter set: m or s | switch |
| pmu e0 e1 | PMN0 and PMN1 counts per clean frame | PMU edges |
| cyc | CCNT per clean frame | PMU edges |
| pm | e0 x 1000 / cyc and e1 x 1000 / cyc (set s: pm0 = stall share in permille) | window end |
| st | game steps in the window (pm_step_calls delta) | pm_phase.c:526 |
| vb | vblanks in the window (C3D_FrameCounter(0) delta) | window end |
| iv | drawn frames by vblank interval 1, 2, 3, 4+ (pm_ph_iv, pm_main.c:450) | per frame bin |

`st - w` is the count of skipped draws (FS1). The pace line fields (steps, drawn, vblanks, intervals) are in the prof line, so `pacestat` and `loglevel=debug` are not needed.

Calibration line, once at init (cold, boot time):

```
pm_log_w("prof cal every %u set %c tick %.2f us pmu %.2f us base %.2f us rc %08x", N, set, tick_us, pmu_us, base_us, rc);
```

- tick_us: mean of 200 `pm_ticks()` reads. pmu_us: mean of 50 GET_VALUE reads (one register). base_us = 6 x pmu_us / N (two 3-register reads per N frames). The pattern is pm_phase.c:310-321.
- The self-check is `oh` and `amort` in each window, plus `base` here. The user sees the cost of the mode in the same line as the numbers.

## 4. Work list and cost (estimated, not measured)

Estimates use 1.3 us per SVC (Azahar tick read, agent_split0c2.md). A console SVC is not measured here.

| When | Work | Cost (estimate) |
|---|---|---|
| Every iteration | load `pm_pf_on`, branch | under 0.01 us |
| Clean drawn frame | 2 u64 subtractions of ticks the loop already has, sums, max, compare to 34 ms in ticks, iv bin (clamp, increment), countdown | about 0.2 us, 0 SVC |
| Arm (1 in N) | PMU read (3 SVC), clean-interval deltas into u64 sums, phase stack reset, `pm_ph_on = 1` | about 4 to 6 us |
| Sampled frame | all existing phase timers for one frame | unknown, measured by `oh`. Guess 3 to 10 ms (phase-on runs were slower than play by that order) |
| Sample end | divert in pm_ph_frame_end (Rec already built, 22 float conversions), take 4 ms values and the dlc delta, PMU read (3 SVC), `pm_ph_on = 0` | about 6 to 8 us |
| Window end (1 in 300) | 6 divisions, 1 C3D_FrameCounter, snprintf, queue | about 50 to 100 us |

Per frame, amortized at N = 30: about 0.2 us (clean) + 0.4 us (edges) + 0.3 us (window) + oh / 30. With oh = 3 to 10 ms: 100 to 330 us per frame, which is 0.3 to 1.0 percent of a 33.4 ms frame. With `prof=120`: 25 to 85 us per frame. No division and no double in per-frame code (window end and init only, cold).

## 5. Files and anchors

| File | Anchor | Change |
|---|---|---|
| 3ds/include/pm_port.h | after line 82 (`pm_ph_frame_end`) | `extern int pm_pf_on; void pm_pf_frame(unsigned long long t0, unsigned long long t1);` |
| 3ds/src/pm_phase.c | after line 283 (end of `pm_pmu_sp0`) | prof block: statics, `pf_init`, `pm_pf_frame`, `pf_take`, `pf_pmu_read`, cold `pf_report` |
| 3ds/src/pm_phase.c | line 254 (`pm_pmu_on = ...`) | add `&& !pf_want` |
| 3ds/src/pm_phase.c | lines 288-294 (`pm_ph_log_gate`) | `pm_pf_on = on && pf_want;` and, when prof is on, `pm_ph_on` is set only by prof |
| 3ds/src/pm_phase.c | line 308 (`pmu_init();`) | `pf_init();` after it |
| 3ds/src/pm_phase.c | line 517 (before `if (ps_on) ps_step(r);`) | `if (pf_samp) { pf_take(r); nwin = 0; return; }` |
| 3ds/src/pm_main.c | after line 490 (`frame_ms = ...`) | `if (pm_pf_on && !last_step_skipped) pm_pf_frame(step_start, now);` |
| 3ds/src/pm_gbi.c | before line 2158 (`sp0_report`) | `unsigned long long pm_gbi_sp0_dlc(void) { return sp0_T[2]; }` |
| 3ds/src/pm_gbi.c | line 2390 | `if (pm_ph_on && !pm_pf_on) { ... }` |

Note: `pm_ph_frame_end` (pm_main.c:472) runs before `pm_pf_frame` (after 490) in the same iteration. pf_take stores the sample. pm_pf_frame then reads the PMU and clears `pm_ph_on`. `pm_gbi_win_reset` (map load) zeroes sp0_T, so the dlc delta uses `cur < prev ? cur : cur - prev`.

Not touched: the `phase`, `phsum`, `fsplit`, `dcsum`, `pmu` and `pacestat` paths. No old path is removed (requirement 8). `prof` with `phase` on: prof does not arm (phase already owns `pm_ph_on`), and the sampled fields print 0.

## 6. Rows (one epic, one code agent, one branch)

| Row | Files | Lines | Content | Check |
|---|---|---|---|---|
| PF1 | pm_port.h, pm_phase.c, pm_main.c | about 50 | switch parse (`prof`, `prof=<N>`, `profstall`), `pm_pf_on` and gate, `pm_pf_frame` clean-frame sums (ft, max, o34, wk, iv), window line with st, vb, fps; sampled and pmu fields print 0; cal line with tick_us only | build, no warning on changed lines |
| PF2 | pm_phase.c, pm_gbi.c | about 40 | arm and disarm of `pm_ph_on` (stack reset), divert in pm_ph_frame_end, fts, wks, oh, amort, gbi, dlc, tex, gpu; dlc accessor; guard at pm_gbi.c:2390; grep checks of section 2.5 (tc_on, os_ sampling) | build |
| PF3 | pm_phase.c, azt_scenarios.json, test_catalog.md | about 40 code | PMU: init (ENABLE, SET_EVENT set m or s, SET_VALUE 0 x3), read at arm and at sample end, u32 deltas into u64 clean sums, pmu fields, pmu_us and base in the cal line, `pm_pmu_on` exclusion; new scenario `mac01t_prof` | the Azahar start below |

Each commit message states the hw_guidelines rules: no per-op site, no per-frame SVC, no division or double outside cold code, prof block cold except `pm_pf_frame`.

## 7. Test

Affected scenario: only `mac01t_prof` (new). No other scenario: no draw code changes, and pm_gbi.c changes only a measurement guard.

- Scenario `mac01t_prof` (added in PF3 to 3ds/tools/azt_scenarios.json and the docs/handoff/test_catalog.md table, same commit). Copy of `mac01t_cap1` with debug `warp=mac_00:4 story=ch0_toad_town tour=mac_00:4,mac_01:0 shots shotwin=500:1 prof=16`. No `loglevel=debug`, no `phase`. until `prof f\d+ w300 `. Subject: Mario in mac_01 at frame 500 and one prof line.
- One Azahar start after PF3: `PM_ROW=PF3 powershell -File 3ds/tools/azt.ps1 mac01t_prof -Row PF3 -Try`. Pass: the cal line and one prof line print, `s` is about 18 (300 / 16), gbi and tex are not 0, the shot shows Mario in Toad Town. On Azahar the pmu fields may be 0 (Azahar may not model the PMU). That is not a fail. Azahar times show direction only.
- One console push after the epic passes: `PM_ROW=PUSH sh /c/Users/Andrew/Documents/GitHub/papermario-3ds-port/3ds/push_3ds.sh --no-build --ip 192.168.1.170 prof`. Read the cal line, `fps`, `ft`, `oh`, `amort`, and pmu `m`. A second push with `prof profstall` gives the stall share.

## 8. Open decisions for the user

1. N default: 30 (about 1 percent cost if oh is near 10 ms) or 120 (lower cost, 2 to 3 samples per window, more noise in gbi and tex).
2. Counter set: fixed per run (`m` default, `profstall` for `s`), or alternate m and s every window like `pmu` (all three quantities in one run, but each window has only one set).
3. PMU read rate: at the sample edges only (plan), or every frame (3 SVC per frame, estimate 4 to 6 us, gives a per-frame max of misses).
4. Window: 300 drawn frames (about 11 s at 27 fps) or a fixed time such as 5 s like the old pace line.
5. Level: warn (plan, works with the default loglevel) or a new VIP warn level so the queue keeps the line longest. The log writer thread still runs with the log on. The user can compare the HUD fps of `prof` with `log=off` on the console to see the cost of the log on at warn level.
6. The frame after a sampled frame may run slower (the phase code evicts I-cache lines). It counts as clean. A rule that skips it from ft is 2 lines, if the numbers show the effect.
7. fps60 (`pm_interp_on > 0`): key frames draw in the idle iteration. The plan does not count them. Not in scope unless the user runs fps60.
