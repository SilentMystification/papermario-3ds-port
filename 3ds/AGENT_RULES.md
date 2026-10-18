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
- Full tour and 4-scene A/B only for map change path, boot, title, file select or intro.
- Screenshots only when needed. One contact sheet at most.
- Memory: at most 10 KB of code or data per slice. Report the 3dsx size change.
- Optimization code starts only after S28. S28 waits for the empty Azahar queue and the user.

## Logging and finish
- Append one line per step to 3ds/PROGRESS.log: `HH:MM ID text`.
- Add at most 3 lines to HANDOFF.md per slice. Put detail in docs/HISTORY.md.
- Move the done row from 3ds/SLICES.md to docs/SLICES_DONE.md.
- Put questions for the user in 3ds/QUESTIONS.md. Keep working on other items.
- Commit locally in plain English. Commit only your own files.
- Write notes, docs, commit messages and reports in ASD-STE100 English: one instruction per sentence, active voice, simple tenses, no semicolons, one plain verb, no praise. Keep every number, path, command and switch name exact.
- Report in 5 lines or fewer: slice id; commit hash; result; checked and not checked; BLOCKED or not.
