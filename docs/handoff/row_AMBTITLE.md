# Row AMBTITLE: the return to the title screen stops the ambient sound (MSEQ) that plays

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names.)
## User fact (2026-11-12)
Leave Gusty Gulch while its wind plays, return to the title screen: the wind keeps playing on the title screen.
## Cause (ORACLE8 code read, main 4404177f0)
- The port's "Return to Title Screen" is the game's soft reset: pm_title_return (3ds/src/pm_warp.c:1392-1403) sets
  GLOBAL_OVERRIDES_SOFT_RESET. step_game_loop (src/main_loop.c:120-145) fades in, then calls only sfx_stop_env_sounds() and
  set_game_mode(GAME_MODE_STARTUP) (:139-141). On the N64 the reset button rebooted the console, so no stop was needed.
- state_init_startup (src/state_startup.c:62-65) stops the songs (bgm_init_music_players) and the sfx (sfx_clear_sounds), not the
  ambient MSEQ player. AmbientSoundData (src/audio/ambience.c) keeps soundID AMBIENT_WIND and the PLAYING flag. The title screen
  never calls PlayAmbientSounds: nothing stops player 0. The wind plays on.
- reset_ambient_sounds (ambience.c:32-34) clears AmbientSoundData without a stop (it runs in load_engine_data at the first boot,
  src/main_loop.c:397, when nothing plays).
Start: `sh 3ds/tools/wt_new.sh AMBTITLE`. PM_ROW=AMBTITLE PM_LANE=AMBTITLE on every call.
## Touch only (places)
src/state_startup.c: the line after `sfx_clear_sounds();` (:65). 3ds/src/pm_warp.c: pm_warp_tick (:1965, one block after
`bypass_tick();`). New 3ds/tools/azt.d/AMBTITLE.json, bench.d/AMBTITLE.json. Not warp_end_intro or pm_warp_world_mode (AMBWARP).
## Correction 2 (ORACLE8, after run 835024): the stop is asynchronous, the check must run later
- snd_ambient_stop_quick(0) -> au_mseq_stop_quick (src/audio/mseq_player.c:168-181) only points player 0 at BlankMseqData with
  delay 1. The audio frame update (mseq_player.c:286-310, au_mseq_player_stop :703) ends it on a later audio frame. So
  snd_ambient_is_stopped(0) right after the call is not AU_RESULT_OK: the line printed 0 for the real edit and for the seed.
- At the first boot no ambient is loaded: numActivePlayers is 0 (src/audio/core/engine.c:848), au_mseq_check_player_index
  (mseq_player.c:65-71) returns AU_AMBIENCE_ERROR_3, and is_stopped passes that status on. That is why `startup 0` printed 0.
- So "silent" = snd_ambient_is_stopped(0) is AU_RESULT_OK (player 0 ended) or AU_AMBIENCE_ERROR_3 (no ambient player at all),
  read at the first title-screen frame, many audio frames after the stop. The fix itself (stop at startup) is complete: the
  audio thread runs on through the startup and the logos and ends the player.
## Exact edits (corrected)
1. src/state_startup.c after `sfx_clear_sounds();`: `snd_ambient_stop_quick(0);` and `reset_ambient_sounds();` as in 883c253a5.
   Replace the pm_log_evt line by `{ static int n; pm_log_evt("startup %d ambient stop issued", n++); }` (information only).
2. pm_warp_tick (the titleret block of 883c253a5 stays): after titleret fired, at the first step with get_game_mode() ==
   GAME_MODE_TITLE_SCREEN, once: `AuResult rc = snd_ambient_is_stopped(0); pm_log_evt("ambchk title f%u stopped %d rc %d", frame,
   rc == AU_RESULT_OK || rc == AU_AMBIENCE_ERROR_3, (int)rc);` (include "audio/public.h" for the AuResult names if needed).
3. Scenario fields (azt.d/AMBTITLE.json): until `ambchk title f\d+ stopped `, cap 45, deadline 20, first_marker `warp: arn_02 entry
   0`, expected_events [{name "titleret", regex `titleret f240`}, {name "silent at title", regex `ambchk title f\d+ stopped 1`}].
   bench.d/AMBTITLE.json: the same until.
4. Commit: `AMBTITLE: the soft reset to the title stops the ambient MSEQ player, titleret switch and scene`.
## Gate (expected values)
1. Seeded fault (a value change, first): edit 1 with `snd_ambient_stop_quick(1)` (the wind uses one player: index 1 is no player,
   player 0 plays on). Run the bench: `ambchk title f<n> stopped 0 rc <not 0>` (FAIL of the event).
2. Index 0 again. Run: PASS with `titleret f240` and `ambchk title f<n> stopped 1`.
3. Build, memcheck OK. nd_host PASS. Budget: the orchestrator reset it. The user checks the title screen sound on the console.
## Stop and end
`ambchk title ... stopped 0` with the real edit, no titleret line (the wind map was not in world mode at frame 240: hand back the mode), or a
tool fault: stop, commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
