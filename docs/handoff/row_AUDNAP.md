# Row AUDNAP: sounds of every type missing or wrong on the O3DS; repro = the Toad House nap jingle

## User facts
User (O3DS XL): sounds of all types are sometimes missing or wrong (blips, Mario's voice, battle, field). Repro: "when you take a nap
at a Toad House, the jingle that plays as Mario gets into bed at the end has a missing or messed up audio channel."
AUDDROP census (merged 2258ad6c5, switch audiostat): all port reasons 0 on Azahar; the console census was never pulled.

## Start state
`sh 3ds/tools/wt_new.sh AUDNAP`, branch AUDNAP. PM_ROW=AUDNAP, PM_LANE=AUDNAP. One Haiku agent.

## Facts (shared oracle, code only)
- Jingle: EVS_ToadHouse_GetInBed -> EVS_PlayRestingSong -> SetMusic(0, SONG_TAKING_REST 0x88) (mac_01/npc.c:1780, music.c:44-48);
  11_134.bgm, 3 tracks, global instruments only, no aux bank, no drums. Ruled out: sample cache key conflicts, loop cap, non-ADPCM
  samples, aux bank pointers.
- Likely chain (about 55%): (1) first-use VADPCM decode on the audio render thread (dsp_voice.c:206-219 -> pm_dspaud.c sample_build
  :88-140, up to 250 KB per sample; core-1 pre-decode only for battle songs, bgm_control.c:428); (2) the late thread then runs up to 6
  blocks back to back (pm_dspaud.c:540-552 pm_dspa_pace); (3) a note restart clears and re-adds a wave buffer on the same channel in
  the same DSP frame (pm_dspaud.c:167-176, 233), so the new buffer can be dropped (UNHEARD) or end early (EARLYEND).
- Second path (about 30%): cache_room (pm_dspaud.c:78-91) cannot evict entries used in the last 60 ms; starts fail and give up after
  16 blocks (dsp_voice.c:220-227).

## Steps
1. REPRO: scenario toad_nap_aud + bench toad_nap_aud_log (same commit): `warp=mac_01:1 story=ch0_toad_town pos=490:20:-165 coins=50
   audiostat loglevel=debug padscript=wait80,A,wait90,A,wait90,A,wait90,A,wait900` (route of toad_house_menu plus "Yes"; check the
   switch names in pm_warp.c); until `audio drops w\d+: ` after the jingle start; cap 45 s (full jingle plus one census window).
   Run on the base (at most 2 starts). A port reason > 0 inside the jingle frames, or catchup > 0, is the shown fault.
2. If the fault shows: fix in order, one commit and one census run each: (a) `pm_prefetch_song(songID)` in _bgm_set_song
   (bgm_control.c:188, TARGET_3DS) on a song change; (b) pm_dspa_pace without bursts (next_due = now when late, count st_late; delete
   the 6-block catch-up); (c) no channel reuse within one DSP frame (ch_rel_frame per channel, rotating cursor, retrigger releases
   the old channel first).
3. If the fault does not show in 2 starts: stop Azahar runs, commit the scenario, hand back NEEDS CONSOLE with the push recipe
   (prof push with -GameDebug "audiostat", nap in mac_01, pull the log).

## Gates (light)
- nd_host PASS; memcheck OK.
- toad_nap_aud census w2: port reasons 0 in the jingle frames; aud_mac01_lag and aud_talk_lag: catchup 0.
- Console (user): nap in mac_01 with audiostat; all 3 jingle parts heard; port reasons 0 in the pulled log.
