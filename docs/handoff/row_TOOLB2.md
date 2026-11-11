# Row TOOLB2: shared run ids, a cold gate that is cold, a cut periodic shot at a pause (Haiku)

(`sh 3ds/tools/wt_new.sh TOOLB2` or the existing build3ds/wt/TOOLB2, PM_ROW=PM_LANE=TOOLB2. Output to build3ds/out/. Delete this row file in the completing commit.)

## Done
Items 1-3 (shared run lock and ids, battle_start_f182_cold -NoWarm) are merged on main: cb5659dad. Gate (a) passed.

## Item 4: a periodic shot cut by the pause fails the shot copy (FFOG1 run 204051, commit 15b875183, 76 of 77 full, 0076 short)
Cause (code read):
- That mim11_floor version had debug `warp=mim_11:0 det wzcull shots padscript=up*180` (no shotwin, no shotmode) and timed out (its
  dlcwz until line never came). Its shots are the periodic dump: pm_gpu.c:1808 returns or falls through with msg empty, so
  pm_gpu.c:1832 logs no "shot N frame F" line. 0076 was not a logged shot: no shot of that run was logged.
- run_azahar.ps1:447-448 pauses Azahar at the timeout; the dump was writing 0076, so the file stays short.
- With no logged shot ($logged.Count 0), line 471 checks every file, the loop waits the full 30 s for 0076, and the cut-file cleanup at
  line 491 runs only when `$logged.Count -gt 0`. So 0076 lands in $short (line 497) and line 501 prints SHOT COPY FAILED.
Edit (3ds/run_azahar.ps1 only): after the closing `}` of the block at lines 491-493, insert:
```
        # TOOLB2: a periodic dump (switch shots, no shotwin or shotmode) logs no shot line (pm_gpu.c dump_shot), so $logged is empty and the
        # cleanup above does not run. The dump writes one file at a time in number order (the folder is cleared at the start, line 281), so
        # only the highest number can be cut by the pause: a short highest file is that cut, removed before the recount below.
        if ($pausedAtUntil -and $logged.Count -eq 0) {
            $topF = Get-ChildItem "$sd\shots" -File -ErrorAction SilentlyContinue | Where-Object { $_.Name -match '^\d{4}_.*\.ppm$' } | Sort-Object Name | Select-Object -Last 1
            if ($topF -and $topF.Length -ne $shotFull) { Write-Output "SHOT FILES: $($topF.Name) ($($topF.Length) B) cut by the pause, removed"; Remove-Item $topF.FullName -Force -ErrorAction SilentlyContinue }
        }
```
The 30 s wait stays in that one case (a paused periodic run with a cut file); every other path is unchanged.

## Gate
(a) Done (cb5659dad).
(b) Parse check: `powershell -NoProfile -Command "$e=$null; [void][System.Management.Automation.Language.Parser]::ParseFile((Resolve-Path '3ds/run_azahar.ps1'), [ref]$null, [ref]$e); $e.Count"` prints 0.
(c) One Azahar start (it also runs the edited script): `python 3ds/tools/bench.py run battle_start_f182_cold --row TOOLB2 --why "TOOLB2 cold gate and shot path"`.
    Its new runs.jsonl record has "warm":0 and result log_pass, and the output has no SHOT COPY FAILED.
Stop: any gate fails, or a test-tool fault: commit and hand back the exact output (CLAUDE.md 4-5).
