<#
perf_capture.ps1 - one command for the O3DS performance baseline (plan docs/plans/post_s28_gaps.md section 2, run G = B7).

  powershell -NoProfile -File 3ds/tools/perf_capture.ps1 [-Ip 192.168.1.168] [-Only A,B,...] [-NoPrompt]

Runs (push o with -GameDebug, then the live log is copied to 3ds/logs/perf/<build>_<run>.txt):
  A boot, B title, C toad, D yard, F battle, E1 phase toad, E2 phase yard, G phase toad battle (B7: grabs, splits, VRAM, swirl, F03)
The E1, E2 and G runs need the FTP app on the console after the run: the script then calls go.ps1 pull o and runs phase_report.py.
Before each run the script waits for Enter: open the Homebrew Launcher and press Y on the console. After each run: HOME, close the game.
At the end it writes 3ds/logs/perf/baseline_<build>.txt. Send that file (and the 3ds/logs/perf folder) back.
Total time: about 15 minutes plus the prompts.
#>
param(
    [string]$Ip = '192.168.1.168',
    [string[]]$Only,
    [switch]$NoPrompt
)
$ErrorActionPreference = 'Continue'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
Set-Location $repo
$build = ((Get-Content 3ds/include/pm_build.h -Raw) -replace '(?s).*PM_BUILD_STR "([^"]+)".*', '$1').Trim()
$out = Join-Path $repo '3ds/logs/perf'
New-Item -ItemType Directory -Force $out | Out-Null

# name, switches, seconds, phase run (needs pull o and phase_report)
$runs = @(
    @{ n = 'A';  sw = 'loglevel=debug';                                                        s = 75; ph = $false },
    @{ n = 'B';  sw = 'audiostat pacestat logstat';                                            s = 60; ph = $false },
    @{ n = 'C';  sw = 'fast preset=toad audiostat pacestat logstat loglevel=debug';            s = 90; ph = $false },
    @{ n = 'D';  sw = 'fast preset=yard audiostat pacestat';                                   s = 60; ph = $false },
    @{ n = 'F';  sw = 'fast preset=yard battle audiostat pacestat';                            s = 60; ph = $false },
    @{ n = 'E1'; sw = 'fast preset=toad phase phasecsv';                                       s = 60; ph = $true },
    @{ n = 'E2'; sw = 'fast preset=yard phase phasecsv';                                       s = 60; ph = $true },
    @{ n = 'G';  sw = 'fast preset=toad battle phase phasecsv loglevel=debug';                 s = 60; ph = $true }
)
if ($Only) { $runs = $runs | Where-Object { $Only -contains $_.n } }

# push once with build, later runs reuse the 3dsx (-NoBuild)
$first = $true
foreach ($r in $runs) {
    Write-Host "=== run $($r.n): $($r.sw) ($($r.s) s) build $build ==="
    if (-not $NoPrompt) { Read-Host "On the O3DS: open the Homebrew Launcher, press Y. Then press Enter here" | Out-Null }
    $args = @('-NoProfile', '-File', '3ds/go.ps1', 'push', 'o', '-ip', $Ip, '-Seconds', $r.s, '-GameDebug', $r.sw)
    if (-not $first) { $args += '-NoBuild' }
    & powershell @args
    $first = $false
    $live = "build3ds/hw_live_log_$($Ip.Split('.')[-1]).txt"
    if (Test-Path $live) { Copy-Item $live (Join-Path $out "${build}_$($r.n).txt") -Force } else { Write-Host "no live log $live for run $($r.n)" }
    if ($r.ph) {
        if (-not $NoPrompt) { Read-Host "Start the FTP app on the O3DS (log pull). Press Enter here" | Out-Null }
        & powershell -NoProfile -File 3ds/go.ps1 pull o -ip $Ip
        if (Test-Path build3ds/hw_o3ds_log.txt) {
            Copy-Item build3ds/hw_o3ds_log.txt (Join-Path $out "${build}_$($r.n)_pull.txt") -Force
            python 3ds/tools/phase_report.py build3ds/hw_o3ds_log.txt | Set-Content (Join-Path $out "${build}_$($r.n)_phase.txt")
        } else { Write-Host "no pulled log for run $($r.n)" }
    }
}

# the baseline file: the numbers the plan asks for, one block per run
function Pick($file, $pattern, $last) {
    if (-not (Test-Path $file)) { return @('  (no log)') }
    $m = Select-String -Path $file -Pattern $pattern | ForEach-Object { '  ' + $_.Line.Trim() }
    if (-not $m) { return @('  (no line)') }
    if ($last) { return @($m | Select-Object -Last $last) }
    return @($m | Select-Object -First 3)
}
$base = Join-Path $out "baseline_$build.txt"
$lines = @("PERF baseline, build $build, O3DS $Ip, $(Get-Date -Format 'yyyy-MM-dd HH:mm')")
foreach ($r in $runs) {
    $f = Join-Path $out "${build}_$($r.n).txt"
    $lines += ''
    $lines += "== run $($r.n): $($r.sw)"
    switch ($r.n) {
        'A' { $lines += Pick $f 'mem commit limit|mem split H03|APT_SetAppCpuTimeLimit' 0 }
        { 'B','C','D','F' -contains $_ } {
            $lines += Pick $f 'audio cost avg' 3
            $lines += Pick $f 'audio posts' 2
            $lines += Pick $f 'pace: wall' 5
            if ($r.n -eq 'C') { $lines += Pick $f 'linear used .* peak|texture failures' 2 }
        }
        default {
            $p = Join-Path $out "${build}_$($r.n)_phase.txt"
            if (Test-Path $p) { $lines += (Get-Content $p | ForEach-Object { '  ' + $_ }) } else { $lines += '  (no phase report)' }
            if ($r.n -eq 'G') {
                $g = Join-Path $out "${build}_G_pull.txt"
                $lines += Pick $g 'b7 f' 6
                $lines += Pick $g 'vram free' 4
            }
        }
    }
}
$lines | Set-Content $base
Write-Host "=== baseline written: $base ==="
Write-Host "Send back: $base and the folder $out"
