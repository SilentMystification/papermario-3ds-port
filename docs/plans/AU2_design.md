# AU2 design: audio decoupling

Written 2026-10-20 (lane B). Facts: O3DS build 902. Audio job 13.6 ms average (max 22.3 ms) per step. Core 0 join wait 5.9 to 9.5 ms (max 22 ms). Mixer 6.1 ms per 60 Hz tick (max 16.2 ms). 2 ticks per step.

## 1. Thread model today (AU1d)

| Thread | Core | Work |
|---|---|---|
| Game | 0 | Game logic, gbi, gpu. Calls `pm_audio_kick(n)` at the main `nuGfxTaskStart`. Joins at `pm_gfx_retrace`, `__wrap_step_game_loop` and the main loop (before `pm_warp_tick`). |
| Audio (`au_thread`, nuAuMgr) | 1 | Per tick: `alAudioFrame` = control (sequencers, voices, `au_syn_*`) and render (mixer), then `osAiSetNextBuffer` of the previous frame. |

The game thread and the audio thread never run together. This is why the engine needs no locks.

## 2. State that the engine uses

Control state (written by game code and by the sequencers, read by the sequencers). It stays on one thread at a time (the audio thread inside the join window, the game thread outside it):
- `AuGlobals` (`gSoundGlobals`): `voices[24]` (AuVoice: envelope, volume, pan, `syncFlags`, `donePending`), `effectChanges`, channel delay fields, `musicEventQueue`, bank sets, song list.
- BGM players A, B, C and snapshots, `SoundManager` (`gSoundManager`, SFX queue written by `au_sfx_enqueue_event` from game code), `AmbienceManager`.
- Callbacks `audioThreadCallbacks[0]` (`BeginSoundUpdateCallback`) and the music event queue. They run inside `au_update_clients_for_video_frame`.
- BGM and SEF files (RAM), instrument banks (RAM headers), the ALHeap.

Render state (written and read only by the mixer):
- `gSynDriverPtr->pvoices[24]` (AuPVoice: decoder with ADPCM state and loop state, resampler, envelope mixer `cvolL/R`, `delta`, `segEnd`, `dirty`).
- `fxBus[4]` (gain, effect type, `AuFx` delay lines in the ALHeap), `wetAccumBuffer`, `dryAccumBuffer`, `curSamples`.
- Channel delay buffers `AuDelayBufferMain/Aux`, `AuDelayCounter`, `AuDelayedBusID`, `AuDelayedChannel`.
- `AuUseGlobalVolume`, `AuGlobalVolume`, `AuSynUseStereo`, `AuSynStereoDirty`.
- DMA cache `nuAuDmaState`, `nuAuDmaNext`, `nuAuFrameCounter` (used by `nuAuDmaCallBack` inside `au_pull_voice`, reset by `nuAuCleanDMABuffers`).
- DSP model in `pm_rsp_audio.c` (static DMEM, ADPCM books).
- PCM buffers `D_800A3628[3]`, written by the mixer, read by `pm_audio_post`.
- ndsp slots and counters in `pm_audio.c` (`wb`, `data`, `au_hash`, `au_dropped`, `au_underrun`).

Shared read-only data for the mixer (the control side changes it only in loads):
- `Instrument` structs and ADPCM books/loop states in the bank heap (`predictor`, `loopState`, `wavData` as a ROM address).
- The ROM sample bytes. The mixer reads them through `nuAuDmaCallBack`, `osEPiStartDma` and `nuPiReadRom`. With the resident ROM this is a `memcpy` of read-only memory. With the file ROM (`romfull`) it is `fseek` and `fread` on one `FILE*`: that needs a lock (LightLock around the file branch).

Does control read back render state? Checked by grep: `au_syn_get_*` have no caller. `pvoices`, `fxBus` and `envMixer` appear only in `syn_driver.c`, `pull_voice.c`, `reverb.c`. The mixer sets no flag that the control side reads (voice end is decided by the envelope in `au_update_voices`). Result: the control side reads nothing that the mixer writes. The one exception is `pm_audio_pending` (osAiGetLength), a heuristic that reads ndsp state. A late value only changes the frame size (552 or 368 samples), never the data. `audiolock` makes it 0.

Control calls that write render state (the only bridge). These are the `au_syn_*` functions that read and change `pvoice` and `envMixer`, plus bus, delay, stereo and volume setters:

| Call | Used by |
|---|---|
| `au_syn_stop_voice`, `au_syn_start_voice_params`, `au_syn_set_wavetable`, `au_syn_set_pitch`, `au_syn_set_mixer_params`, `au_syn_set_pan_fxmix`, `au_syn_set_volume_delta`, `au_syn_set_pan`, `au_syn_set_fxmix`, `au_syn_start_voice` | `au_syn_begin_audio_frame`, `au_flush_finished_voices`, `au_engine_init` |
| `au_pvoice_set_bus`, `au_bus_set_volume`, `au_bus_set_effect`, `au_bus_set_fx_params` | init, `au_syn_begin_audio_frame`, `au_fade_set_volume` |
| `au_set_delay_time`, `au_delay_left_channel`, `au_delay_right_channel`, `au_disable_channel_delay`, `au_init_delay_channel` | `au_syn_begin_audio_frame`, init |
| `au_set_stereo_enabled` (game code, `snd_interface.c`), `au_use_global_volume`, `au_set_global_volume` (`nuAuPreNMIProc`) | game, NMI |
| `nuAuCleanDMABuffers` | `nuAuMgr` after each audio task |
| `osAiSetNextBuffer` (post PCM), `osAiSetFrequency` | `nuAuMgr`, init |

