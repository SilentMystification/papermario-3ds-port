# Row TOOLB4: a run refuses to start while an azahar.exe that is not a slot copy runs

(Model: Haiku. The agent reads only this file, CLAUDE.md and what this file names.)
## Facts (DETBASE step 1, 2026-11-10, commit f748db329)
Run 217833 (warm restart, slot 1) logged up to `#23 audiohash f120` and then nothing for 10 s: "game log silent for 10 s before the
first frame: modal dialog suspected", 0 shots. Run 217012 just before it reached f388 4.5 s after its restart. The user then closed a
stray Azahar instance that was open on the machine.
## Cause (oracle code read)
- run_azahar.ps1:247-251 stops a stray azahar.exe only on a cold start, and only one whose path is this slot's exe ($exe). An Azahar from
  another folder (the user's own, or a copy elsewhere) is never seen by run_azahar.ps1 or azt.ps1, and it shares the host CPU and GPU.
- The emulated game then runs slowly. spot_mac01_det runs with prof=4 (log off): its log only gets event lines, under det the audiohash
  line every 120 frames (pm_main.c:693). The stall rule (run_azahar.ps1:421-424, $Stall 10 s with no log growth) then stops the run.
- A warm restart itself is not the cause: pool.log shows "warm wait boot yes log rotated yes after 5.4 s" for run 217833, and the game
  ran to f120.
## Start
`sh 3ds/tools/wt_new.sh TOOLB4` (build3ds/wt/TOOLB4, branch TOOLB4 from main). PM_ROW=TOOLB4 PM_LANE=TOOLB4 on every bench call.
Start it after row DETBASE is done (DETBASE runs alone).
## Touch only
3ds/run_azahar.ps1 (one block after line 217, the `$sd = $sl.sd; ...` line).
## Exact edit
After line 217 insert:
```
# TOOLB4: an azahar.exe that is not a slot copy (the user's own Azahar, or a copy from another folder) shares the host CPU and GPU.
# The game then runs slowly, and the stall rule below (no new game log line for $Stall s) stops a run with few log lines (prof mode,
# det audiohash every 120 frames) as silent. Refuse the run: the user closes that Azahar. Slot Azahars of other runs are allowed.
$slotExeRx = '^' + [regex]::Escape((Join-Path $script:AzRepo "build3ds\azslots\slot")) + '\d+\\azahar\.exe$'
$foreign = @(Get-CimInstance Win32_Process -Filter "Name='azahar.exe'" -ErrorAction SilentlyContinue | Where-Object { -not $_.ExecutablePath -or $_.ExecutablePath -notmatch $slotExeRx })
if ($foreign.Count -and -not $Interactive) {
    Release-Lock
    End-Run 3 ("HOST BUSY: azahar.exe not of a slot: {0}. Close it, then run again. Nothing was run." -f (($foreign | ForEach-Object { "pid $($_.ProcessId) $($_.ExecutablePath)" }) -join "; "))
}
```
If `Release-Lock` or `$script:AzRepo` is not defined at that line, stop and hand back (an edit that does not fit).
## Gate
- Parse check: `powershell -NoProfile -Command "$e=$null; [void][System.Management.Automation.Language.Parser]::ParseFile((Resolve-Path '3ds/run_azahar.ps1'), [ref]$null, [ref]$e); $e.Count"` prints 0.
- With no Azahar open outside the slots: `python 3ds/tools/bench.py run spot_mac01_det --row TOOLB4 --why "TOOLB4 no foreign Azahar"`:
  LOG CHECKS PASS and no HOST BUSY line. One Azahar start. The refusal path is the user's check (open an own Azahar, start a run).
## Stop and end
A failed gate, an edit that does not fit the code, or a test-tool fault: commit and hand back the exact output (CLAUDE.md 4-5).
At the end delete this row file in the completing commit. Hand back: the ready hash and the gate results.
