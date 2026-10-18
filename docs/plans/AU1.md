# AU1 plan: audio off core 0

Written 2026-10-18. Read-only planning. No build. No Azahar run. Row AU1 in 3ds/SLICES.md.
User workload split: core 0 = logic plus the CPU part of the RSP work. Core 1 = audio. PICA = rasterization plus vertex work. Consider ndsp for mixing.
Tags: MEAS = measured (Azahar, docs/HISTORY.md). EST = estimate. HW = only the O3DS can settle it.

## 1. Current thread model

| Part | Where | What it does |
|---|---|---|
| `nuAuMgr` (src/audio/core/system.c:112) | thread `au_thread`, `threadCreate(..., prio = main prio, core -2)` in `pm_au_thread_start` (3ds/src/pm_audio.c). Core -2 = the default core = core 0. | Decomp audio manager. Waits for the retrace message, calls `alAudioFrame`, sends the audio task, posts PCM with `osAiSetNextBuffer`. |
| Driver `pm_audio_tick` (3ds/src/pm_os.c:124) | game thread, core 0, top of each loop iteration (3ds/src/pm_main.c:271) | Sends the retrace message. Resumes the audio thread up to 8 times. Plays the RSP and scheduler role: it answers each audio task at once. Logs `audio cost avg/max us per 60 Hz tick`. |
| Tick count `pm_audio_retraces` (pm_audio.c) | game thread | Wall clock: 60 Hz retraces since the last call, at most 3. |
| RSP microcode | `pm_rsp_audio.c` in C, called directly by the `a*` macros (3ds/include/pm_rsp_audio.h) | The mix runs inside `alAudioFrame` on the audio thread. No command list is built. |
| Output | `pm_audio_post` (pm_audio.c) | One ndsp channel, stereo PCM16, 32 kHz, 8 linear buffers of 8 KB. Full queue = the buffer is dropped. |
| Log writer | core 1, prio 0x3F, after `APT_SetAppCpuTimeLimit(30)` (3ds/src/pm_log.c:379) | Proof that core 1 is usable in Azahar. HW not proven. |

The custom yield (to review):
- `osRecvMesg` (pm_os.c:78) loops on an empty queue and calls `pm_au_thread_yield`. The yield releases `au_idle` and waits on `au_run` (two LightSemaphores). `pm_au_thread_resume` does the opposite on the game thread.
- Result: strict lockstep. Only one of the two threads runs. The audio thread gives no parallel work today. It is a coroutine on core 0.
- The message queues (`osSendMesg`, `osRecvMesg`) have no lock and no barrier. They are safe only because of the lockstep.
- The yield is not a sleep loop and not a hand-rolled lock. It uses libctru natives. It is allowed, but it costs 2 thread switches per resume and it blocks core 0 for the full audio frame.
- The yield also holds the class E crash tests `crashtest_audio` and `crashtest_audio_call`. Keep them.

Review verdict: keep the coroutine idea (nuAuMgr is decomp code that blocks, it must stay a thread). Move the driver role into the yield on the audio thread (slice AU1c). Then the game thread only kicks and joins.

## 2. Cost of the N64 synth per tick

| Item | Value | Tag |
|---|---|---|
| Output rate | 32,000 Hz (`HARDWARE_OUTPUT_RATE`) | code |
| Samples per 60 Hz tick | 552 (`AlFrameSize`), 368 for a short frame (`AlMinFrameSize`) | code (frameSize 534, rounded to 184-sample blocks) |
| Physical voices | 24 (`config.num_pvoice`) | code |
| Effect buses | 4 (`config.num_bus`), types smallroom, bigroom, chorus, flange, echo, custom 0 to 3 | code (src/audio.h:168) |
| Work per voice per 184-sample block | VADPCM decode, resample, envelope mix, bus mix | code |
| Work per bus | delay line taps: load, save, optional resample (chorus, flange), pole filter | code (src/audio/core/reverb.c) |
| Cost in Azahar | avg 3,000 us (up to 4,000 us), max 30,000 us per tick. 18 to 24 % of a 16.7 ms tick | MEAS, not valid for HW |
| Cost per 33.3 ms game step (2 ticks) | about 6,000 to 8,000 us on core 0 | EST from MEAS |
| Cost on the O3DS | unknown. ARM11 268 MHz, no NEON. Can be slower than Azahar | HW (S28) |
| The 30,000 us max | is likely a bank or song load (`au_read_rom`) inside the tick, not the mix | EST, AU1a splits it |

## 3. Options

