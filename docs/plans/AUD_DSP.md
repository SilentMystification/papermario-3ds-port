# AUD-DSP design: audio mixing on the 3DS DSP (ndsp)

Written 2026-10-21 (lane B). Tags: M = measured (Azahar, this session), C = read in code, E = estimate, H = only the console can settle it.
A5: the DSP path is the only audio path at every boot, there is no `dspaudio` switch and no old CPU mixer output (the sections below that name the switch describe the work before A5). If ndsp or the DSP init fails the game logs one error and runs without sound. The AU2 queue is the base of the DSP path and stays. Test switches that tune the DSP path stay: `dspfx`, `dspgain`, `dspflush`, `dspaux`. Logs of the measurements: 3ds/logs/perf/auddsp/a1_tour.txt, a1_bank.txt, a1_ws.txt.

## 1. Summary

1. The game uses at most 14 voices of 24 (M). The average is 4 to 8. Every sample is VADPCM (M: 0 raw16 in 72 banks). Two effect buses are in use: bus 0 (BGM, small room) and bus 1 (SFX, custom 0, mostly an empty parameter set). Buses 2 and 3 are never used (M).
2. The set of samples that a 10 s window plays is small: 9 to 16 unique samples, 71 to 178 KB of VADPCM (M). As PCM16 it is 253 to 633 KB.
3. Recommended codec: decode the VADPCM on the CPU once, when a sample first plays, into a PCM16 cache in linear memory (budget 1.5 MB). No offline tool and no SD library are needed. The decode is the exact N64 decode. The ndsp channels play mono PCM16.
4. ndsp does the resample (pitch), the pan and the mix. The N64 control side stays. The AU2 command queue becomes the bridge: a timeline thread on core 1 replays the commands in time order and writes the ndsp channel parameters once per DSP frame (4.9 ms).
5. Effects: the two aux outputs of ndsp carry the wet sends. A small CPU reverb in the aux callback processes them. The result returns through one reserved stereo PCM16 channel (channel 23). The game then has 23 voices (it used 14 at most).

## 2. What the N64 engine does per tick (C)

- Tick = 60 Hz retrace, 552 samples at 32000 Hz, 3 render blocks of 184 samples (`AUDIO_SAMPLES`). Two ticks run back to back in each 30 FPS step (AU1d kick and join).
- Control side per tick (`alAudioFrame`): sequencers (BGM A, B, C, SFX, ambience), `au_update_voices` (envelope step per voice), `au_syn_begin_audio_frame` (writes `au_syn_*`). It stays unchanged.
- Voice (`AuVoice`, 24 slots): instrument (one sample per patch), pitch ratio, volume, pan 0 to 127, reverb amount (fxMix 0 to 127), bus, delta (ramp length in samples), priority. The engine allocates and steals voices by priority inside the 24 slots (`au_get_free_voice`-style code in `engine.c`).
- Sample: VADPCM, 9 bytes for 16 samples, codebook (predictor) per instrument, optional loop (start, end, count; count -1 = endless) with a stored loop state (`ADPCM_STATE`). `Instrument.wavData` is a ROM address. The mixer reads it through the DMA cache. The BK header and tables are in RAM.
- Mixer per voice and block (`au_pull_voice`): decode, resample (ratio = sample rate / 32000 times pitch), envelope mix. Volume is `SQ(vol) >> 15`. The left and right gains are `volume * AuEqPower[pan]` and `volume * AuEqPower[127 - pan]`. A voice sends dry (`AuEqPower[fxMix]`) to the main bus and wet (`AuEqPower[127 - fxMix]`) to the aux bus. A gain ramps linearly over `delta` samples to the new target (`_getVol`, `lratm`, `lratl`).
- Mono mode (`AuSynUseStereo` false, `au_set_stereo_enabled`): pan is the centre value.
- Buses (`fxBus[4]`): gain (`au_bus_set_volume`, used by the fades), effect type (`au_bus_set_effect`). Effects (`reverb.c`): small room, big room, chorus, flange, echo, custom 0 to 3. Each is up to 4 delay sections with feedback, feed-forward, gain, chorus rate and depth, and a low-pass.
- Channel delay (`AuDelay*`, BGM command): one channel of one bus is delayed by a number of blocks. The game turns it off in mono mode.
- Global volume (`AuUseGlobalVolume`, `AuGlobalVolume`): fade at reset.
- Voice end: the mixer records the stop when the sample runs out (`au2_stop_record`). The control side frees the voice at frame F+2 (`au2_apply_stops`). The envelope end also frees a voice (`donePending`).
- Measured load (Azahar, tour of 7 maps and a battle, 12 windows of 300 steps): voices in use average 3.7 to 8.0, maximum 14. Ticks with 13 to 16 voices: 0.1 to 6 percent. Ticks with 17 or more: 0. Voice starts: 75 to 140 per 10 s. Pitch above 2.0: 0 voices. Looped voices: 1 to 3.5 on average.

