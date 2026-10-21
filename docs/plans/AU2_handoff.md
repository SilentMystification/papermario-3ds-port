# AU2 handoff (lane B, branch lane-b, PM_LANE=laneB). Design: docs/plans/AU2_design.md

## Design in 5 lines
- Control part (nuAuMgr, sequencers, callbacks, au_syn_* calls) stays in the audio thread with the AU1d kick and join.
- Render part (mixer) runs on a new thread (core 1) from a queue of 20-byte commands (AuCmd, 8 jobs of 256).
- Each au_syn_*, bus, delay, stereo, global volume call, the DMA clean and the PCM post is a command. The render thread replays the original function in call order.
- Bank loads, map change, save, suspend, exit drain the queue (pm_au2_sync, pm_audio_drain). An idle render thread lets CLEAN_DMA and POST run inline (pm_au2_idle).
- Default ON now (see AU2 FINAL below). Switch `au2off` turns it off.

## Built (commits on lane-b, last e3f094673)
- 3ds/src/pm_au2.c, 3ds/include/pm_au2.h (queue, render thread, stats). 3ds/tests/au2/ (host test: sh 3ds/tests/au2/run.sh).
- src/audio/core/syn_driver.c (wrappers au2_put, au2_exec, au_render_block_x, trace), system.c, engine.c (sync at bank loads).
- 3ds/src/pm_audio.c, pm_os.c, pm_log.c, pm_map.c, pm_flash.c, pm_main.c (recs[3]).
- Switches: au2, au2off, au2trace (hash per block c/r/p, dump ticks 1400-1600, au2cmd lines for ticks 1534-1536), audiostat (300-step log lines), audiolock, audiohash.

## Gate and reference logs (3ds/logs/perf/au2/)
- Full cycle: ref1.txt (old HEAD 42f40d643), a1.txt (AU2 on): 144 of 144 fnv equal, statehash equal. Passed with the early build. Re-check with `au2` after any fix.
- Battle tour command (PM_ROW=AU2, PM_FL=5000 sh 3ds/run_azahar.sh -F -x <3dsx> -t 150 -u "audiohash posts [0-9]* fnv [0-9a-f]* dropped [0-9]* underrun [0-9]* \(final\)" -o <log> -d "<S>"):
  S="fast warp=kmr_20:4 story=-107 tour=kmr_20:4,kmr_03:0,kmr_02:0 battle battlef=700 audiohash audiolock fixedseed statehash=10 loglevel=debug audiostat smoke smokeframes=1500" (add au2 and au2trace as needed). Create the log dir first.
- e_tour_off.txt, e_tour_off2.txt: au2off, equal (post 1560 fnv bc0edea7, final 97de973b). e_tour_on.txt: AU2, post 1560 f2bbe40a, final f4c8f49e.
- Trace logs: g_off_tour.txt (old build, no op print), g_on_tour.txt, h_on_tour.txt, i_on_tour.txt (au2cmd ops). 3dsx: /tmp/claude-1000/pm_au2_*.3dsx (may be gone; rebuild: PM_LANE=laneB sh 3ds/build.sh).

## Proved
- Queue and replay are faithful: c hash (control) equals r hash (render) in every block; host test with ThreadSanitizer ok.
- The control side decides differently at tick 1535 block 2 (block 4607): AUC_VOL_DELTA(voice 7, vol 0, delta 184) from au_reset_voice / au_reset_nonfree_voice (engine.c:514, 522), called by the SFX player (sfx_player.c:1289, 1312, also 1238, 1352, 1663). These use only control state (voice->priority, manager->priority). au2off has no command there. Game log and statehash are equal in both modes.

## Excluded
Control reads none of the fields the mixer writes: pvoices, fxBus, AuDelay*, AuDelayCounter, AuGlobalVolume, AuUseGlobalVolume, AuSynUseStereo, nuAuDma*, nuAuFrameCounter, curSamples, Instrument->wavDataLength (only copied into the unused sfxInstrument). au_syn_get_* has no caller. A snapshot is not needed. Fixed on the way: stereo flag as a command at the call point, custom reverb drain (AU_FX_CUSTOM_PARAMS pointer), commit before au_busy = 0.

## Next trace (both modes, same switches)
Per tick print: soundID of each request consumed in au_sfx_begin_video_frame (sfx_player.c:618-710, loop at "Process each pending sound effect"), the 24 voices priority/clientPriority/donePending at the start of au_update_clients_for_audio_frame (engine.c:332), and each call of au_load_BK_to_bank / au_load_aux_bank / au_load_song_files with its tick. Compare the first tick where au2off and AU2 differ. Suspects: event arrival time against ticks, or bank/envelope data read while the render thread reads the ROM.

## Budget
Row AU2: all granted starts are used (last: i_on_tour). Ask the orchestrator for a reset. Default stays OFF until the battle tour fnv equals au2off at all checkpoints and the full-cycle hash passes.

## AU2 FINAL f4c8f49e (battle tour final hash, AU2 on and au2off equal at all 32 checkpoints; full demo cycle final fnv bef03f47 at posts 17520, 148 of 148 equal; statehash equal)
- Cause: au_pull_voice (pull_voice.c) called au_release_voice when a sample ran out. That call wrote AuVoice cmdPtr and priority from the mixer. The earlier note "control reads no mixer field" was wrong. With AU2 the render thread wrote it later, so the SFX player freed a player at a different block (first seen: sound 141 on voice 23 at block 1700, next sound took voice 22).
- Fix: the mixer only records the stop (au2_stop_record: block number, voice start count). The control side applies the records of frames F-2 and older at the start of frame F (au2_apply_stops in alAudioFrame), in both modes. A restart of the voice since the stop cancels the record. The control side waits for the render thread to finish frame F-2 (pm_au2_wait_jobs).
- au2off now frees a finished voice up to 2 frames later than HEAD 42f40d643. Its hash differs from ref1 on the tour (bc0edea7 and 97de973b before the fix). AU2 on equals au2off.
- Full cycle: underrun 19 in both modes, late steps 65 of 8809 (on) and 66 of 8783 (off). Join wait at the main site: 0 us (on), 3062 us average (off).
- Default is ON now. Switch au2off turns it off.