| Option | What | Gain on core 0 | Exact output | Memory | Effort | Verdict |
|---|---|---|---|---|---|---|
| A. N64 mixer on core 1 | Same decomp and `pm_rsp_audio.c`. The audio thread moves to core 1. It runs while core 0 interprets the display list, submits to the GPU and waits for vblank. | all audio cost leaves core 0 (6 to 8 ms per step EST) | yes, bit exact (same code, same order) | about +1 KB code. Stack: the 64 KB audio stack exists. No linear change. | small | DO FIRST |
| A+. A plus ARMv6 SIMD kernels | `SMLAD`, `QADD16`, `SSAT` in the hot loops of `pm_rsp_audio.c` (resample, envmixer, mix, ADPCM) | less core 1 time | must stay bit exact (hash) | about +2 KB | medium | ONLY IF core 1 time > budget (section 7) |
| B. Voices to ndsp channels | 24 pvoices to 24 ndsp channels. Pitch by `ndspChnSetRate`, volume and pan by `ndspChnSetMix`, loops by wave buffers. | mix and resample go to the DSP | NO. ndsp ADPCM is the DSP-ADPCM format, not N64 VADPCM (9-byte frames, vector predictor). Envelope ramps, pan law and the effects differ. | sample data must be in linear memory. The resident ROM is in the newlib heap, not linear. Copy as PCM16 = 3.56 x the VADPCM size. Re-encode to DSP-ADPCM = lossy plus load time. EST 1 to 4 MB linear, the pool margin is 2,712 KB | large, touches shared decomp code (`syn_driver.c`, `voice.c`) | RESEARCH ONLY, plan with opus if A and A+ fail |
| B-fx. Effects in ndsp | ndsp has no reverb. The aux buses only give a CPU callback (`ndspAuxSetCallback`). `reverb.c` would run on the CPU anyway. | none for effects | no | small | large | REJECT |
| C. ndsp for the final mix only | One stereo channel, the DSP does the last resample and output. | this is the current state | yes | 64 KB linear (8 x 8 KB) | done | KEEP |
| D. Own DSP firmware | soh-3ds `research/dsp/synth-probe`: a Teak kernel for mixing. | in theory all of the mix | yes if exact | DSP memory | very large | REJECT. First O3DS run failed at the DSP cache flush (`e0e3fff6`). CIA only. Not a 3dsx path. |

Reference ports:
- sm64-3ds `src/pc/audio/audio_3ds.c`: audio thread on core 1 (O3DS) or core 2 (N3DS), `APT_SetAppCpuTimeLimit(80)`, fallback to the main thread. One ndsp channel. Same as option A.
- soh-3ds `platform/3ds/source/audio_runtime_3ds.cpp`: worker on core 1 with `LightEvent` start and done. It starts the job during render and waits before the next logic tick ("emulated N64 message queues are never accessed concurrently"). It tries limits 80, 70, 50, 30. Fallback to synchronous synth. This is the model for AU1.

## 4. The overlap window

The audio state is shared with game logic (sound queue `au_sfx_enqueue_event`, BGM player fields, `AuGlobals`). The audio thread must never run at the same time as game code. Only the port layer may run in parallel.

| Point | Where | Why |
|---|---|---|
| Kick (start audio on core 1) | entry of `nuGfxTaskStart` for the main task (flag `NU_SC_SWAPBUFFER`, src/main_loop.c:251), before `pm_gbi_run` | game logic and the display list of this step are complete. After it, `gfx_draw_frame` only flips an index and calls an empty shim. AU1b must read `gfx_draw_frame` and confirm. |
| Join (core 0 waits for audio done) | entry of `__wrap_step_game_loop` (3ds/src/pm_phase.c:171) and before `pm_warp_tick` and `pm_map_tick` (pm_main.c:310) | the next game code that can touch audio. Warp and map tick can call game code (`set_game_mode`, songs). |
| In the window on core 0 | `pm_gbi_run` (CPU part of the RSP work), `pm_gpu_end`, the non-draw iteration, the vblank wait | port and GPU work only |

Tick count: at the kick, take `n = pm_audio_retraces()` (normally 2 per step). The non-draw iteration does no audio tick. The two ticks run back to back once per step.