## 3. What ndsp offers (C: libctru headers, /opt/devkitpro/libctru/include/3ds/ndsp)

- 24 channels. Format per channel: mono or stereo PCM8, PCM16, DSP-ADPCM. Rate per channel in Hz (the DSP resamples to 32728 Hz). Interpolation: polyphase, linear, none.
- Mix array of 12 floats per channel: 0 to 3 = main (front left, front right, back left, back right), 4 to 7 = aux 0, 8 to 11 = aux 1. `ndspChnSetMix`.
- Wave buffers: a queue per channel. Fields: address, `nsamples`, `looping`, `status`, `sequence_id`, ADPCM start state. A looping buffer repeats the whole buffer. `ndspChnGetWaveBufSeq`, `ndspChnGetSamplePos`, `ndspChnIsPlaying` report progress.
- Frame: 160 samples (4.9 ms). `ndspSetCallback` runs once per frame. Parameters set within one frame collapse to the last value (H: to be confirmed in A2).
- Aux: `ndspAuxSetEnable(id)` and `ndspAuxSetCallback`. The callback gets `nsamples` and four sample pointers. It is not known whether the DSP mixes the changed samples back (E: it does not, the aux is an output for the application). The design does not depend on it: the return goes through a normal channel (H: A2 must show that the callback runs and what the buffers hold).
- Master volume: `ndspSetMasterVol`. Output mode stereo or mono.
- The DSP reads physical memory. Only linear memory converts (`osConvertVirtToPhys`). The resident ROM is in the newlib heap, so samples must be copied. Flush the data cache after a write (`DSP_FlushDataCache`).

## 4. Mapping

| N64 | ndsp |
|---|---|
| voice i (0 to 22) | channel i |
| voice 23 | not usable: reserved by the engine at init (priority never free). Channel 23 is the effect return. |
| pitch ratio r | `ndspChnSetRate(i, r * 32000)` (sample rate in Hz is `sampleRate`, ratio is rate / 32000 already) |
| volume, pan, fxMix | mix floats: dry L = volL * dry, dry R = volR * dry, aux0 L/R = volL/volR * wet (bus 0), aux1 L/R (bus 1) |
| bus 0, bus 1 | aux 0, aux 1 |
| bus 2, bus 3 | not used by the game (M). The design refuses them with a log line, no mix. |
| `au_bus_set_volume` | scale on the wet return gain and on the dry mix of the voices of that bus (control keeps the bus gain and multiplies it in the mix update) |
| global volume | `ndspSetMasterVol` |
| mono mode | pan = centre in the mix, same as the N64 code |
| start voice | `ndspChnReset`-free: clear the queue, set format, rate, mix, coefficients none, add wave buffer(s) |
| stop voice | `ndspChnWaveBufClear` |
| sample end | wave buffer status DONE or `ndspChnIsPlaying` false, then `au2_stop_record` with the block of the timeline |

Voice limit and stealing: the engine already picks and steals inside its slots by priority, so the mapping keeps its rules. With 23 usable slots against a measured maximum of 14, no extra stealing code is needed. A log counter reports any tick with 23 voices.

## 5. Codec decision

