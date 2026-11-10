# Row STEREOCRASH: find the store that overwrites RenderTaskListIdx in a stereo frame (user OK 2026-11-10: one gdb run)

(Diagnostic, 1 Azahar start. `sh 3ds/tools/wt_new.sh STEREOCRASH` on branch AZTSTEREO 2c29fb04a; it has the merlee_stereoat scenario. PM_ROW=PM_LANE=STEREOCRASH. CLAUDE.md rules 1-7. No git stash. Foreground waits.)
## Fact (oracle)
`RenderTaskListIdx` (model.c:1375) becomes 0x01D46D04, a heap pointer, in one frame after "merlee: darken done" (stereo on from f500). queue_render_task and execute_render_tasks then read RenderTaskLists[huge] = 0, and the game calls appendGfx through NULL.
## Touch only
Nothing in src. Output goes to files in this worktree only.
## Run (rule 1 exception: bench.py has no gdb option; the user approves this one run)
1. Build: `PM_ROW=STEREOCRASH PM_LANE=STEREOCRASH sh 3ds/build.sh > build3ds/out_build.txt 2>&1`
2. Write build3ds/wt/STEREOCRASH/build3ds/gdb_watch.txt:
```
watch -l RenderTaskListIdx if RenderTaskListIdx > 2
commands
bt 12
info registers
x/6i $pc-12
end
```
3. Start Azahar once with the merlee_stereoat debug string (the "debug" field of merlee_stereoat in 3ds/tools/azt_scenarios.json), the `-Gdb -GdbScript` switches of 3ds/run_azahar.ps1 (usage in its header lines 1-20) and -Timeout 60. Write the status line first (rule 1).
## Expected
build3ds/last_gdb.txt stops at the store: the backtrace names a function, file and line. Copy the bt, the registers and the 6 instructions into the handback.
## Stop
- Stopped at the watch: hand back WRITER FOUND with the bt. The oracle names the fix.
- The game crashed with no stop (the Azahar JIT may not check watchpoints): hand back WATCH NOT HIT with last_exception.txt.
- 1 start only. STOP RULE: 90 min or 150 tool calls.
Affected scenarios: merlee_stereoat.