Port code that the audio thread reaches and that is not thread safe:
- Phase timer stack `pm_ph_push` (`PH_PUSH(PH_ROM)` in the ROM read, pm_os.c:274). `pm_gbi_run` uses the same stack at the same time. Fix: `PH_*` does nothing off the game thread. Count audio in its own counter.
- ROM read path (`nuPiReadRom`, remap): in the window only the audio thread reads the ROM. AU1c must list its static state.
- `pm_log`: lock-free multi-producer queue (L01). Safe.
- ndsp calls: only the audio thread calls them after AU1d (`osAiGetLength` is called on the audio thread). Safe.
- FPSCR mode: `pm_fpscr_setup("audio")` runs on the audio thread. Keep it there.

## 5. Latency

| Stage | Now | After AU1d | Tag |
|---|---|---|---|
| Sound event in step N to the first tick that reads it | 0 to 16.7 ms (top of the next iteration) | 0 ms (kick in the same step, after logic) | code |
| nuAuMgr: frame built at retrace k, posted at retrace k+1 | +16.7 ms | +16.7 ms | code |
| ndsp queue | 1 to 3 frames (17 to 50 ms) | 2 to 4 frames (2 frames posted at once) | EST |
| Total | about 33 to 83 ms | about 33 to 83 ms | EST |

The `osAiGetLength` feedback (frame size 552 or 368) keeps the queue short. No change.

## 6. Memory

| Item | Option A | Option B |
|---|---|---|
| Code | about +1 KB (kick, join, yield driver), +1 KB for AU1a switches | large, not estimated |
| Stack | 0 (the 64 KB audio stack exists). The second semaphore goes away. | 0 |
| Linear pool | 0 | EST 1 to 4 MB sample copies (margin 2,712 KB) |
| Heap | 0 | transcode buffers |

Option A keeps the 10 KB per slice rule in every slice.

## 7. Measure on the O3DS (S28)

S28 runs the current lockstep path. It gives the baseline. AU1 does not wait for S28 to start AU1a to AU1d. AU1e uses the S28 numbers.

| S28 must record | Source | Scenes |
|---|---|---|
| `audio cost avg / max us per 60 Hz tick` | game log, switch `audiostat` (line every 2 s) | title (music), `preset=toad` (music and sound), a battle (`battle`) |
| Phase split per step: logic, dl, gbi, gpu, idle, audio (ms) | switch `phase phasecsv`, `3ds/tools/phase_report.py` | same |
| Steps per second | switch `pacestat` | same |
| Core 1 usable | log lines `log: APT_SetAppCpuTimeLimit(30) = ...` and `log writer core 1` | boot |
| Crackle or gaps | user ear check, plus the AU1a underrun counter if AU1a is in the build | same |
| N3DS same lines | for comparison | same |

Decision rule (EST thresholds, for AU1e):
- Core 1 budget at limit 30: 30 % of 33.3 ms = 10 ms per step = 5 ms per tick.
- Audio avg <= 5 ms per tick: option A at limit 30.
- 5 to 13 ms per tick: option A with a higher limit (50, then 80, as sm64-3ds and soh-3ds), then A+.
- Above 13 ms per tick (80 % of the budget): A+ first. Option B research only if A+ fails. Lower quality (fewer voices, lower rate) needs the user approval.
- If core 0 still exceeds 33.3 ms per step after audio leaves it, the next work is PO2 and PO3, not audio.

## 8. A/B check

Option A runs the same code in the same order. The PCM output must be bit exact.
- Switch `audiohash` (AU1a): FNV-1a of every buffer that `pm_audio_post` receives (hash before the full-queue drop). One log line per 120 posts and one at smoke end.
- Switch `audiolock` (AU1a): exactly 2 ticks per drawn step (not the wall clock), and `osAiGetLength` returns 0 (always 552-sample frames). This removes the two time-dependent inputs. Without it the hash is not deterministic.
- Baseline B0: `-Turbo -Smoke -Debug "fast preset=toad audiohash audiolock"`, the same for `yard` and one `battle`. Run twice. Same hashes = deterministic.
- Each later slice must give B0 exactly. Run the threaded path 3 times (a race can pass once by luck).
- Ear check: the user on hardware (AU1e). No WAV dump now. Add one only for option B.

## 9. Risks