| Option | Size (all 72 banks, 9.4 MB VADPCM) | Working set per scene | Work | Quality | Verdict |
|---|---|---|---|---|---|
| A. Decode on first play to PCM16 (cache) | 33.5 MB if all, but only the played samples: 253 to 633 KB per 10 s window (M) | 1.5 MB cache is enough (E) | CPU decode ~30 cycles per sample on a loader step: 128 k samples for the largest sample (72 KB) is ~14 ms, 30 KB is ~6 ms (E) | exact N64 decode, no second quantisation | RECOMMENDED |
| B. Offline DSP-ADPCM library | 9.6 MB (+1.6 percent), needs an SD file or heap | ~0.4 MB | converter with LPC analysis, second quantisation, loop alignment to 14-sample frames, loop history | about 3 dB more noise (E) | only if A fails the memory test |
| C. Offline PCM16 library | 33.5 MB | n/a | no | exact | rejected: the heap has 4 MB left on the O3DS (H, topology.md) |
| D. Re-use the VADPCM books as DSP-ADPCM coefficients | n/a | n/a | the 16-sample and 14-sample frames do not align, scale and predictor change inside a DSP frame | n/a | rejected |

Option A details:
- Cache key: the `wavData` ROM address. Entry: PCM16 data in linear memory, sample count, loop split. Pinned while a voice plays it. LRU eviction of unpinned entries when the budget is full (switch `dspcache=<KB>`, default 1536).
- Decode runs the existing `pm_rsp_audio.c` ADPCM decode (same codebook, same arithmetic). Source bytes come from `pm_os` ROM access (resident ROM: direct pointer).
- When to decode: (1) at a miss when a voice starts, on the dsp thread, the voice starts when the decode is done (delay about 6 ms in the worst case, once per sample after an eviction); (2) as a prefetch when a BGM player picks an instrument (program change) and when the SFX player resolves a sound. Most BGM instruments are chosen before the first note.
- Loops: PCM16 loops can start at any sample. Buffer 1 = samples [0, loopStart), buffer 2 = the loop body decoded with the stored loop state (`lstate`), flagged `looping` for count -1. A finite count is queued as that many copies, then the tail [loopEnd, end). This gives the N64 sample stream.
- Cache memory is a single linear block (no fragmentation) with a first-fit allocator.

## 6. Control to ndsp bridge (timeline thread)

The AU2 command queue (`AuCmd`, ring of jobs, render thread on core 1) already carries every `au_syn_*`, bus, delay, stereo and global volume call in call order. `dspaudio` reuses it:
- The render thread becomes the DSP thread. It does not call `au_pull_voice`. A shadow voice array keeps the state that the N64 mixer keeps (volume, pan, fxMix, ramp `delta`, `segEnd`, pitch, sample, motion). The existing `au_syn_*_x` code already updates `envMixer`. It is reused for the shadow.
- Timeline: each `AUC_BLOCK` is 184 samples of audio time. The thread consumes the queue paced by the DSP frame callback (one DSP frame = 156.4 N64 samples). Commands are replayed up to the time that the played frame reaches plus the lead. Lead = one step (33 ms), the same idea as `PM_AU_LEAD`. This gives the ring the role of the jitter buffer, so the bursts of two ticks per step do not collapse into one parameter write.
- Each DSP frame the thread advances the shadow ramps (`_getVol` math) and writes `ndspChnSetMix` only for voices whose gains changed, and `ndspChnSetRate` for voices whose pitch changed. Voice start and stop become wave buffer add and clear at the frame where the command is due.
- Voice end: the thread polls the wave buffer status of each playing voice once per DSP frame and calls `au2_stop_record` with the timeline block number. `au2_apply_stops` stays as it is. The AU2 stop record rule is kept (voice restarted since the stop cancels the record).
- Banks: `au_load_*` bank loads keep their `pm_au2_sync(AU2_SYNC_BANK)`. The cache stays valid because the key is the ROM address. A bank clear drops no cache entry (the data is by ROM address).
- Hash gates (`audiohash`, `audiolock`) do not apply to `dspaudio`. `audiolock` makes `osAiGetLength` return 0 for the old path. In `dspaudio` the N64 manager still calls `osAiSetNextBuffer`: the post becomes a no-op that counts frames (the manager needs the call to pace itself, C: `nuAuMgr`). The output buffer is not filled.

