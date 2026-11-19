# ws_AZWARM: no restart key to a kept Azahar younger than 20 s (tool fault)
Counter: none (tool fault). Model: Haiku (closed edits). Edit only 3ds/run_azahar.ps1, 3ds/tools/azslot.ps1, 3ds/tools/azt.ps1, 3ds/tools/bench.py, and add 3ds/tools/azt.d/AZWARM.json, 3ds/tools/bench.d/AZWARM.json.
## Facts (pool.log, proof code since 2026-11-08T17:48:20, 4562 warm tries, all 156 fails are "boot no")
F1. FACT: first warm key after a cold start, by Azahar age: under 16 s 3 of 13 boot. 16 to 30 s 49 of 57. 30 to 90 s 76 of 95.
F2. FACT: a failed key kills the Azahar and starts it cold (run_azahar.ps1:398, :405). The next run comes about 14 s later, so it is again under 16 s. NAT1D got cold 4 times in a row (pool.log 2026-11-18 23:51:17 to 23:52:32, ages 21, 12, 15, 14 s).
F3. FACT: bench.py base stores a base only with 2 warm runs (bench.py:1766). The wait goes before warm.start (run_azahar.ps1:359): the azt.ps1 deadline does not run yet (azt.ps1:635). The queue cap is cap + 30 s (azslot.ps1:353).
## Edits (directory order: 3ds/tools, then 3ds)
E1. azslot.ps1:19. After `$script:AzIdleSec = 3600` add a comment line "# AZWARM: the restart key goes to a kept Azahar only when it is this many seconds old (ws_AZWARM F1)." and `$script:AzWarmMinAgeSec = 20`.
E2. azslot.ps1, after the closing `}` of Get-WarmState (line 169), add:
```
function Get-WarmWaitSec([double]$ageSec) {
    # AZWARM: seconds to wait before the restart key to a kept Azahar of age $ageSec (0 when it is old enough)
    return [Math]::Max(0.0, $script:AzWarmMinAgeSec - $ageSec)
}
```
E3. run_azahar.ps1:353. Before: `    $hk = Get-SlotHotkey $slotK "Restart Emulation"`. After: these 6 lines first, then that line unchanged:
```
    $azAge = ((Get-Date) - $kept.StartTime).TotalSeconds # AZWARM: a young Azahar drops the first key (ws_AZWARM F1)
    $azWait = Get-WarmWaitSec $azAge
    if ($azWait -gt 0) {
        Write-PoolLog $slotK "warm age wait" $kept.Id $kept.Id ("age {0:N1} s, wait {1:N1} s before the key" -f $azAge, $azWait)
        Start-Sleep -Milliseconds ([int][Math]::Ceiling($azWait * 1000))
    }
```
E4. azt.ps1, after the closing `}` of the `foreach ($d in $dlCases)` loop (line 172), add 4 cases in the same form as lines 163-172. Cases (age -> want): 0 -> 20, 14.5 -> 5.5, 20 -> 0, 300 -> 0. Compare with `-eq`. Line text: `"{0} warm age wait: age {1} s -> wait {2} s (want {3} s)"`. A miss does `$fails++`.
E5. bench.py:1160. Before: `        elif t.startswith("stale "):`. Add above it: `        elif t.startswith("warm age wait"):` and `            ev["warm age wait"] += 1`.
E6. New azt.d/AZWARM.json: `{"scenarios": [ ... ]}` with one copy of the spot_mac01_drec object of azt.d/HOSTCORE2.json. Change only: name `azwarm_cold`, extra `["-NoWarm"]`, reason "AZWARM: spot_mac01_drec with -NoWarm: a forced cold start, so the next run of the list restarts a young Azahar." Keep CRLF if HOSTCORE2.json has CRLF.
E7. New bench.d/AZWARM.json: one copy of the bench of bench.d/HOSTCORE2.json. Change only: name and scenario `azwarm_cold`, purpose "AZWARM: forced cold start, the first run of the AZWARM gate list."
## Gate (1 build, 2 Azahar starts, budget 4 starts)
G1. Seeded fault: set `$script:AzWarmMinAgeSec = 0`. Run `powershell -NoProfile -File 3ds/tools/azt.ps1 -SelfTest > build3ds/out/azwarm_seed.txt`. Want: 2 FAIL warm age wait lines and "SELFTEST FAILED". Set it back to 20.
G2. Same command to build3ds/out/azwarm_self.txt. Want: 4 PASS warm age wait lines and "SELFTEST PASS".
G3. `PM_LANE=AZWARM sh 3ds/build.sh > build3ds/out/azwarm_build.txt 2>&1` (timeout 600000). Then with PM_ROW=AZWARM PM_LANE=AZWARM: `python 3ds/tools/bench.py run azwarm_cold,spot_mac01_drec --row AZWARM --why "AZWARM gate: young kept Azahar" > build3ds/out/azwarm_run.txt 2>&1` (timeout 600000).
G4. Want, all of: rc 0. azwarm_run.txt has 2 `azslot: slot` lines, the first ends ` cold (option)`. build3ds/azslots/pool.log has a line of tree AZWARM with `warm age wait` and a wait above 0, then the `warm try` of run 2 at least 20 s after the `start cold: option` line of run 1.
G5. Report, not judged: the end of the second azslot line (` warm` or ` cold (restart failed)`). FACT F1: at 16 to 30 s about 1 key in 7 is still lost.
## Fix rules
- A PowerShell parse error or a typo of E1-E7: fix it yourself.
- A G4 miss: send build3ds/out/azwarm_run.txt and the last 20 pool.log lines of tree AZWARM to the oracle. Do not change 20 or the scenario.
## Stop and hand-back
Stop: G1-G4 pass, one commit of the changed files that deletes this page. Hand-back (SubagentHandback): commit hash, the G2 PASS count, the 2 azslot lines, the warm age wait line, the G5 value, "counter none".
