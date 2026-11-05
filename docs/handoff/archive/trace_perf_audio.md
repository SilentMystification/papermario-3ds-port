# trace_perf_audio: audio voices, splash grab cost, pause loads, battle transition garble

Branch native. Read-only trace. No Azahar start was run. No status_TRPA.txt line was written, because no start was made.
PROVEN = read in code or log. INFERRED = not run.

## 1. Missing sound effects (console)

Voice mapping
- 3ds/include/pm_dspaud.h:7 PM_DSPA_VOICES 23. Voices use ndsp channels 0 to 22.
- 3ds/src/pm_dspaud.c:382 chan_setup resets 24 channels (0 to 23). Channel 23 is the effect return (pm_dspaud.c:386-391, stereo, 32728 Hz).
- 3ds/src/pm_dspaud.c:320 fx_emit adds wave buffers to channel 23. Eight effect buffers (FXR_BUFS, pm_dspaud.c:17). A full queue drops the frame (pm_dspaud.c:310, counter fx_dropped).
- src/audio/core/dsp_voice.c:90 loops i < PM_DSPA_VOICES. Voice index 23 and up never play. PROVEN.
- src/audio/core/syn_driver.c:166 allocates config->num_pvoice physical voices. include/nu/nualsgi.h:55 caps the count at 32. The configured num_pvoice value is NOT checked. INFERRED: the game uses 24 or more voices and the port plays 23.
- Music channel use is not traced. Open item.

Silent start failure (PROVEN by code)
- 3ds/src/pm_dspaud.c:82-102 cache_get returns NULL on: no ROM data, no heap, sample larger than cache, or no evictable entry (all pinned or used in the last 60 ms).
- 3ds/src/pm_dspaud.c:175-183 pm_dspa_voice_start returns with c->active = 0. Nothing plays. No retry.
- src/audio/core/dsp_voice.c:161-163 sets v->on = 1 and v->gen after the call, whatever the result. The voice is not retried until the sample generation changes. A dropped start is silent for the rest of that voice's life.
- Log: the first 40 drops print one warning line (pm_dspaud.c:72). Counter c_fail goes into the audio dsp stats line (pm_dspaud.c:490, "failures N"), reset each stats period.
- Cache budget 1536 KB default (pm_dspaud.c:30, 413). Switch dspcache=<KB>, minimum 256.

Bank load timing at file select: not traced.

Fix proposal
- Step 1 (cheapest): dsp_voice.c:163 sets v->on only when pm_dspa_voice_start got a sample. Return a result from pm_dspa_voice_start and retry on the next block. This removes the permanent silence.
- Step 2: pm_dspaud.c:72 counts drops per reason. Print drops per reason per stats period.
- Step 3 (24 voices): channel 23 holds the effect return. Options: (a) mix the wet part on the CPU before the aux bus (aux_frame already sums wet and dry in acc, pm_dspaud.c:335-371) and free channel 23 for a 24th voice. Needs the libctru aux return rule, which was not confirmed (no libctru headers were found in this session). (b) Keep 23 voices and free one channel from music. Needs the music channel count (not traced). Recommended first: step 1 and the counters, then decide.

Proof
- Console (orchestrator runs): PM_ROW=PUSH sh .../push_3ds.sh --no-build --ip 192.168.1.170 audiostat ... loglevel=debug. Read the "audio dsp" lines: failures, dropped, voices max. Non-zero failures prove the silence path. Zero failures means the cause is elsewhere (music, or voice index 23).
- Azahar gives direction only. Not used for this item.

## 2. Water splash lag (nok_11)

Code
- src/effects/water_splash.c:206 gDPLoadMultiTile reads nuGfxCfb_ptr for each particle.
- 3ds/src/pm_gbi.c:275 and pm_gbi.c:281 call pm_gpu_grab (one per load). pm_gpu.c:2012 is a second caller.
- 3ds/src/pm_gpu.c:1840-1854 grab_impl: pm_gpu_begin_wait, flush(), off_begin, copy.
- 3ds/src/pm_gpu.c:1848-1851 reuse path. A grab is reused only when no draw dirtied its rows. Each particle draws, so every grab copies.
- 3ds/src/pm_gpu.c:1807-1814 pm_gx_room runs the GX queue and waits for the GPU once numEntries reaches 24. This wait is the stall. Called from grab_queue_copy (pm_gpu.c:1819).
- 3ds/src/pm_gpu.c:1820-1821 GX_TextureCopy size: non-wide, s0 = 5, s1 = 44, so 40 strips of 7680 B = 307200 B per copy. Wide: 50 strips = 384000 B. The figure 314880 B (41 strips) in the brief does not match this code. PROVEN by arithmetic.

