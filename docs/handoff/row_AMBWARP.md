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
## Touch only (places)
3ds/src/pm_warp.c: warp_end_intro (the 4 lines from `bgm_set_song(0, ...` to its pm_log), pm_warp_world_mode (:1416, one block at
its start). New 3ds/tools/azt.d/AMBWARP.json, 3ds/tools/bench.d/AMBWARP.json. Nothing else (no open row edits these places).
## Exact edits
1. warp_end_intro, after `sfx_stop_env_sounds();` add `play_ambient_sounds(-1, 0); /* AMBWARP: the MSEQ ambient of the left map (a
   warp may land on a map that does not clear it) */`. Change its log text to `"warp: songs, loop sounds and ambient stopped,
   demoState was %d, game mode %d"`.
2. pm_warp_world_mode, at its start: debug switch ambchk (read once, as `static int ac = -1; if (ac < 0) ac =
   pm_debug_has("ambchk");`): when on, `pm_log_evt("ambchk f%u map %s stopped %d", frame, gAreas[gGameStatusPtr->areaID].maps[
   gGameStatusPtr->mapID].id, snd_ambient_is_stopped(0) == AU_RESULT_OK);` (include "audio/public.h" if the file lacks it).
3. Scenario `amb_warp_arn02_mac01`: debug `warp=arn_02:0 story=ch0_toad_town tour=arn_02:0,mac_01:0 tourtime=3 ambchk loglevel=debug
   det`, until `ambchk f\d+ map mac_01 `, cap 45, expected event `ambchk f\d+ map mac_01 stopped 1`. Bench `amb_warp_arn02_mac01`,
   kind azahar, scenario of the same name.
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