## 7. Effects

- Bus 0 (BGM) and bus 1 (SFX) map to aux 0 and aux 1. The aux callback runs once per DSP frame on the ndsp thread. It reads the wet samples, runs a CPU delay network and writes the stereo wet to a ring. The DSP thread queues the ring as PCM16 stereo wave buffers (160 samples each) on channel 23. The reverb adds one to two frames of latency (5 to 10 ms), which is inaudible for a reverb.
- CPU reverb: the same sections as `reverb.c` (input tap, output tap, feedback, feed-forward, gain, low-pass, chorus modulation), written for s16 buffers at the ndsp rate, not through the RSP model. Small room is 3 sections, length 11 units. E: 2 channels x 3 sections x 32728 samples/s x ~10 cycles = 2 M cycles/s, below 1 percent of one ARM11 core. Delay lengths scale by 32728 / 32000.
- Bus 1 has custom 0. It is `NULL_PARAMS` (0 sections) unless an SFX sets `customReverbParams` (sfx_player.c:726). With 0 sections the wet passes dry or is cut as in `au_pull_fx` (to be read in A3).
- Chorus, flange, echo, big room, custom 1 to 3 are written with the same section code. They are not used in the tour (M: only types 1 and 6 were set). They get a log line when set, so the user can tell which scene uses them.
- Channel delay: if on, the wet and dry of the delayed bus both go through the CPU path (both sends to the aux, delay and return). It costs CPU only while on. Open question 2.
- If the aux callback does not run on the console (H), the fallback is: no reverb, a log line, the user hears dry only. The old mixer remains available with the switch off.

## 8. Audible differences (not bit exact)

- Resampler: the DSP uses its own polyphase or linear filter. The N64 mixer used a 2-tap fixed-point filter. Expect slightly different high-frequency content.
- Rate: the DSP runs at 32728 Hz. Pitch is corrected by the rate argument. Delay times of effects are scaled.
- Gains: float against 16-bit fixed-point. Envelope steps are per DSP frame (4.9 ms) against per sample. Fast attacks of 1 to 2 frames sound the same. Slower envelopes are smoother than before.
- Pan law, dry and wet amounts: same tables, same formulas. The wet level must match by a calibration (aux scale constant, tested by ear and by the spectrum test).
- Clipping: the N64 mixer saturated at 16 bit per bus. The DSP sums in higher precision and clips at the end. Loud scenes can differ slightly.
- Latency: lead of one step in the timeline plus 1 to 2 DSP frames, about the same as today (PM_AU_LEAD, 44 ms).
- Decode: exact (option A). No new quantisation noise.

## 9. CPU and memory after the change (E)

- Core 1 per tick: control only (0.2 to 0.8 ms per step measured in AU2). DSP thread per DSP frame: shadow ramps and a few `ndspChnSetMix` calls. E: 14 voices x 200 Hz x 2 us = 0.6 percent of a core. Mixer, resampler and ADPCM decode in the tick: gone (was 3.07 ms per tick on the O3DS, M hw_p3). Reverb: below 1 percent. Decode on first play: 6 to 14 ms of core 1 per new sample, about 10 to 40 samples per scene change.
- Memory: linear +1.5 MB cache (budget). The pool margin on the O3DS is 2.6 MB (M, topology.md). MEM1 frees 3.5 MB of GPU command buffer, if the GV1 agent applies it. Code +8 KB. The old mixer code stays.
- 3dsx size: +10 KB (E).

## 10. Risks

