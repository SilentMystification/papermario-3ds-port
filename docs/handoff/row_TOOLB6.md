# Row TOOLB6: crash symbols reach last_crash.txt (lane ELF path, lane output path)

(Model: Haiku. The agent reads only this file, CLAUDE.md and what this file names.)
## Facts (HC4e4 gate 2, run 238358, mdl5_iwa10_det)
"SCENARIO FAILED: run_azahar reported crash symbols (rc 2)", the until line matched, no last_crash.txt anywhere, and the next run on
the slot overwrote last_log.txt, last_unmapped.txt and the emulator log. The crash evidence of that run is lost.
## Cause (oracle code read)
- run_azahar.ps1:222-223 sets `$outLane = $out` and then `$out = "$outLane\slot$slotK"`. pm_3ds.elf is in the lane folder ($outLane).
- run_azahar.ps1:636 (TOOLB1) mounts `${out}:/elf` (the slot folder, no ELF) for addr2line, so addr2line prints nothing, and
  `Tee-Object -FilePath "$out\last_crash.txt"` gets no line: no file. End-Run 2 still runs.
- azt.ps1:586 reads last_crash.txt from the lane folder ($outDir), not the slot folder. The addresses and the unmapped list are only
  in run_azahar's console output, which azt.ps1 does not keep.
## Start
`sh 3ds/tools/wt_new.sh TOOLB6` (build3ds/wt/TOOLB6, branch TOOLB6 from main). Not at the same time as TOOLB4 (same file).
## Touch only
3ds/run_azahar.ps1 (the `if ($addrs) {` block at :633-638).
## Exact edit
Replace the docker line (:636) with:
```
    $sym = @(& docker run --rm -v "${outLane}:/elf" devkitpro/devkitarm:latest /opt/devkitpro/devkitARM/bin/arm-none-eabi-addr2line -f -C -p -e /elf/pm_3ds.elf $addrs)
    # TOOLB6: the ELF is in the lane folder; the file always gets the addresses and the unmapped sites, also when addr2line prints nothing
    $crashTxt = @("run end: $reason", ("addresses: " + ($addrs -join " "))) + @(Get-Content "$out\last_unmapped.txt" -ErrorAction SilentlyContinue | Select-Object -First 10) + $sym
    $crashTxt | Set-Content "$outLane\last_crash.txt"; $crashTxt | Set-Content "$out\last_crash.txt"; $crashTxt | Write-Output
```
## Gate
- Parse check: `powershell -NoProfile -Command "$e=$null; [void][System.Management.Automation.Language.Parser]::ParseFile((Resolve-Path '3ds/run_azahar.ps1'), [ref]$null, [ref]$e); $e.Count"` prints 0.
- The real path is proven by the next crash run (HC4e4 step C below uses it); no Azahar start in this row.
## Stop and end
A failed gate or an edit that does not fit the code: commit and hand back the exact output (CLAUDE.md 4-5).
At the end delete this row file in the completing commit. Hand back: the ready hash and the gate result.
