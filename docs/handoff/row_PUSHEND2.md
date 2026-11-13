# Row PUSHEND2: go.ps1 prints one PUSH END line with the result of push_3ds.sh, in every mode

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names.)
## User fact (2026-11-12)
The push must succeed or end with one clear end line (ok or failed, the reason) and an exit code. Never a silent wait.
## Cause (oracle code read, ORACLE8, main 3c7d977ab, 3ds/go.ps1)
- :246-252: the end line "PUSH END rc N <date>" is written only with -Fg (the detached child). A foreground -Interactive push has
  no end line. The line holds no reason.
- :188: any exit code other than 0 is a failure. PUSHEND1 adds exit 6 (the 3dsx was sent, the game sent no line) and writes the
  result line to build3ds/out/push_result.txt.
- Correction 1 (ORACLE8, after WIP 285b4842a): go.ps1 starts the detached child with Win32_Process.Create (:229). WMI starts that
  process from its service with the default environment, not the caller's. So test (d)'s PM_SRC3DSX, PM_3DSLINK, PM_ARCHIVE_DIR
  and PM_PUSH_LOG never reach the child: it checks build3ds/build_key.txt (missing in a worktree) and ends `PUSH RESULT failed
  rc 5: stale 3dsx` (build3ds/out/push_o_20261112-193513.txt). Test (d) passed before only because it checked `PUSH END` alone.
Start (PUSHEND1 merged): `sh 3ds/tools/wt_new.sh PUSHEND2`. PM_ROW=PUSHEND2 PM_LANE=PUSHEND2 on every call.
## Touch only (places)
3ds/go.ps1: the exit check line in Push-One (`if ($LASTEXITCODE -ne 0) { throw "push_3ds.sh exited $LASTEXITCODE" }`, :188) and
the `finally` block of the push verb (:246-253), and the `$cmdLine = ...` line of the detached start (:228). 3ds/tests/push/run.sh: the two lines after `until grep -q 'PUSH END'` in (d) (:71-73).
## Exact edits
1. Right before :188 add:
   `            if ($LASTEXITCODE -eq 6) { Write-Host "=== push ${t}: sent, the game sent no line ==="; return 6 } # PUSHEND2`
2. Replace the whole `finally { if ($Fg) { ... } }` block (:246-253) with:
```
    } finally {
        # PUSHEND2: one end line in every mode: rc, time and the PUSH RESULT line of push_3ds.sh (only one written by this run).
        $rf = Join-Path $repo 'build3ds/out/push_result.txt'
        $res = $null
        if ((Test-Path $rf) -and (Get-Item $rf).LastWriteTime -ge (Get-Process -Id $PID).StartTime) { $res = Get-Content $rf | Select-Object -Last 1 }
        if (-not $res) { $res = "PUSH RESULT failed rc ${rc}: go.ps1 ended before push_3ds.sh wrote a result (see the lines above)" }
        $endLine = "PUSH END rc $rc $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss') $res"
        Write-Output $endLine
        New-Item -ItemType Directory -Force -Path (Join-Path $repo 'build3ds/out') | Out-Null
        Set-Content -Path (Join-Path $repo 'build3ds/out/push_last.txt') -Value $endLine
    }
```
3. run.sh (d): replace the two lines starting `if grep -q 'PUSH END' build3ds/out/push_last.txt` and `else echo "FAIL detached PUSH END
   within 60 s"` with:
```
if grep -q 'PUSH END rc 6 .*PUSH RESULT sent-no-log rc 6' build3ds/out/push_last.txt 2>/dev/null; then echo "ok   detached PUSH END ($(tr -d '\r\n\357\273\277' <build3ds/out/push_last.txt))"
else echo "FAIL detached PUSH END with the result ($(cat build3ds/out/push_last.txt 2>/dev/null))"; fail=1; fi
```
3b. Correction 1: replace the line `$cmdLine = 'powershell.exe ' + ...` (:228) with these 3 lines:
```
        # PUSHEND2: Win32_Process.Create gives the child the default environment: the PM_* and N3DS_IP values of this call go through cmd's set.
        $envSet = ''; foreach ($e in @(Get-ChildItem Env: | Where-Object { $_.Name -like 'PM_*' -or $_.Name -eq 'N3DS_IP' })) { $envSet += 'set "' + $e.Name + '=' + $e.Value + '" && ' }
        $cmdLine = $envSet + 'powershell.exe ' + ($childArgs -join ' ') + ' > "' + $log + '" 2> "' + $log + '.err"'
```
4. Commit: `PUSHEND2: go.ps1 PUSH END line in every mode with the PUSH RESULT of push_3ds.sh, exit 6 is a sent push`.
## Gate (expected values)
0. Correction 1: edits 1-3 and gate 1 are done in 285b4842a. Make edit 3b, then run gate 2 again.
1. Seeded fault: make edit 3 only, `sh 3ds/tests/push/run.sh > build3ds/out/push_test_before.txt 2>&1`. Expected `FAIL detached
   PUSH END with the result (PUSH END rc 1 ...)`, `PUSH TESTS FAIL`.
2. Make edits 1-2, run again into build3ds/out/push_test.txt. Expected `ok   detached PUSH END (PUSH END rc 6 <date> PUSH RESULT
   sent-no-log rc 6: ...)`, every other case ok, `PUSH TESTS PASS`. No console push, no Azahar start. Counter: none (tool row).
## Stop and end
A failed gate, a line that does not match the text above, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5).
Delete this row file in the completing commit. Hand back: the ready hash and the two test outputs.
