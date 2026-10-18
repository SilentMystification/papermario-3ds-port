# Agent rules (slice agents)

## Scope
- Do only the row you get. Do not start other rows.
- Lane rules: see docs/LANES.md. Work only in your lane.
- Sub-model rule: the orchestrator picks the model. Do not start other agents.

## Do not
- Do not push to git.
- Do not add Claude or Anthropic attribution to commits or PRs.
- Do not write a stub or stand-in for decomp code or data.
  A port-layer shim is allowed only for real N64 OS or hardware functions with no game logic.
  If a stub looks necessary, write `BLOCKED: <reason>` in HANDOFF.md and stop.
- Do not hide a fault with a guard. Fix the root cause. Grep every caller before you edit a function.
- Do not run `git checkout` on tracked files. Do not commit logs, traces or shots.
- Do not start azahar.exe with any argument except the .3dsx path. Use 3ds/run_azahar.ps1.
- Do not leave `fast` in debug3ds.txt.
- Do not hand-roll locks or sleep loops. Use libctru natives.

## Reading
- Read docs/INDEX.md first. Do not search the tree.
- Read logs with the last 40 lines only. Never read a whole log.
- Read files with offset and limit.
- Use summary scripts: 3ds/tools/phase_report.py, rom_trace_classify.py, rom_trace_check.py, check_data_layout.py.
- Do not read docs/HISTORY.md. Grep it for one topic only.
- Read HANDOFF.md only if the task needs it.

## Tests and memory
- Test only what the change touches: one scene or map, fixed short length, one text check.
- Fix every fault a test finds. Name the areas you did not test.
- Use -Turbo -Smoke -FaultStop and a warp preset for feature tests. Never wait open ended.
- Lighter check: smoke on presets yard and toad, then shots of touched scenes only.
- STOP RULE: stop when the done check of your row passes. Run at most 3 build and run rounds per row, then report what blocks you. Do not add checks that the row does not name. A fault outside your row becomes a new row in 3ds/SLICES.md (one line). Do not fix it in this slice. Before each Azahar run, log one line with the expected length. If one step takes more than 15 minutes, log the reason.
- Fast checks: each Azahar start costs about 17 s. Check many maps in ONE run with `tour=<map:entry,...>` (one start, many maps). Run `golden.py` with scene names for the touched scenes only. Skip the yard and toad smoke when `golden.py` already ran yard and toad in the same build. Run the full golden set and the full tour only at a milestone. Use a high -TurboFactor for runs that do not measure timing.
- Full tour and 4-scene A/B only for map change path, boot, title, file select or intro.
- Screenshots only when needed. One contact sheet at most.
- Memory: no size cap per slice (user decision 2026-10-18). Report the 3dsx size change. Do not stop a slice for its size.
- Optimization code starts only after S28. S28 waits for the empty Azahar queue and the user.

## Logging and finish
- Append one line per step to 3ds/PROGRESS.log: `HH:MM ID text`.
- Add at most 3 lines to HANDOFF.md per slice. Put detail in docs/HISTORY.md.
- Move the done row from 3ds/SLICES.md to docs/SLICES_DONE.md.
- Put questions for the user in 3ds/QUESTIONS.md. Keep working on other items.
- Commit locally in plain English. Commit only your own files.
- Write notes, docs, commit messages and reports in ASD-STE100 English: one instruction per sentence, active voice, simple tenses, no semicolons, one plain verb, no praise. Keep every number, path, command and switch name exact.
- Report in 5 lines or fewer: slice id; commit hash; result; checked and not checked; BLOCKED or not.
