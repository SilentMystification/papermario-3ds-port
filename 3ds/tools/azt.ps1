<#
azt.ps1 - run one named test scenario in Azahar through run_azahar.ps1, with fail-fast and a subject check.

  powershell -File 3ds/tools/azt.ps1 <scenario> [-Row <PM_ROW>] [-Build <lane or build3ds/lane-<name>/*.3dsx>] [-Try]
  powershell -File 3ds/tools/azt.ps1 -List
  powershell -File 3ds/tools/azt.ps1 <scenario> -CheckLog <file>     (checks an existing log, starts nothing)

Scenarios: 3ds/tools/azt_scenarios.json. Catalog: docs/handoff/test_catalog.md.
Steps:
 1. An unknown scenario, or one with status unverified, is refused (exit 4). -Try runs an unverified scenario anyway.
 2. PM_ROW is set from -Row (default azt). PM_LANE is set from -Build. Without -Build the default build3ds\pm_3ds.3dsx runs.
 3. run_azahar.ps1 starts with the stored -Debug, -Until, -Timeout (the stored cap, default 90 s) and -Force.
    -Force makes every call a real Azahar start. A cached result never hides a run.
 4. While the run goes, this script reads the game log (%APPDATA%\Azahar\sdmc\3ds\PaperMario\log.txt).
    The clock starts when the Azahar process of this run appears.
    - first_marker not in the log within deadline seconds: the Azahar of this run is killed by PID.
      run_azahar.ps1 then ends its own run and writes its budget line. Exit 2, "SCENARIO FAILED: marker missing".
    - The scenario expects warp or tour and the log has more than 2 title or demo mode lines (game mode 2, 3 or 17):
      the run is stopped. Exit 3, "ATTRACT LOOP".
 5. After the run, each expected_events regex is checked against the run log. A [CRASH] line fails the scenario.
    The shot files of this run are listed with the subject. The script never prints a full PASS. The shot must be read by a person.
Exit codes: 0 log checks pass, 1 expected event, crash or run failure, 2 marker missing, 3 attract loop,
4 unknown or unverified scenario, 5 argument or build error, 9 test budget of the row used up.
#>
param(
    [Parameter(Position = 0)][string]$Scenario = "",
    [string]$Row = "azt",
    [string]$Build = "",
    [string]$CheckLog = "",
    [switch]$Try,
    [switch]$List
)
$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$scs = @((Get-Content (Join-Path $PSScriptRoot "azt_scenarios.json") -Raw | ConvertFrom-Json).scenarios)
$attractRe = 'game mode (2|3|17) frame' # GAME_MODE_TITLE_SCREEN, GAME_MODE_ENTER_DEMO_WORLD, GAME_MODE_DEMO (game_modes.h)

if ($List -or -not $Scenario) {
    foreach ($s in $scs) { Write-Output ("{0,-14} {1,-16} {2}" -f $s.name, $s.status, $s.subject) }
    if (-not $Scenario) { Write-Output "usage: azt.ps1 <scenario> [-Row <id>] [-Build <lane>] [-Try] [-CheckLog <file>]" }
    exit 0
}
$sc = $scs | Where-Object { $_.name -eq $Scenario } | Select-Object -First 1
if (-not $sc) { Write-Output "SCENARIO UNKNOWN: $Scenario. See -List."; exit 4 }
if ($sc.status -eq "unverified" -and -not $Try) { Write-Output "SCENARIO UNVERIFIED: $($sc.name). Reason: $($sc.reason) Use -Try to run it anyway."; exit 4 }
if (@($sc.expected_events).Count -eq 0 -or -not $sc.first_marker) { Write-Output "SCENARIO INVALID: $($sc.name) has no expected_events or no first_marker."; exit 5 }
$expectsWarp = ($sc.expects -eq "warp" -or $sc.expects -eq "tour")
$cap = if ($sc.cap) { [int]$sc.cap } else { 90 }
$deadline = if ($sc.deadline) { [int]$sc.deadline } else { 20 }

# Lane and build
$lane = ""
if ($Build) {
    if ($Build -match '(?:^|[\\/])lane-([A-Za-z0-9_]+)[\\/][^\\/]*\.3dsx$') { $lane = $Matches[1] }
    elseif ($Build -match '^[A-Za-z0-9_]+$') { $lane = $Build }
    else { Write-Output "BUILD: '$Build' is not a lane name or build3ds/lane-<name>/*.3dsx. Nothing was run."; exit 5 }
}
$outDir = if ($lane) { Join-Path $repo "build3ds\lane-$lane" } else { Join-Path $repo "build3ds" }
$lastLog = Join-Path $outDir "last_log.txt"
$shotDir = Join-Path $outDir "shots"
$sd = Join-Path $env:APPDATA "Azahar\sdmc\3ds\PaperMario"
$logPath = Join-Path $sd "log.txt"
$pidFile = Join-Path $env:TEMP "pm_azahar_run.pid"
$runDir = Join-Path $repo "build3ds\azt"

# Shared state for the log reader
$script:pos = 0L
$script:carry = ""
$script:checkRc = 0

function Read-LogChunk {
    # New complete lines of the game log since the last call (Azahar keeps the file open, so share it)
    if (-not (Test-Path $logPath)) { return "" }
    try {
        $fs = [IO.File]::Open($logPath, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::ReadWrite)
        try {
            if ($fs.Length -lt $script:pos) { $script:pos = 0; $script:carry = "" }
            $n = [int]($fs.Length - $script:pos)
            if ($n -le 0) { return "" }
            $fs.Seek($script:pos, [IO.SeekOrigin]::Begin) | Out-Null
            $buf = New-Object byte[] $n
            $got = $fs.Read($buf, 0, $n)
            $script:pos += $got
        } finally { $fs.Close() }
        $text = $script:carry + [Text.Encoding]::UTF8.GetString($buf, 0, $got)
        $k = $text.LastIndexOf("`n")
        if ($k -lt 0) { $script:carry = $text; return "" }
        $script:carry = $text.Substring($k + 1)
        return $text.Substring(0, $k + 1)
    } catch { return "" }
}

function Get-AzaharProc([datetime]$since) {
    # The Azahar PID that run_azahar.ps1 wrote, only when that process started after $since (never another user's Azahar)
    try {
        $id = [int]((Get-Content $pidFile -Raw).Trim())
        $pr = Get-Process -Id $id -ErrorAction Stop
        if ($pr.ProcessName -eq "azahar" -and $pr.StartTime -ge $since) { return $pr }
    } catch {}
    return $null
}

function Show-Checks([string]$file, [string]$shots, [datetime]$since) {
    # Sets $script:checkRc: 0 log checks pass, 1 failed, 3 attract loop
    $script:checkRc = 0
    if (-not (Test-Path $file)) { Write-Output "NO LOG: $file is missing. Nothing to check."; $script:checkRc = 1; return }
    $text = Get-Content $file -Raw
    $lines = $text -split "`n"
    $attract = ([regex]::Matches($text, $attractRe)).Count
    if ($expectsWarp -and $attract -gt 2) { Write-Output "ATTRACT LOOP: $attract title or demo mode lines (limit 2). The scenario expects $($sc.expects)."; $script:checkRc = 3; return }
    if ($text -match '\[CRASH\]') { Write-Output "FAIL  crash: a [CRASH] line is in the log."; $script:checkRc = 1 }
    $fails = 0
    foreach ($ev in @($sc.expected_events)) {
        $hit = $lines | Where-Object { $_ -match $ev.regex } | Select-Object -First 1
        if ($hit) { Write-Output ("PASS  {0}: {1}" -f $ev.name, $hit.Trim()) }
        else { Write-Output ("FAIL  {0}: no line matches /{1}/" -f $ev.name, $ev.regex); $fails++ }
    }
    $shotFiles = @()
    if (Test-Path $shots) { $shotFiles = @(Get-ChildItem $shots -File | Where-Object { $_.LastWriteTime -ge $since } | Sort-Object Name) }
    Write-Output "SUBJECT: $($sc.subject)"
    Write-Output "SHOTS: $($shotFiles.Count) from this run"
    foreach ($f in $shotFiles) { Write-Output "  $($f.FullName)" }
    if ($sc.needs_picture -and $shotFiles.Count -eq 0) { Write-Output "FAIL  picture: no shot file from this run"; $fails++ }
    if ($fails) { Write-Output "SCENARIO FAILED: $fails check(s) failed."; $script:checkRc = 1; return }
    if ($script:checkRc -eq 0) { Write-Output "LOG CHECKS PASS ($(@($sc.expected_events).Count) of $(@($sc.expected_events).Count)). The script does not read the picture. A person must open the shot and confirm the subject." }
}

if ($CheckLog) {
    Show-Checks $CheckLog $shotDir ([datetime]::MinValue)
    exit $script:checkRc
}

# Start the run
$argv = @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File", (Join-Path $repo "3ds\run_azahar.ps1"),
          "-Until", [string]$sc.until, "-Timeout", [string]$cap, "-Force", "-Debug", [string]$sc.debug)
if ($sc.extra) { $argv += @($sc.extra) }
foreach ($a in $argv) { if ($a -match '"') { Write-Output "ARG: a value has a double quote, not allowed: $a"; exit 5 } }
if (-not (Test-Path (Join-Path $outDir "pm_3ds.3dsx"))) { Write-Output "BUILD: $outDir\pm_3ds.3dsx is missing. Build first. Nothing was run."; exit 5 }
$cmd = ($argv | ForEach-Object { if ($_ -match '^-[A-Za-z]+$') { $_ } else { '"' + $_ + '"' } }) -join " "

$env:PM_ROW = $Row
if ($lane) { $env:PM_LANE = $lane } else { Remove-Item Env:PM_LANE -ErrorAction SilentlyContinue }
if (Test-Path $lastLog) { Remove-Item $lastLog -Force }
$t0 = Get-Date
# No output redirect: with a redirect the child exit code is lost in this shell. The result is read from the budget file.
$child = Start-Process -FilePath (Get-Process -Id $PID).Path -ArgumentList $cmd -PassThru -WindowStyle Hidden
Write-Output "AZT: scenario $($sc.name), row $Row, lane '$lane', cap $cap s, deadline $deadline s after the Azahar start"

$azStart = $null
$markerSeen = $false
$stop = ""
$attract = 0
$capGuard = $cap + 120
while (-not $child.HasExited) {
    Start-Sleep -Milliseconds 500
    if (-not $azStart) {
        if (Get-AzaharProc $t0) { $azStart = Get-Date; Write-Output "AZT: Azahar started, the deadline clock runs" }
        continue
    }
    $chunk = Read-LogChunk
    if ($chunk) {
        if (-not $markerSeen -and $chunk -match $sc.first_marker) { $markerSeen = $true; Write-Output ("AZT: first marker seen at {0:N1} s" -f ((Get-Date) - $azStart).TotalSeconds) }
        $attract += ([regex]::Matches($chunk, $attractRe)).Count
    }
    $el = ((Get-Date) - $azStart).TotalSeconds
    if (-not $markerSeen -and $el -gt $deadline) { $stop = "marker"; break }
    if ($expectsWarp -and $attract -gt 2) { $stop = "attract"; break }
    if ($el -gt $capGuard) { $stop = "guard"; break }
}
if ($stop) {
    Write-Output "AZT: stopping the run ($stop). Killing the Azahar of this run."
    $pr = Get-AzaharProc $t0
    if ($pr) { Stop-Process -Id $pr.Id -Force -ErrorAction SilentlyContinue }
}
if (-not $child.WaitForExit(150000)) {
    Stop-Process -Id $child.Id -Force -ErrorAction SilentlyContinue
    Write-Output "NOT ENDED: run_azahar.ps1 (PID $($child.Id)) did not end. It was stopped by force. Check for an Azahar with Get-Process azahar."
}
$rc = $child.ExitCode
if ($rc -eq 9) { Write-Output "BUDGET: row $Row has no starts left. Nothing more was run. The orchestrator resets a row with run_azahar.ps1 -ResetBudget <row>."; exit 9 }
# The end line of this start in the budget file: rc and the result text of run_azahar.ps1
$endRow = $null
$bf = Join-Path $repo "3ds\logs\test_budget.csv"
if (Test-Path $bf) {
    $rowsB = @(Get-Content $bf | Where-Object { $_ })
    for ($i = $rowsB.Count - 1; $i -ge 0 -and -not $endRow; $i--) {
        $f = $rowsB[$i].Split(",")
        if ($f.Count -ge 6 -and $f[1] -eq $Row -and $f[4] -eq "end") {
            if ([datetime]$f[0] -ge $t0.AddSeconds(-5)) { $endRow = $f }
            break
        }
    }
}
if (-not $endRow) { Write-Output "NO END LINE: run_azahar.ps1 wrote no end line for row $Row after this start. It did not run (lock timeout, switch error or budget). Nothing was checked."; exit 1 }
$rc = [int]$endRow[5]
Write-Output ("run: rc {0}: {1}" -f $rc, (($endRow[6..($endRow.Count - 1)]) -join ","))
if ($stop -eq "marker") {
    Write-Output "SCENARIO FAILED: marker missing: $($sc.first_marker)"
    Write-Output "Not in the log $deadline s after the Azahar start. Azahar was stopped. Log: $logPath"
    exit 2
}
if ($stop -eq "attract") {
    Write-Output "ATTRACT LOOP: $attract title or demo mode lines (limit 2) in a run that expects $($sc.expects). Stopped."
    exit 3
}
if ($stop -eq "guard") { Write-Output "SCENARIO FAILED: the run did not end within $capGuard s."; exit 1 }
if ($rc -eq 2) { Write-Output "SCENARIO FAILED: run_azahar reported crash symbols (rc 2). See $runOut"; exit 1 }
if ($rc -ne 0) { Write-Output "SCENARIO FAILED: run_azahar rc $rc. See $runOut"; exit 1 }

Show-Checks $lastLog $shotDir $t0
exit $script:checkRc