Game code does not call any `au_*` function directly (grep of `src` outside `src/audio`: no call). Game code uses `snd_*` and `bgm_*`. They only queue events, except the loads below.

Loads that change data the mixer reads (banks): `au_load_BK_to_bank`, `au_load_static_BK_to_bank`, `au_load_aux_bank` (also from `snd_song_load` on the game thread), `au_clear_instrument_group`, `au_load_INIT/PER/PRG`, `au_engine_init`. They need a render drain first (section 4).

## 3. Cut line

Option A (control on core 0 at step time, render on core 1): control costs core 0 time every step. The control cost is not known yet. A stays as the fallback (`audiocore0`).

Option B (chosen): the cut is inside `alAudioFrame`, at the line between "update the clients and write `au_syn_*`" (control) and "pull the voices, effects, mix and save" (render).
- Control stays in the audio thread (nuAuMgr coroutine) with the AU1d kick and join. It runs in the window of gbi and gpu on core 1 and the game joins it at the same sites. The join now waits for the control cost only, not for the mixer. The control cost is small and fits the window.
- Render is a new thread (core 1, lower priority than control). It consumes a command queue and runs until the queue is empty. The game thread never waits for it.
- Reason for B over A: the same join sites keep the control side exclusive with game code (no new race for the sequencers, callbacks, SFX queue). A would put the control cost on core 0.

Why commands and not a full snapshot of the voices: `au_syn_set_*` read and change render state (`envMixer->delta`, `cvolL`, `segEnd`, `lratm`). A copy of the parameters is not enough: the mixer has to run the same functions on its own state, in the same order. So control emits one command per call (op, voice, arguments: 20 bytes), and the render thread runs the original function on the command. The replay order is the call order. The mixer state sees the same inputs in the same order as before, so the PCM is equal bit for bit. Cost of the copy: 20 bytes per call, about 20 to 100 calls per tick (measured by the log line `audio2`).

Block structure: `alAudioFrame` makes 3 blocks of 184 samples for 552 samples. Per block: control updates (commands), then one command `AUC_BLOCK` (render of the block).

Command list: `AuCmd { u8 op, idx, b1, b2; s16 s1, s2; s32 v0, v1, v2; }` (20 bytes). Ring: 8 job slots of 256 commands (40 KB). A job = commands between two commits. Commit points: end of `alAudioFrame`, slot full, idle (audio thread signals "done"), render drain. Free slots are counted by a LightSemaphore (control blocks when 8 jobs wait: reason `full`). Filled jobs are counted by a second LightSemaphore (render sleeps on it).

Order of the commands equals the order of the old calls:
1. `alAudioFrame` start: `AUC_STEREO_SET` if the flag changed (game code wrote it before the tick), then the video-frame update (commands), then `AUC_STEREO_REPAN` if dirty.
2. Per block: control updates, then `AUC_BLOCK(outBuf)`.
3. In `nuAuMgr` at the next retrace: `AUC_CLEAN_DMA` (was `nuAuCleanDMABuffers`), `AUC_POST(buf, bytes)` (was `osAiSetNextBuffer`), then the next `alAudioFrame`.
The render thread runs the commands in this order. Post k therefore always follows the render of frame k, and the PCM buffers `D_800A3628[3]` are never written while a post reads them (single render thread, same order as before).

Callbacks and fixed-step work stay on control: `BeginSoundUpdateCallback`, `snd_song_clear_music_events`, sequencers, SFX queue, envelope step (`au_update_voices`). Nothing of them moves.

## 4. Join and drain

Control join (unchanged): `pm_audio_join_at` sites 0 to 3 (`pm_audio_kick`, `pm_gfx_retrace`, `__wrap_step_game_loop`, main loop before `pm_warp_tick`). It waits for the control job only.

Render drain `pm_audio_render_sync(reason)`: commit the open job, wait until the render thread has run every committed job. Callers:
- Bank loads and clears (list in section 2). Reason `bank`. Rare.
- Map change (`pm_map.c` load wrap) and flash save. Reason `map`, `save`. The mixer reads no game memory, but one drain keeps the ROM and SD card access single at those points.
- `pm_audio_shutdown`, suspend (APT sleep) and exit. Reason `shutdown`. HW7 order stays: `pm_flash_flush`, `pm_audio_join_at(0)`, drain, `pm_audio_shutdown` (stops the render thread, then `ndspChnReset`, `ndspExit`). The render thread ends before the DSP is released.
- Ring full (8 jobs wait). Reason `full`.
Counts of each reason are logged.

