# Row BOTHUD: the bottom screen HUD draws also when logging is off (prof push), and a HUD toggle shows at once

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names.)
## User facts (2026-11-12, console pushes)
After a push the bottom screen sometimes shows nothing, also when the settings are toggled. The HUD is missing even when the
user turns it on again. Push values must override options.ini at boot only, and a live change in the menu must win afterwards.
## Cause (ORACLE8 code read, main 4404177f0)
- The push profile prof (bench_defs.json profiles: "prof=30 ...") turns logging off: pm_log_prof_apply (pm_log.c:655-660) ->
  pm_log_prof_row(1) sets the live, disk and console levels to -1 (:651). Then qon() is false.
- The game thread only copies the HUD rows (pm_gpu_hud, pm_gpu_ui.c:111-122, `hud_new = 1`). The log sink thread draws them in
  pm_gpu_hud_flush (pm_gpu_ui.c:156), called from con_flush (pm_log.c:879). sink_main (pm_log.c:885-888): with qon() false it
  waits `LightEvent_Wait(&ev_sink)` with no timeout. ev_sink is signalled only by a log state change. So with logging off the
  HUD rows, the footer and the frame are never drawn: the bottom screen stays empty. Turning the HUD option on (pm_opt.dbg_hud,
  pm_main.c:843) sets hud_new again, but the sink still waits: nothing shows. This is (a) and (b).
- Not a write-back: the options file stores the saved levels (sv_* at pm_options.c:109-110, :225), not the run levels of the
  push. The boot args apply once at boot (pm_options.c:229-241, pm_log_prof_apply at pm_main.c:946). That is the rule the user
  wants: no change there.
Start: `sh 3ds/tools/wt_new.sh BOTHUD`. PM_ROW=BOTHUD PM_LANE=BOTHUD on every call.
## Touch only (places)
3ds/src/pm_log.c: sink_main line `else LightEvent_Wait(&ev_sink);` (:888). 3ds/src/pm_gpu_ui.c: pm_gpu_hud_flush (one counter).
3ds/src/pm_main.c: one probe block next to the shotbottom block (:813-818). New azt.d/BOTHUD.json and bench.d/BOTHUD.json.
## Exact edits
1. pm_log.c :888: `else LightEvent_WaitTimeout(&ev_sink, 100000000LL); /* BOTHUD: with logging off the sink still draws the HUD rows
   at 10 Hz (pm_gpu_hud_flush in con_flush) */`.
2. pm_gpu_ui.c: `unsigned pm_hud_draws;` and in pm_gpu_hud_flush, after the early return `if (!(hud || foot || frame) || ...)`
   and the menu test, `pm_hud_draws++;`.
3. pm_main.c probe: switch `hudchk` (read once): at frame 300, `pm_log_evt("hudchk f%u draws %u", frame, pm_hud_draws);`.
4. Scenario (all fields of the azt.d format, as 3ds/tools/azt.d/FPS60W2.json) in 3ds/tools/azt.d/BOTHUD.json: name
   `bothud_prof`, status `unverified`, subject, proves, debug `warp=mac_01:0 pos=0:0:200 prof=30 hudchk`, until `hudchk f300 `,
   cap 30, deadline 20, first_marker `prof=1 log=off` (pm_log_prof_apply prints it at boot when prof is on), expects `none`,
   needs_picture false, expected_events [{name "prof on", regex `prof=1 log=off`}, {name "hud draws", regex
   `hudchk f300 draws [1-9]\d+`}], reason. Bench in bench.d/BOTHUD.json: name `bothud_prof`, kind `azahar`, speed `normal`,
   default false, purpose, scenario, cap 30, the same until, tags ["azahar"], gates [].
5. Commit: `BOTHUD: the log sink draws the bottom HUD at 10 Hz with logging off (prof pushes), hudchk probe`.
## Gate (expected values)
1. Seeded fault (a value change, first): edit 1 with the timeout `0x7FFFFFFFFFFFFFFFLL` (waits as before). Run the bench:
   `hudchk f300 draws 0` or 1 (FAIL of the event).
2. Edit 1 as written. Run: PASS, draws 10 or more. Build, memcheck OK. nd_host PASS. Budget 6 Azahar starts.
3. The user checks the bottom screen on the console after a prof push (and the HUD toggle).
## Stop and end
draws below 10 with the real edit, or a tool fault: stop, commit, hand back the exact output (CLAUDE.md 4-5). Delete this row
file in the completing commit.
