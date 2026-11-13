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
## Exact edits
1. src/state_startup.c after `sfx_clear_sounds();` add:
   `snd_ambient_stop_quick(0); /* AMBTITLE: the soft reset (return to title) does not reboot the audio: stop the ambient MSEQ player */`
   `reset_ambient_sounds();`
   and after them `{ static int n; pm_log_evt("startup %d ambient stopped %d", n++, snd_ambient_is_stopped(0) == AU_RESULT_OK); }`
   (n 0 is the first boot, 1 and up a return to the title. Declare
   `void pm_log_evt(const char* fmt, ...);` at the top if no header gives it).
2. pm_warp_tick after `bypass_tick();`: debug switch `titleret=<frame>` (read once): at that frame, if the game mode is
   GAME_MODE_WORLD, call pm_title_return() once and `pm_log_evt("titleret f%u", frame)`.
3. Scenario (correction 1: every field of the azt.d format, as 3ds/tools/azt.d/FPS60W2.json) in 3ds/tools/azt.d/AMBTITLE.json:
   name `amb_title_arn02`, status `unverified`, subject and proves (one sentence each), debug `warp=arn_02:0 story=ch0_toad_town
   titleret=240 loglevel=debug det`, until `startup [1-9]\d* ambient stopped ` (the boot line has n 0), cap 45, deadline 20,
   first_marker `warp: arn_02 entry 0`, expects `none`, needs_picture false, expected_events [{name "titleret", regex
   `titleret f240`}, {name "ambient stopped", regex `startup [1-9]\d* ambient stopped 1`}], reason (one sentence). Bench in
   bench.d/AMBTITLE.json: name `amb_title_arn02`, kind `azahar`, speed `normal`, default false, purpose, scenario, cap 45, the
   same until, tags ["azahar"], gates [].
4. Commit: `AMBTITLE: the soft reset to the title stops the ambient MSEQ player, titleret switch and scene`.
## Gate (expected values)
1. Seeded fault (a value change, first): edit 1 with `snd_ambient_stop_quick(1)` (another player index: player 0 stays).
   `python 3ds/tools/bench.py run amb_title_arn02 --row AMBTITLE --why "AMBTITLE seeded fault"`: `startup 1 ambient stopped 0`.
2. Index 0 again. Same command, --why "AMBTITLE gate": PASS with `titleret f240` and `startup 1 ambient stopped 1`.
3. Build, memcheck OK. nd_host PASS. Budget 6 Azahar starts. The user checks the title screen sound on the console.
## Stop and end
`stopped 0` with the real edit, no titleret line (the wind map was not in world mode at frame 240: hand back the mode), or a
tool fault: stop, commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in the completing commit.
