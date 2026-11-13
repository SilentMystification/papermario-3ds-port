# Row AMBWARP: a warp stops the ambient sound (MSEQ) of the map it leaves, not only the songs and the loop sounds

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names.)
## User fact (2026-11-12)
Sometimes warps do not free the audio channels: the ambient sounds of the old map stay (for example the Gusty Gulch wind).
## Cause (ORACLE8 code read, main 4404177f0)
- Ambient sounds are MSEQ players, not loop sfx. A map starts one with PlayAmbientSounds (src/evt/audio_api.c:194, Gusty Gulch:
  src/world/area_arn/arn_02/music.c:12 AMBIENT_WIND) and stops it with ClearAmbientSounds (:183 -> play_ambient_sounds(-1, t)).
  update_ambient_sounds (src/audio/ambience.c:36-74, called every step at src/main_loop.c:112) does the stop and start.
- In the game, a map that needs silence calls ClearAmbientSounds itself (arn_08/music.c:12, dgb_00, dgb_01, jan_01 ...). A map
  that does not (mac_01 Toad Town) keeps the ambient of the map before. A normal exit never jumps from Gusty Gulch to such a map.
- Every port warp ends the old scene in warp_end_intro (3ds/src/pm_warp.c:1023-1039, called at :1143, :1205, :1226, :1947). Its
  log says "song and ambient sounds stopped", but it stops only the two songs (bgm_set_song) and the loop sounds
  (sfx_stop_env_sounds, src/audio/sfx_control.c:361). It never calls play_ambient_sounds(-1, ...): the MSEQ wind keeps playing.
Start: `sh 3ds/tools/wt_new.sh AMBWARP`. PM_ROW=AMBWARP PM_LANE=AMBWARP on every call.
## Correction 2 (ORACLE8, after 74b1ce319, run of commit 43e3f1358)
- The scene used the tour. The tour changes the map in tour_step (pm_warp.c:1445-1492: its own curtain, song B and
  set_game_mode(GAME_MODE_CHANGE_MAP)), not through warp_end_intro: the edit never ran (no "warp: songs" line in the log).
- The user's warp is the Select menu: pm_options.c:856 and :862 -> pm_warp_live -> pm_warp_live_now -> warp_end_intro
  (pm_warp.c:1143). fswarp (:1947), the boss warp (:1205) and pm_warp_now for the title and intro (:1226) also call it. The test
  switch livewarp=<scene frame>:<map>:<entry> (livewarp_hook, pm_warp.c:1881-1890) runs exactly the menu call. The tour is a test
  tool only: it stays as it is.
- Observable: the same rule as AMBTITLE (row_AMBTITLE.md correction 2): silent = snd_ambient_is_stopped(0) is AU_RESULT_OK or
  AU_AMBIENCE_ERROR_3, read at the first world frame of the new map (pm_warp_world_mode), many audio frames after the stop.
  `stopped 0` at arn_02 is right (the wind plays there).
- Code 74b1ce319 stays. Only the probe expression (if not yet the rc rule) and the scenario change.
## Correction 3 (ORACLE8, after 4267f2fe6, runs 5b8fcae99)
- That run did not take the menu path: its log has no `livewarp:` line and no `warp: songs` line, so the map changed by the
  tour (tour_step, no stop). The scenario debug must be exactly `warp=arn_02:0 story=ch0_toad_town livewarp=300:mac_01:0 ambchk
  loglevel=debug det`, with no `tour=` and no `tourtime=`. The two new expected events prove the path ran.
- The probe place is right: mac_01 starts no ambient of its own (grep PlayAmbientSounds and ClearAmbientSounds in
  src/world/area_mac/mac_01: none), so player 0 at the first world frame of mac_01 can only hold the wind of arn_02. The seed
  (the same id AMBIENT_WIND) keeps it: `stopped 0`. The real edit stops it: `stopped 1`.
## Touch only (places)
3ds/src/pm_warp.c: warp_end_intro (the 4 lines from `bgm_set_song(0, ...` to its pm_log), pm_warp_world_mode (:1416, one block at
its start). New 3ds/tools/azt.d/AMBWARP.json, 3ds/tools/bench.d/AMBWARP.json. Nothing else (no open row edits these places).
## Exact edits
1. warp_end_intro, after `sfx_stop_env_sounds();` add `play_ambient_sounds(-1, 0); /* AMBWARP: the MSEQ ambient of the left map (a
   warp may land on a map that does not clear it) */`. Change its log text to `"warp: songs, loop sounds and ambient stopped,
   demoState was %d, game mode %d"`.
2. pm_warp_world_mode, at its start: debug switch ambchk (read once, as `static int ac = -1; if (ac < 0) ac =
   pm_debug_has("ambchk");`): when on, `pm_log_evt("ambchk f%u map %s stopped %d", frame, gAreas[gGameStatusPtr->areaID].maps[
   gGameStatusPtr->mapID].id, rc == AU_RESULT_OK || rc == AU_AMBIENCE_ERROR_3);` with `AuResult rc = snd_ambient_is_stopped(0);` before it (ORACLE8
   correction 1: no ambient player at all, numActivePlayers 0, gives AU_AMBIENCE_ERROR_3 and is silent too: row_AMBTITLE.md) (include "audio/public.h" if the file lacks it).
3. Scenario (all fields of the azt.d format, as 3ds/tools/azt.d/FPS60W2.json) in 3ds/tools/azt.d/AMBWARP.json: name
   `amb_warp_arn02_mac01`, status `unverified`, subject, proves, debug `warp=arn_02:0 story=ch0_toad_town livewarp=300:mac_01:0
   ambchk loglevel=debug det` (correction 2: the menu warp, not the tour), until `ambchk f\d+ map mac_01 `, cap 45, deadline 20, first_marker `warp: arn_02 entry 0`,
   expects `none`, needs_picture false, expected_events [{name "arn_02 reached", regex `ambchk f\d+ map arn_02 `}, {name
   "menu warp ran", regex `livewarp: scene frame 300 to mac_01 entry 0`}, {name "warp_end_intro ran", regex `warp: songs, loop
   sounds and ambient stopped`}, {name "ambient stopped", regex `ambchk f\d+ map mac_01 stopped 1`}], reason. Bench in bench.d/AMBWARP.json: name
   `amb_warp_arn02_mac01`, kind `azahar`, speed `normal`, default false, purpose, scenario, cap 45, the same until, tags
   ["azahar"], gates [].
4. Commit: `AMBWARP: warps stop the MSEQ ambient of the left map (play_ambient_sounds(-1)), ambchk probe and scene`.
## Gate (expected values)
1. Seeded fault (a value change, first): edit 1 with `play_ambient_sounds(AMBIENT_WIND, 0)` (the same id: no stop).
   `python 3ds/tools/bench.py run amb_warp_arn02_mac01 --row AMBWARP --why "AMBWARP seeded fault"`: the event shows `stopped 0`
   (FAIL). Also check the arn_02 entry line `ambchk ... map arn_02 ...` exists (the wind map was reached).
2. Edit 1 with -1. Same command, --why "AMBWARP gate": PASS with `ambchk f<n> map mac_01 stopped 1`.
3. Build, memcheck OK. nd_host PASS. Budget 6 Azahar starts. The user checks the sound on the console.
## Stop and end
`stopped 0` with the real edit, no arn_02 line, or a tool fault: stop, commit, hand back the exact output (CLAUDE.md 4-5).
Delete this row file in the completing commit.
