# push_batch.ps1: one batch console run. Push, wait, pull the log, fxcen report, save all output.
# Usage (repo root): powershell -File 3ds/tools/push_batch.ps1 -Ip <ip> -Label <label> -Seconds <s> -Args "<game args>" [-Stored <file>] [-Dry] [-FromLog <file>]
# Steps:
#   1 push: Git Bash "sh 3ds/push_3ds.sh --ip <ip> -t <s> <game args>". Exit code not 0: stop with the same code.
#   2 wait: until <s> seconds after the push returned (1 s steps, a line every 10 s).
#   3 pull: python 3ds/tools/pm_logrecv.py --ip <ip> --label <label> --timeout 60 --out build3ds. Exit not 0: stop.
#   4 report: python 3ds/tools/fxcen_report.py build3ds/hw_<label>_log.txt --label <label>.
#     -Stored <file>: compare the rows of "table sorted by us" of the new report and the stored report per idx.
#   5 save: all output to build3ds/out/pushbatch_<label>_<yyyyMMdd_HHmm>.txt; the path is printed last.
# -FromLog <file>: skip steps 1-3, run 4-5 on that log (replay of a stored run).
# -Dry: print the 4 commands and stop (nothing runs, nothing is saved).
param(
    [string]$Ip = "",
    [string]$Label = "azahar",
    [int]$Seconds = 0,
    [Alias("Args")][string]$GameArgs = "",
    [string]$Stored = "",
    [switch]$Dry,
    [string]$FromLog = ""
)

$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
Set-Location -LiteralPath $root
$stamp = Get-Date -Format "yyyyMMdd_HHmm"
$outDir = Join-Path $root "build3ds\out"
$outPath = Join-Path $outDir ("pushbatch_{0}_{1}.txt" -f $Label, $stamp)
$inv = [System.Globalization.CultureInfo]::InvariantCulture
$buf = New-Object System.Collections.Generic.List[string]
$script:rc = 0

function Say([string]$s) {
    $script:buf.Add($s)
    Write-Output $s
}

function Find-Bash {
    $g = Get-Command git -ErrorAction SilentlyContinue
    if ($g) {
        $dir = Split-Path -Parent $g.Source
        for ($i = 0; $i -lt 3; $i++) {
            $cand = Join-Path $dir "bin\bash.exe"
            if (Test-Path -LiteralPath $cand) { return $cand }
            $dir = Split-Path -Parent $dir
        }
    }
    return "C:\Program Files\Git\bin\bash.exe"
}

function Get-Table([string]$path) {
    $rows = @{}
    $in = $false
    $hdr = $false
    foreach ($l in Get-Content -LiteralPath $path) {
        if ($l -eq "table sorted by us") { $in = $true; $hdr = $true; continue }
        if (-not $in) { continue }
        if ($hdr) { $hdr = $false; continue }
        if ($l.Trim() -eq "") { break }
        $f = $l.Trim() -split "\s+"
        $rows[[int]$f[0]] = @{ name = $f[1]; us = [double]$f[4]; walk = [double]$f[-1] }
    }
    return $rows
}

function F1($v) { return ([double]$v).ToString("F1", $inv) }

function Compare-Stored([string]$newPath, [string]$storedPath) {
    Say ("compare: new {0} vs stored {1}" -f $newPath, $storedPath)
    $a = Get-Table $newPath
    $b = Get-Table $storedPath
    $ids = @(@($a.Keys) + @($b.Keys)) | Sort-Object -Unique
    Say ("{0,5} {1,-24} {2,10} {3,10} {4,8} {5,10} {6,10} {7,8}" -f "idx", "name", "us new", "us stored", "us diff", "walk new", "walk stored", "walk diff")
    foreach ($id in $ids) {
        if (-not $a.ContainsKey($id)) { Say ("idx {0} missing in new" -f $id); continue }
        if (-not $b.ContainsKey($id)) { Say ("idx {0} ({1}) missing in stored" -f $id, $a[$id].name); continue }
        $us = $a[$id].us - $b[$id].us
        $wk = $a[$id].walk - $b[$id].walk
        Say ("{0,5} {1,-24} {2,10} {3,10} {4,8} {5,10} {6,10} {7,8}" -f $id, $a[$id].name,
            (F1 $a[$id].us), (F1 $b[$id].us), (F1 $us), (F1 $a[$id].walk), (F1 $b[$id].walk), (F1 $wk))
    }
}

