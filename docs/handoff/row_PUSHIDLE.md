# Row PUSHIDLE: console push watchers and tails must end on their own

## User facts
User (2026-11-09): "There are a ton of stale background tasks, why aren't they getting closed properly? The live tail should
terminate when the watchdog doesn't get a message for a while or when the session terminates properly."
Found on 2026-11-09: 88 tail.exe processes. Most were agent Monitor tails, one new tail per check, never stopped (the orchestrator
stopped them). From the push tools: netload tails `tail -n +1 -f build3ds/.netload_attempt_*.log` from 2026-11-09 01:58 and 02:40
still ran; an interactive push watches for 86400 s (go.ps1:147).

## Start state
Worktree build3ds/wt/PUSHIDLE, branch PUSHIDLE from main. PM_ROW=PUSHIDLE, PM_LANE=PUSHIDLE. One Haiku agent. Tools only.

## Steps
1. 3ds/push_3ds.sh live log watch: add an idle limit. When no log line comes from the console for `--idle <s>` seconds
   (default 300), print "live log idle <s> s: stop" and exit 0. A line resets the timer. -t stays the hard limit.
2. go.ps1:147: an -Interactive push keeps -t 86400 as the hard limit but passes --idle 300 (a console that is off or at the HOME
   menu stops sending lines). Add -IdleSeconds <n> to go.ps1 to change it.
3. Parent exit: push_3ds.sh records its parent PID at start; the watch loop exits when the parent is gone (kill -0 on Git Bash, or
   a PowerShell check). go.ps1 stops push_3ds.sh in a finally block when it ends or is stopped.
4. Every background child in push_3ds.sh (the netload `tail -n +1 -f "$outfile" &` at about line 345, the docker run, the python
   socket reader) is killed by one `trap cleanup EXIT INT TERM HUP` that kills the recorded child PIDs.
5. Same review for 3ds/run_azahar.ps1 -Interactive and 3ds/azt.ps1: any tail or reader they start ends with the script.
6. docs: the push section of the tools doc (search INDEX.md) states the idle rule.

## Gates
- Test with no console: `sh 3ds/push_3ds.sh --attach --ip 192.168.1.250 -t 60 --idle 10` (unused address) ends within 15 s with
  the idle line, and no tail.exe or bash child of it remains (list with PowerShell Get-CimInstance Win32_Process).
- Stop a go.ps1 push with Ctrl+C or Stop-Process during the netload stage: no tail.exe remains after 5 s.
- bash -n 3ds/push_3ds.sh passes. No console push in this row (the user runs the next push).