Cost per frame (40 grabs, non-wide)
- 40 GPU copies = 12.3 MB of texture copy per frame.
- 40 flush() calls and 40 split points (one per grab).
- Up to 40 GX queue waits when the queue passes 24 entries.

Fix (option 2, one grab per frame)
- water_splash.c: take one grab before the particle loop (one gDPLoadMultiTile), bind that one texture for all particles, and map the tile offsets (lines 200-212) to the copy slot.
- pm_gbi.c:275-281: no change needed if the first grab is made before the loop.
- pm_gpu.c:1848-1851: the reuse check must skip the splash draws. The splash reads the pre-splash frame, so the copy stays valid. Add a frame-scoped flag (grab frame == pm_frame_count) for this path.
- Result: 1 copy (307200 B), 1 flush, 1 split per frame. INFERRED that the picture is unchanged. Needs a byte-equal check on the splash scene.

## 3. Pause screen: slow once, some things not loaded

What the pause menu loads
- VRAM users at runtime: copy block 512 KB (pm_gpu.c:370-379 copy_get), full off-screen images 512 KB each (OFF_N = 4), texture slab extension. The last two come from the grab3 handoff (docs/handoff/agent_grab3.md), not re-read here.
- Failure path 1: copy_get returns NULL when bank_block(BLK_FULL) fails. It logs "vram alloc fail copy" (pm_gpu.c:373). grab_tex_init returns 0 (pm_gpu.c:1793-1794). grab_impl returns 0 (pm_gpu.c:1856), so the caller reads the frame with the CPU. CPU readback costs about 8.8 ms (grab3 handoff, frame 174). PROVEN by code, cost from the handoff.
- Failure path 2: off-screen image alloc fails. pm_gpu.c:1720 and 1730 set off_fail = 1. grab_impl returns 0 (pm_gpu.c:1847) and the pass draws on the screen. pm_gpu.c:1949 clears off_fail at the start of the next frame, so the failure lasts one frame.
- Texture slots: pm_mem.c:240 logs "tex fail cum" and evictions per map. The 3072 KB texcache (two 1536 KB halves) is the main texture budget.

Likely stall
- A pause open needs the CPU readback (8.8 ms) and a texture set that may not fit. With the copy block taken, the texture cache loses its slot and entries evict. INFERRED.

Test (catalog, no new scenario)
- pause_spin (docs/handoff/test_catalog.md:89, BTLSTAR, unverified): add the switch grablog and read the grab lines and "vram alloc fail" lines. Read the "texmap state" line (pm_mem.c:240) for tex fail cum and evict cum. Azahar numbers show direction only.
- Console: the orchestrator pushes with loglevel=debug and reads "vram alloc fail", "FX2a", and "V05" lines.

## 4. Battle start transition: garbled at random

Shared resources (PROVEN)
- copy_blk is one 512 KB block (pm_gpu.c:370-379). Two users in one frame: the frame grab (grab_tex_init, pm_gpu.c:1793) and the FX snap path (pm_gpu.c:436, "this copy overwrites the frame start grab").
- Each user queues GX_TextureCopy into the same block. The GX queue runs in order at frame end. Whether the CPU read waits for the second copy is not verified. INFERRED race if the snap copy lands in the same block during the grab read.

Random failure candidates (PROVEN that each path exists)
- VRAM fail on copy_get: grab falls back to CPU readback (pm_gpu.c:1793-1856). Result depends on the VRAM state at that moment.
- VRAM fail on off-screen image: off_fail = 1 (pm_gpu.c:1720, 1730). The transition pass draws on the screen for one frame (reset at pm_gpu.c:1949). This gives a one-frame garble at the same place when the VRAM state repeats. Best fit for "same place, random". INFERRED as the cause.

Test (deterministic, decides the cause)
- Force the failure with the switch vramfill=<KB> (dummy VRAM blocks in pm_tex.c, from the grab3 handoff) on battle_start_fb. Read the "vram alloc fail offslot" and "V05" lines and the transition shots 0000 to 0014.
- If the garble appears with vramfill and the log shows V05 or copy fail on the same frames, the cause is VRAM failure. Fix options: texcache halves 1536 KB to 1024 KB per bank (grab3 option a), or OFF_N 4 to 2 (option b).
- A race test needs repeated Azahar runs of the same transition. Azahar does not show the console randomness. Not run.

## Shared cause with item 3
- Items 3 and 4 both use the 512 KB copy block and the full off-screen images in the same two VRAM banks. A shortfall in one hits both. INFERRED. The log lines are the same ("vram alloc fail copy", "V05").

## Open items
- Configured num_pvoice (syn_driver.c:166 caller).
- Music channel use.
- libctru aux return rule (needed to remove channel 23).
- Whether off_fail is cleared on map load or only at pm_gpu.c:1949.
- No Azahar run. No status_TRPA.txt line.