| # | Risk | Effect | Control |
|---|---|---|---|
| R1 | Game code runs while the audio thread runs (a missed path into `snd_*`, `bgm_*`, `au_*` outside the window) | rare data race, wrong sound or a crash, not reproducible | strict window (section 4), join at `step_game_loop` and warp or map tick, hash 3 runs, tour subset with battles |
| R2 | Core 1 on the O3DS gives less time than the limit says (system threads, time-slice granularity unknown) | job wall time > window, core 0 waits at the join, underruns | measure job wall time and join wait (AU1d line), limit 30 to 80 (AU1e), fallback path on core 0 |
| R3 | nok_01 fault: unmapped access in `au_syn_begin_audio_frame` (src/audio/core/engine.c:488, `au_syn_start_voice_params` with `voice->instrument`) | an existing fault moves to core 1. It can become timing dependent and harder to trace. | AU1d runs `warp=nok_01:0` once on the old and the new path and records the same fault (function, address). AU1 does not fix it (V09 is shelved by the user). Partial edits are on local branch `shelf/v09-nok01` (src/audio/bgm_player.c, engine.c). Not merged, not verified. Do not use them. |
| R4 | Non-thread-safe port code on the audio thread (phase stack, ROM read statics) | wrong phase numbers, or a corrupt ROM read state | AU1c fix (PH off the game thread), AU1c lists the ROM read statics |
| R5 | Crash on core 1 not caught | no crash trace | AU1d check: `crashtest_audio` gives a trace that names core 1 |
| R6 | Fallback path differs from the core 1 path | two code paths to test | one code path: the fallback only changes the core id (0) and joins at once |
| R7 | Option B: accuracy loss and large linear memory | not N64 sound, memory | research only |

## 10. Slices

Rule for all: no stubs, libctru natives only (threadCreate, LightEvent, LightSemaphore, svc), at most 10 KB per slice, smoke on `yard` and `toad` with `-Turbo -Smoke`, the normal chain `-Debug "fast autoa freshsave" -Stall 40` after a slice that changes the loop. Model tier by the MODEL RULE (audio code = sonnet, measurement table = haiku, plan = opus).

1. **AU1a** (sonnet). Add switches `audiohash` and `audiolock` (section 8). Add to the `audiostat` line: underrun count (pending bytes 0 at post), dropped posts (queue full), and per-op us (ADPCM, resample, envmixer, mix, effects, ROM read inside the tick). Check: 2 runs each of `toad`, `yard`, one `battle` with `audiohash audiolock` give the same hashes. Record B0 in docs/HISTORY.md. Size in bytes in the result.
2. **AU1b** (sonnet). Move the tick call from the top of the loop to the kick point (entry of the main `nuGfxTaskStart`, section 4), still synchronous on core 0. Read `gfx_draw_frame` and confirm that no game code runs after the kick. Check: new hash B1 is deterministic in 2 runs (B1 can differ from B0, the tick position changed). From here B1 is the reference. Smoke, normal chain.
3. **AU1c** (sonnet). Review and replace the custom yield. The driver role (send retrace message, answer audio tasks) moves into `pm_au_thread_yield` on the audio thread. The game thread gets `pm_audio_kick(n)` and `pm_audio_join()` (LightEvent start and done). Remove the `au_run` / `au_idle` pair and the resume loop. `PH_*` does nothing off the game thread. List the ROM read statics. Still core 0, join right after kick. Check: hash == B1 in 2 runs. `audio cost` line still printed (kick to done).
4. **AU1d** (sonnet). Create the audio thread on core 1 (the log already sets `APT_SetAppCpuTimeLimit(30)`). Move the join to `__wrap_step_game_loop` and before `pm_warp_tick` / `pm_map_tick`. Fallback: core 0 with join at once. Add `audio wait` (core 0 join wait us) and job wall time to the `audiostat` line. Check: log `threads: audio on core 1`, hash == B1 in 3 runs, smoke, normal chain, a tour subset with battles, `crashtest_audio` trace names core 1, `warp=nok_01:0` gives the same fault as before (R3).
5. **AU1e** (haiku, needs the user and the O3DS). Push AU1d with `push-o`. Record the section 7 lines plus `audio wait`, job wall time, underruns, and the user ear check. Table in HANDOFF.md (3 lines) and docs/HISTORY.md. If `audio wait` > 0 at limit 30, try 50 and 80 (one constant in pm_log.c) and record each. A haiku BLOCKED or unclear result: relaunch on sonnet.
6. **AU1f** (sonnet, ONLY IF AU1e shows core 1 time above the budget at limit 80). ARMv6 SIMD in the hot `pm_rsp_audio.c` loops, worst op first (AU1a per-op numbers). Check: hash == B1 (bit exact), per-op us lower in Azahar, then on the O3DS.
7. **AU1g** (opus, plan only, ONLY IF AU1f is not enough). Design for option B (ndsp voices, VADPCM transcode, linear memory, effects on aux). Output: a plan file in docs/plans. No code.

Slices 1 to 4 run in Azahar without the user. Slice 5 needs the user. Slices 6 and 7 are conditional.