## 5. Latency and underrun

Render starts when the control thread commits a job (end of a tick). A frame is posted when the render thread reaches its `AUC_POST` (emitted one tick later by control). In the normal case the time of the post equals the old time plus the wait for the render of one tick (6 ms on O3DS), well below one step. If the render thread is late, the ring fills and queued PCM stays small: this shows as `underrun` and `ring fill`. The accepted worst case is one extra step (33 ms) of lookahead (8 jobs = 4 steps of buffering is the maximum, not the use).

## 6. Hash gate (rule)

The posted PCM stream is the same data in the same order. The time of a post changes. The hash `audiohash` is FNV-1a over all posted bytes in post order and is logged every 120 posts, so compare the line `audiohash posts N fnv X` at equal N. Rule: with `audiohash audiolock` the `fnv` values at every N must equal the AU1b/AU1f reference build (`/tmp/claude-1000/pm_au2_ref.3dsx`, HEAD 42f40d643). The `dropped` and `underrun` fields can differ and are not compared. The final line is written after the drain, so it also equals when the post count equals. No frame skip and no one-step delay is needed. Also equal: counters and `statehash` (control keeps the game state).

Host test: `3ds/tests/au2/test_queue.c` runs the ring (producer thread, consumer thread, random sizes, forced full ring) and checks no loss and no reorder.

## 7. Slices

1. AU2a: queue, render thread, commands, switch `au2off`. Gate: hash equal, counters equal, host test.
2. AU2b: metrics, drain callers, shutdown and suspend. Azahar numbers at cpulimit 80 and 30.
3. AU2c: default on after the gate passes.

## 8. Result (Azahar, lane B, 2026-10-20)

Build: `/tmp/claude-1000/pm_au2_d.3dsx` (AU2 on by default, `au2off` switch for the old path). Full demo cycle, `fixedseed statehash=10`, frame limit 5000.
- Hash gate: `audiohash audiolock` against the reference build (HEAD 42f40d643): all `fnv` values at equal post counts are equal (143 of 143 checkpoints in the last run, 144 of 144 in the first run). `statehash` and the `pc` counter line are equal. Host test `sh 3ds/tests/au2/run.sh` (ThreadSanitizer): 1,055,293 commands, no loss, no reorder, drain exact.
- Cost of the control part: audio job 0.23 to 0.77 ms per step (was 5.5 ms in Azahar with `au2off`). Commands per job: average 2, maximum 9. Copy cost: 4 us per job. Ring fill: 1 to 2 jobs (the ring was never full: 0 full waits).
- Core 0 join wait per step, average of the 300-step lines: `au2off` 651 us (max 23.9 ms) at cpulimit 80, 5231 us (max 45.4 ms) at cpulimit 30. AU2 on: 13 to 20 us (max 2 to 8.8 ms) at cpulimit 80, 44 us (max 17.4 ms) at cpulimit 30. The remaining maxima come from map loads and bank loads (drain).
- Render thread busy time per job: average 1.4 to 2.1 ms, maximum 6.0 ms at cpulimit 80. At cpulimit 30: average 3.7 ms, maximum 15.6 ms.
- Underruns in Azahar are not 0 in the old path either: `au2off` 157 of 17055 posts at 80, 143 of 15967 at 30. The first AU2 build gave 202 of 17048 at 80 and 387 of 17047 at 30: the first post of a step waited for a wake of the render thread (the queue holds only about one frame at the start of a step). Fix: when the render thread is idle and nothing is open, `nuAuCleanDMABuffers` and the post run at once on the control thread (`pm_au2_idle`), so the first post has no thread hop. After the fix: 98 of 16797 (audiolock run, real queue level measured by `pending_raw`). The non-lock numbers for the fixed build need 2 more Azahar starts (test budget of row AU2 is used).
- Not tested: HOME menu and sleep hooks, exit through HOME with the render thread, `romfull` (file ROM lock), O3DS.

## 9. Feed and latency (PT14)
- osAiGetLength reports the ndsp backlog minus PM_AU_LEAD (1416 samples, 3ds/src/pm_audio.c), or 0 below that.
- Reason: the game (nuAuMgr in src/audio/core/system.c) mixes a full frame of 552 samples when the value is below 184, else a short frame of 368. On the N64 the value is the rest of one DMA buffer. Here it was the whole backlog, and at 30 FPS two ticks come in one burst per step. So every second tick was a short frame: supply 27.6 kHz against 32 kHz, and the DSP ran dry in every step (O3DS build 926: 9.6 percent of posts underran).
- Now the game mixes full frames until the backlog holds about 1600 samples (50 ms) and then keeps it there with its own regulator. Azahar: short frames 281 to 59 of 568 posts, supply 27646 to 31957 Hz.
- Cost: the audio is 50 to 80 ms later than before. The switch audiolock still returns 0, so the PCM hash gate does not change.
- The switch audiostat logs `audio feed:` (posts, short frames, post interval, backlog at post, supply Hz).