| # | Risk | Control |
|---|---|---|
| R1 | Aux callback does not run or does not carry the samples (Azahar or console) | A2 tests it first. Fallback: dry only, log line. |
| R2 | Gain changes within one DSP frame collapse; envelope steps audible | Timeline pacing (section 6). A2 measures how fast a parameter write is applied. |
| R3 | Cache miss delays a note | Prefetch at instrument pick. Counter of late starts. Budget switch. |
| R4 | Linear memory on the O3DS | Budget switch, MEM1 row, log of cache bytes and evictions. |
| R5 | `ndspChnSetMix` and wave buffer calls cost more than estimated on the O3DS | A2 logs the cost per call. The console numbers come from the next push. |
| R6 | ndsp thread and callback run on core 0 and cost game time | A2 logs the core of the callback thread and its time per frame. |
| R7 | Voice end timing differs, so the engine reuses a voice at another time than before | The stop record rule is kept (F+2). The dspaudio path has its own reference; the N64 sound will not be identical. |
| R8 | Mono mode, channel delay, big room and flange untested in the tour | Test list in A3. Log lines name each effect type once. |

## 11. Test plan

- Host test (A3): feed one recorded `AuCmd` stream (switch `au2cmd` dump from the old path) into (a) the old software mixer and (b) a float model of ndsp (rate, linear interpolation, gains, loop buffers) driven by the new shadow code. Compare the two PCM streams with a Python script (3ds/tools/aud_cmp.py): RMS level per 100 ms, spectrum per second (FFT bands), peak. Pass: band levels within 1.5 dB and RMS within 1 dB per window. This checks the translation, not the DSP firmware.
- Azahar (A3): voices log, cache log (hits, misses, evictions, late starts), `audio dsp:` lines (control cost per tick, DSP thread cost per frame, queue depth, any dropped frames), no crash in the tour (kmr, nok, mac, pra, iwa and a battle). Azahar has a video dump option in its Qt menu, not scriptable from `run_azahar.sh`. If the user records an Azahar dump, the Python script compares it with the old path run. Otherwise: the user listens on the console.
- Console (next push): `dsptest:` and `audio dsp:` numbers, underruns, the ear test.

## 12. Stages

- A2: switch `dsptest` (24 channels, looped PCM16 and DSP-ADPCM, 10 s, control call costs, aux callback check, mix write latency).
- A3: switch `dspaudio` (cache, decode, shadow voices, timeline thread, effects, logs, host test).
- A4: converter only if the cache budget fails on the console (option B).

## 13. Open questions (defaults)

1. Codec: decode to PCM16 on first play (default) or an offline DSP-ADPCM library. Default: PCM16 cache, A4 not built.
2. Channel delay (Haas, a few songs): send both dry and wet of the bus through the CPU path (default) or drop the effect.
3. Reverb return: reserve channel 23 and 23 voices (default) against a voice limit of 24 with no reverb.
4. Cache budget 1.5 MB (default) against 1.0 MB.
5. Bus 2 and 3 refused with a log line (default), as the game never uses them.

## 14. A2 and A3 results (Azahar, 2026-10-21)

- A2 (`dsptest`, commit a7a9c7bb0): 24 channels, 600 ticks in 10 s. One tick of 24 SetMix + 24 SetRate + 4 restarts costs 361 us in Azahar (about 2.7 us per call, one spike of 40 ms). The frame callback runs every 4.9 ms and costs 1 us. The aux 0 callback never runs in Azahar (n 0). The console numbers come from the next push (lines `dsptest:` in the pulled log).
- A3 (switch `dspaudio`, default off): tour of 3 maps and a battle, 2500 frames, no crash. Voices average 4 to 8, maximum 14. Cache: 15 decodes at the start, then hits only. Cache peak 702 KB of 1536 KB. Decode: 0.5 to 14 ms average, 36 ms for the largest sample (synchronous on the DSP thread, the backlog of 57 ms covers it). Block cost on the DSP thread: average 12 to 76 us. ndsp parameter write: about 1 us. Backlog: 10 blocks (about 1840 samples), stable. Late resyncs: 3 at the first window, then 0. ndsp frames dropped: 0.
- Not tested in Azahar: the effect return and the channel delay (the aux callback does not run there), the old path after the change (the only change is a flag test), the sound itself.
- Files: src/audio/core/dsp_voice.c (shadow voices to ndsp), 3ds/src/pm_dspaud.c (cache, decode, ndsp calls, effects, stats), hooks in syn_driver.c (AUC_BLOCK, no wait for the render thread) and pm_audio.c (init, post, pending, stats).
- Log lines for the console: `dspaudio:` (on, effect type per bus, channel delay), `audio dsp:` per 300 steps (voices, block cost, backlog, param writes, cache hits and misses and bytes, decodes, aux callbacks with core and cost, frames queued and dropped), `dsptest:` (switch dsptest). Control cost per tick: the existing `audio cost` and `audio2:` lines.