function Run-Batch {
    $bash = Find-Bash
    if ($FromLog -ne "") { $rep = $FromLog } else { $rep = "build3ds/hw_${Label}_log.txt" }
    $newRep = Join-Path $outDir ("fxcen_{0}.txt" -f $Label)

    if ($FromLog -eq "") {
        if ($Ip -eq "") { Say "push_batch: -Ip is required without -FromLog"; $script:rc = 2; return }
        $cmd1 = "sh 3ds/push_3ds.sh --ip $Ip -t $Seconds $GameArgs"
        Say "step 1 push: $bash -c `"$cmd1`""
        & $bash -c $cmd1 2>&1 | ForEach-Object { Say ([string]$_) }
        $code = $LASTEXITCODE
        if ($code -ne 0) { Say "step 1 push: exit $code, stop"; $script:rc = $code; return }
        $pushEnd = Get-Date

        Say "step 2 wait: $Seconds s after the push returned"
        $deadline = $pushEnd.AddSeconds($Seconds)
        $n = 0
        while ((Get-Date) -lt $deadline) {
            Start-Sleep -Seconds 1
            $n++
            if ($n % 10 -eq 0) { Say ("step 2 wait: {0} s of {1}" -f $n, $Seconds) }
        }
        Say "step 2 wait: done"

        Say "step 3 pull: python 3ds/tools/pm_logrecv.py --ip $Ip --label $Label --timeout 60 --out build3ds"
        & python 3ds/tools/pm_logrecv.py --ip $Ip --label $Label --timeout 60 --out build3ds 2>&1 | ForEach-Object { Say ([string]$_) }
        $code = $LASTEXITCODE
        if ($code -ne 0) { Say "step 3 pull: exit $code, stop"; $script:rc = $code; return }
    } else {
        Say "steps 1-3 skipped (-FromLog)"
    }

    Say "step 4 report: python 3ds/tools/fxcen_report.py $rep --label $Label"
    & python 3ds/tools/fxcen_report.py $rep --label $Label 2>&1 | ForEach-Object { Say ([string]$_) }
    $code = $LASTEXITCODE
    if ($code -ne 0) { Say "step 4 report: exit $code, stop"; $script:rc = $code; return }

    if ($Stored -ne "") {
        Compare-Stored $newRep $Stored
    }
}

if ($Dry) {
    if ($FromLog -eq "") { $rep = "build3ds/hw_${Label}_log.txt" } else { $rep = $FromLog }
    Write-Output "1 push: sh 3ds/push_3ds.sh --ip $Ip -t $Seconds $GameArgs"
    Write-Output "2 wait: $Seconds s after the push returned"
    Write-Output "3 pull: python 3ds/tools/pm_logrecv.py --ip $Ip --label $Label --timeout 60 --out build3ds"
    Write-Output "4 report: python 3ds/tools/fxcen_report.py $rep --label $Label"
    if ($FromLog -ne "") { Write-Output "(-FromLog: steps 1-3 skipped)" }
    exit 0
}

New-Item -ItemType Directory -Force -Path $outDir | Out-Null
Run-Batch
[System.IO.File]::WriteAllLines($outPath, $buf.ToArray(), (New-Object System.Text.UTF8Encoding($false)))
Write-Output ("saved {0} lines" -f $buf.Count)
Write-Output $outPath
exit $script:rc