## 15. A/B list for the console (switch `dspaudio` in debug3ds.txt, add `audiostat loglevel=debug` for numbers)

Play the same scene twice, once without and once with `dspaudio`.
1. Music: title, a town, a battle. Listen for the same pitch and tempo, no clicks at note starts, no hum.
2. Held notes and loops: long BGM instruments and ambience (wind, water) must not stop or click at the loop point.
3. Sound effects: jump, hammer, many hits at once in a battle (more than 8 voices). Nothing may cut off early or stay on.
4. Reverb: BGM with the small room (most maps). Expect a short room tail. If the tail is missing, the aux callback did not run: read `aux callbacks` in `audio dsp:`. Dry sound is then the expected fallback.
5. Pauses and fades: open and close the pause menu, change maps (fade out and in), start a battle. Volume must follow without a jump.
6. Mono and stereo: switch the sound mode in the options. Stereo spread must change.
7. Late notes: at the first play of a scene a note can start up to 36 ms late once (decode). Note it if it is audible.
8. Numbers to pull: `audio dsp:` lines (block cost, backlog, cache, aux core and cost), `audio cost` per tick (control only), `dsptest:` lines.

## 16. PT22 distortion: cause and fix (2026-10-21)

- Cause found: the cache decode (`vadpcm`) updated the two history samples after every sample. The N64 decode (aADPCMdecImpl) keeps the history of a block of 8 samples fixed and takes the last two samples of the block before. Every sample after the first in a block was wrong by a small amount. The result sounds like noise on the whole signal. Azahar could not show this because nobody listened to the Azahar run. Build 970 had the fault in the cache for every sample.
- Fix: `3ds/src/pm_vadpcm.h` (decode moved out of pm_dspaud.c). Host test `sh 3ds/tests/dspdec/run.sh`: 4000 random cases, equal to aADPCMdecImpl (plain C path). Before the fix: 3998 of 4000 differ.
- The aux callback did not run on the console or in Azahar. Cause candidate: libctru sets the aux volume to 0.0 at start. The code now sets 1.0. If the callback still does not run, the wet and delay sends go to the main mix (`mix_out` in pm_dspaud.c), so no part of the sound is lost. The log line `dspaudio: first aux callback ran` shows the change to the effect path.
- New switches: `dspfx=off` (no aux, wet to main), `dspgain=<percent>` (scale the mix, for a clip test), `dspflush` (flush the sample before every wave buffer add), `dspaux=<mask>` (1 keep volume 0.0, 2 front bypass off, 4 output count 2, 8 register the callbacks after the first voice start). The log line `dspaudio: aux mask ...` shows the state.
- Console test, three combinations: (1) `dspaudio` alone. (2) `dspaudio dspfx=off`. (3) `dspaudio dspgain=50 dspflush`. Add `audiostat loglevel=debug` and read `audio dsp: aux callbacks`.
- Not changed: hard voice stops (the old mixer stops a voice at the same point), loop cap of 4 repeats for a finite loop count above 3.

## A5 (default and only path)
- `dspaudio` and the old mixer output (ndsp channel 0 with posted PCM) are removed. `pm_audio_post` does nothing: the voices play on ndsp channels. `audiohash` and the posts counters no longer measure audio. `au2off` now means silent audio (DSP needs the queue).
- The wet fold stays (the user hears it and it sounds right). One log line `dspaudio: the folded wet sends of a voice reach gain ...` the first time a voice sum is above 1.0.
- The aux callback of ndsp still did not run on the O3DS (log of build 1036): cause not found, code reading only is open.
- Boot check: the log shows `dspaudio: on, 23 voices on ndsp channels ...` with no switch.
