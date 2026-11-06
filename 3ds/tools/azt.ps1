<#
azt.ps1 - run one named test scenario in Azahar through run_azahar.ps1, with fail-fast and a subject check.

  powershell -File 3ds/tools/azt.ps1 <scenario> [-Row <PM_ROW>] [-Build <lane or build3ds/lane-<name>/*.3dsx>] [-Try] [-Speed turbo|normal] [-NoKeep]
  (default: the Azahar of the run stays open in its slot after a matched until line, the next run on the slot warm-restarts in it; -NoKeep closes it)
  powershell -File 3ds/tools/azt.ps1 a,b,c [-Row ...] [-Build ...] [-Try]    (AZT-3 list: one child start per scenario, one kept slot, warm restart between them; prints LIST wall times)
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
    An event with "max" is a numeric gate: capture group 1 of its regex must be <= max. "min" (optional) must be >= min. "pick": "last" takes the last matching line.
    The shot files of this run are listed with the subject. The script never prints a full PASS. The shot must be read by a person.
Exit codes: 0 log checks pass, 1 expected event, crash or run failure, 2 marker missing, 3 attract loop,
4 unknown or unverified scenario, 5 argument or build error, 6 no new log line for 40 s (stalled), 7 until static check failed (nothing run), 9 test budget of the row used up.
-SelfTest runs the static check on bad scenario copies and the stall rule (no start).
#>
param(
    [Parameter(Position = 0)][string]$Scenario = "",
    [string]$Row = "azt",
    [string]$Build = "",
    [string]$CheckLog = "",
    [switch]$Try,
    [switch]$NoKeep, # AZT-3b: default is to leave the slot's Azahar open after a matched run (paused; a later run on the slot restarts in it, warm; azreap closes it after 10 min idle). -NoKeep closes it after the run.
    [ValidateSet("", "turbo", "normal")][string]$Speed = "", # BENCH3: speed of this start (default: -Turbo in the scenario, else normal)
    [switch]$List,
    [switch]$SelfTest
)
$ErrorActionPreference = "Stop"
. "$PSScriptRoot\azslot.ps1"
$repo = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$scs = @((Get-Content (Join-Path $PSScriptRoot "azt_scenarios.json") -Raw | ConvertFrom-Json).scenarios)
$attractRe = 'game mode (2|3|17) frame' # GAME_MODE_TITLE_SCREEN, GAME_MODE_ENTER_DEMO_WORLD, GAME_MODE_DEMO (game_modes.h)
# UNTILCHK: static check of a scenario's until line (no start). Stall rule: a run with no new log line for $stallSec stops (exit 6).
$stallSec = 40
function Test-StallExpired([datetime]$lastNew, [datetime]$now, [int]$limit) { return (($now - $lastNew).TotalSeconds -gt $limit) }
$script:srcText = $null
function Get-SourceText {
    # All C source that can print a log line: the 3DS port, its headers and the decomp source.
    # AZT-3c: cached in build3ds\azt\srctext.cache. The key is every source file's path, size and write time (about 0.3 s to list,
    # a full read is about 2.5 s). A changed, added or removed file changes the key and rebuilds the cache.
    if ($null -ne $script:srcText) { return $script:srcText }
    $key = New-Object System.Text.StringBuilder
    $paths = New-Object System.Collections.Generic.List[string]
    foreach ($d in @("3ds\src", "3ds\include", "src")) {
        $p = Join-Path $repo $d
        if (-not [IO.Directory]::Exists($p)) { continue }
        foreach ($fi in ([IO.DirectoryInfo]$p).EnumerateFiles("*", [IO.SearchOption]::AllDirectories)) {
            if ($fi.Extension -eq ".c" -or $fi.Extension -eq ".h") {
                [void]$key.Append($fi.FullName).Append("|").Append($fi.Length).Append("|").Append($fi.LastWriteTimeUtc.Ticks).Append("`n")
                $paths.Add($fi.FullName)
            }
        }
    }
    $sha = [Security.Cryptography.SHA256]::Create()
    $stamp = [BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($key.ToString()))).Replace("-", "")
    $cacheDir = Join-Path $repo "build3ds\azt"
    $cache = Join-Path $cacheDir "srctext.cache"
    try {
        if (Test-Path $cache) {
            $c = [IO.File]::ReadAllText($cache)
            if ($c.StartsWith($stamp + "`n")) { $script:srcText = $c.Substring($stamp.Length + 1); return $script:srcText }
        }
    } catch {}
    $sb = New-Object System.Text.StringBuilder
    foreach ($f in $paths) { [void]$sb.Append([IO.File]::ReadAllText($f)) }
    $script:srcText = $sb.ToString()
    try {
        # Written to a temp file and moved in place: a concurrent reader sees the old cache or the new one, never a part
        New-Item -ItemType Directory -Force -Path $cacheDir | Out-Null
        $tmp = "$cache.$PID.tmp"
        [IO.File]::WriteAllText($tmp, $stamp + "`n" + $script:srcText)
        Move-Item $tmp $cache -Force -ErrorAction Stop
    } catch { Remove-Item "$cache.$PID.tmp" -ErrorAction SilentlyContinue }
    return $script:srcText
}
function Get-UntilProblem($s) {
    # Returns "" when the until line can be printed by this build and its frame is reachable, else the reason.
    $u = [string]$s.until
    $dbg = [string]$s.debug
    $capS = if ($s.cap) { [int]$s.cap } else { 90 }
    try { [void][regex]::new($u) } catch { return "until is not a valid regex: /$u/" }
    # Literal runs of the until line: split at regex syntax, classes, groups and digits. Each run of 4 or more letters must occur in the source.
    $parts = [regex]::Split($u, '\\[dDwWsSbB]|\\.|\[[^\]]*\]|\([^)]*\)|[\+\*\?\|\^\$\{\}\.\-0-9]+')
    $src = Get-SourceText
    foreach ($r in $parts) {
        $t = $r.Trim()
        if ($t.Length -ge 4 -and $t -match '[A-Za-z]' -and -not $src.Contains($t)) { return "no source line prints '$t' (until /$u/)" }
    }
    $fm = [regex]::Match($u, 'frame (\d+)\\b')
    $sm = [regex]::Match($dbg, 'shotwin=(\d+):(\d+)')
    if ($fm.Success -and $sm.Success) {
        $f = [int]$fm.Groups[1].Value; $a = [int]$sm.Groups[1].Value; $b = [int]$sm.Groups[2].Value
        if ($f -lt $a -or $f -gt ($a + $b - 1)) { return "until frame $f is outside shotwin $a to $($a + $b - 1)" }
    }
    foreach ($m in [regex]::Matches($u, '\bf(\d+)\b')) {
        $n = [int64]$m.Groups[1].Value
        if ($n -gt ($capS * 60)) { return "until frame f$n is past the run length: cap $capS s at 60 steps per s" }
    }
    return ""
}

if ($SelfTest) {
    # Self-test: bad scenario copies are written to a temp file and loaded like the real file. No Azahar start.
    $fails = 0
    $tmp = Join-Path $env:TEMP "azt_selftest_scenarios.json"
    $allObj = Get-Content (Join-Path $PSScriptRoot "azt_scenarios.json") -Raw | ConvertFrom-Json
    $base = @($allObj.scenarios | Where-Object { $_.name -eq "toad_pond" })[0]
    $cases = @(
        @{ n = "good until (frame 196 in shotwin 160:40)"; u = 'shot \d+ frame 196\b'; ok = $true },
        @{ n = "bad frame (300 outside shotwin 160:40)"; u = 'shot \d+ frame 300\b'; ok = $false },
        @{ n = "bad literal (no source prints it)"; u = 'zzqx nothing prints this \d+'; ok = $false },
        @{ n = "bad frame past the run length"; u = 'warp: scene f99999'; ok = $false }
    )
    foreach ($c in $cases) {
        $sc2 = @([pscustomobject]@{ name = "selftest"; until = $c.u; debug = $base.debug; cap = $base.cap }) # the copy's fields only
        $sc2 | ConvertTo-Json -Depth 5 | Set-Content $tmp
        $cp = (Get-Content $tmp -Raw | ConvertFrom-Json)[0]
        $p = Get-UntilProblem $cp
        $good = (($p -eq "") -eq $c.ok)
        if (-not $good) { $fails++ }
        Write-Output ("{0} {1}: {2}" -f $(if ($good) { "PASS" } else { "FAIL" }), $c.n, $(if ($p) { $p } else { "accepted" }))
    }
    foreach ($s in $scs) {
        $p = Get-UntilProblem $s
        if ($p) { Write-Output ("NOTE  current scenario {0}: {1}" -f $s.name, $p) }
    }
    $now = Get-Date
    $stallOk = ((Test-StallExpired ($now.AddSeconds(-41)) $now $stallSec) -and -not (Test-StallExpired ($now.AddSeconds(-39)) $now $stallSec))
    if (-not $stallOk) { $fails++ }
    Write-Output ("{0} stall rule: 41 s without a line stops, 39 s does not" -f $(if ($stallOk) { "PASS" } else { "FAIL" }))
    Remove-Item $tmp -ErrorAction SilentlyContinue
    if ($fails) { Write-Output "SELFTEST FAILED: $fails case(s)"; exit 1 }
    Write-Output "SELFTEST PASS"
    exit 0
}
if ($List -or -not $Scenario) {
    foreach ($s in $scs) { Write-Output ("{0,-14} {1,-16} {2}" -f $s.name, $s.status, $s.subject) }
    if (-not $Scenario) { Write-Output "usage: azt.ps1 <scenario> [-Row <id>] [-Build <lane>] [-Try] [-Speed turbo|normal] [-CheckLog <file>]" }
    exit 0
}
if ($Scenario -match ",") {
    # AZT-3: a list "a,b,c" runs its scenarios one after the other, one child azt.ps1 start each. The Azahar stays open (default),
    # so the next child restarts emulation in the same slot (warm restart, about 2 s instead of a cold start of 12 to 16 s).
    $names = @($Scenario.Split(",") | ForEach-Object { $_.Trim() } | Where-Object { $_ })
    $refused = @()
    foreach ($n in $names) {
        $one = @($scs | Where-Object { $_.name -eq $n })[0]
        if (-not $one) { $refused += "unknown ${n}"; continue }
        if ($one.status -eq "unverified" -and -not $Try) { $refused += "unverified ${n} (use -Try)"; continue }
        $up = Get-UntilProblem $one
        if ($up) { $refused += "rejected ${n}: $up" }
    }
    if ($refused.Count) { Write-Output ("LIST REFUSED: {0}. Nothing was run." -f ($refused -join "; ")); exit 4 }
    $listT0 = Get-Date
    $childExe = (Get-Process -Id $PID).Path
    $summary = @()
    $worst = 0
    Write-Output ("LIST: {0} scenarios, one kept slot, warm restart between them: {1}" -f $names.Count, ($names -join ", "))
    foreach ($n in $names) {
        $ca = @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File", $PSCommandPath, $n, "-Row", $Row)
        if ($Build) { $ca += @("-Build", $Build) }
        if ($Try) { $ca += "-Try" }
        if ($Speed) { $ca += @("-Speed", $Speed) }
        $t1 = Get-Date
        Write-Output ("LIST: start {0} at {1}" -f $n, $t1.ToString("HH:mm:ss"))
        & $childExe @ca
        $rc = $LASTEXITCODE
        $sec = ((Get-Date) - $t1).TotalSeconds
        Write-Output ("LIST: {0} rc {1}, wall {2:N1} s" -f $n, $rc, $sec)
        $summary += ("{0} rc {1} {2:N1} s" -f $n, $rc, $sec)
        if ($rc -gt $worst) { $worst = $rc }
        if ($rc -eq 9) { Write-Output "LIST: row $Row has no starts left. The rest of the list did not run."; break }
    }
    Write-Output ("LIST DONE: wall {0:N1} s. {1}" -f ((Get-Date) - $listT0).TotalSeconds, ($summary -join "; "))
    exit $worst
}
$sc = $scs | Where-Object { $_.name -eq $Scenario } | Select-Object -First 1
if (-not $sc) { Write-Output "SCENARIO UNKNOWN: $Scenario. See -List."; exit 4 }
if ($sc.status -eq "unverified" -and -not $Try) { Write-Output "SCENARIO UNVERIFIED: $($sc.name). Reason: $($sc.reason) Use -Try to run it anyway."; exit 4 }
$uerr = Get-UntilProblem $sc
if ($uerr) { Write-Output "SCENARIO REJECTED: $($sc.name): $uerr. Nothing was run."; exit 7 }
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
# AZT-1: the slot is taken below, just before the start; these paths then point into the slot (slot<k> folder of the lane)
$shotDir = Join-Path $outDir "shots"
$sd = ""; $logPath = ""; $pidFile = ""; $slotK = 0
$runDir = Join-Path $repo "build3ds\azt"

# Shared state for the log reader
$script:pos = 0L
$script:carry = ""
$script:logCT = $null
$script:checkRc = 0

function Read-LogChunk {
    # New complete lines of the game log since the last call (Azahar keeps the file open, so share it)
    if (-not (Test-Path $logPath)) { return "" }
    try {
        $fs = [IO.File]::Open($logPath, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::ReadWrite)
        try {
            $ct = (Get-Item $logPath).CreationTimeUtc
            if ($null -ne $script:logCT -and $ct -ne $script:logCT) { $script:pos = 0; $script:carry = "" } # a new log.txt: the game booted again (warm restart)
            $script:logCT = $ct
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
        # AZFIX: only the Azahar that this run's run_azahar.ps1 started (its parent). A run that waits for the lock sees the other run's PID in the shared file.
        $par = (Get-CimInstance Win32_Process -Filter "ProcessId=$id" -ErrorAction Stop).ParentProcessId
        if ($pr.ProcessName -eq "azahar" -and $pr.StartTime -ge $since -and $par -eq $script:childPid) { return $pr }
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
        # pick "last" takes the last matching line (the window of the until-line); default "first".
        # max = numeric gate: group 1 of regex must be <= max.
        $hits = @($lines | Where-Object { $_ -match $ev.regex })
        $hit = if ($ev.pick -eq "last") { $hits | Select-Object -Last 1 } else { $hits | Select-Object -First 1 }
        if (-not $hit) { Write-Output ("FAIL  {0}: no line matches /{1}/" -f $ev.name, $ev.regex); $fails++; continue }
        if (($ev.PSObject.Properties.Name -contains "max") -or ($ev.PSObject.Properties.Name -contains "min")) {
            $m = [regex]::Match($hit, $ev.regex)
            if (-not $m.Success -or $m.Groups.Count -lt 2) { Write-Output ("FAIL  {0}: regex has no capture group 1 for the gate" -f $ev.name); $fails++; continue }
            $val = [double]$m.Groups[1].Value
            $hasMax = $ev.PSObject.Properties.Name -contains "max"
            $hasMin = $ev.PSObject.Properties.Name -contains "min"
            if ($hasMax -and $val -gt [double]$ev.max) { Write-Output ("FAIL  {0}: {1} is above the limit {2}: {3}" -f $ev.name, $val, $ev.max, $hit.Trim()); $fails++ }
            elseif ($hasMin -and $val -lt [double]$ev.min) { Write-Output ("FAIL  {0}: {1} is below the limit {2}: {3}" -f $ev.name, $val, $ev.min, $hit.Trim()); $fails++ }
            else { Write-Output ("PASS  {0}: {1} in range {2} to {3}: {4}" -f $ev.name, $val, $(if ($hasMin) { $ev.min } else { "-" }), $(if ($hasMax) { $ev.max } else { "-" }), $hit.Trim()) }
        }
        else { Write-Output ("PASS  {0}: {1}" -f $ev.name, $hit.Trim()) }
    }
    $shotFiles = @()
    if (Test-Path $shots) { $shotFiles = @(Get-ChildItem $shots -File | Where-Object { $_.LastWriteTime -ge $since } | Sort-Object Name) }
    if ($since -gt [datetime]::MinValue -and $shotFiles.Count -gt 0) {
        # Keep this run's shots per scenario: the shared shots folder is overwritten by the next run
        $arch = Join-Path $repo "build3ds\azt\shots\$($sc.name)"
        New-Item -ItemType Directory -Force -Path $arch | Out-Null
        Remove-Item (Join-Path $arch "*") -Force -ErrorAction SilentlyContinue
        foreach ($f in $shotFiles) { Copy-Item $f.FullName $arch }
        $shotFiles = @(Get-ChildItem $arch -File | Sort-Object Name)
    }
    if ($since -gt [datetime]::MinValue) {
        # The slot is free for the next run once this script ends, and that run overwrites the slot log: read this run's log from this copy
        $ld = Join-Path $repo "build3ds\azt\logs"
        New-Item -ItemType Directory -Force -Path $ld | Out-Null
        Copy-Item $file (Join-Path $ld "$($sc.name).txt") -Force
        Write-Output "LOG: $(Join-Path $ld "$($sc.name).txt") (copy of this run's log; slot $slotK)"
    }
    Write-Output "SUBJECT: $($sc.subject)"
    Write-Output "SHOTS: $($shotFiles.Count) from this run"
    foreach ($f in $shotFiles) { Write-Output "  $($f.FullName)" }
    if ($sc.needs_picture -and $shotFiles.Count -eq 0) { Write-Output "FAIL  picture: no shot file from this run"; $fails++ }
    if ($fails) { Write-Output "SCENARIO FAILED: $fails check(s) failed."; $script:checkRc = 1; return }
    if ($script:checkRc -eq 0) { Write-Output "LOG CHECKS PASS ($(@($sc.expected_events).Count) of $(@($sc.expected_events).Count)). The script does not read the picture. A person must open the shot and confirm the subject." }
}

function Exit-Run([int]$code) {
    # Records this scenario start in 3ds/logs/perf/runs.jsonl (bench.py ingest-last): under the bench of this scenario, else adhoc:<scenario>.
    $ia = @((Join-Path $repo "3ds\tools\bench.py"), "ingest-last", "--log", $lastLog, "--scenario", $sc.name, "--row", $Row,
            "--rc", [string]$code, "--shots", (Join-Path $repo "build3ds\azt\shots\$($sc.name)"))
    if ($env:PM_BENCH_LABEL) { $ia += @("--label", $env:PM_BENCH_LABEL) }
    if ($env:PM_BENCH_BUILD) { $ia += @("--build", $env:PM_BENCH_BUILD) }
    if ($env:PM_BENCH_COMMIT) { $ia += @("--commit", $env:PM_BENCH_COMMIT) }
    $ia += @("--speed", $speed)
    if ($null -ne $frameLimit) { $ia += @("--frame-limit", [string]$frameLimit) }
    & python @ia
    if ($LASTEXITCODE -ne 0) { Write-Output "BENCH: run not recorded (bench.py exit $LASTEXITCODE)" }
    if ($script:slotK) { Exit-Slot $script:slotK; $script:slotK = 0 } # the slot is free for the next run once the logs are read and recorded
    exit $code
}

if ($CheckLog) {
    Show-Checks $CheckLog $shotDir ([datetime]::MinValue)
    exit $script:checkRc
}

# Start the run
$argv = @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File", (Join-Path $repo "3ds\run_azahar.ps1"),
          "-Until", [string]$sc.until, "-Timeout", [string]$cap, "-Force", "-Debug", [string]$sc.debug)
if ($sc.extra) { $argv += @($sc.extra) }
# BENCH3: the speed of this start, recorded by Exit-Run. Turbo: frame_limit is the TurboLimit of the run (run_azahar default 1000)
$extraArgs = @($sc.extra)
$scTurbo = $extraArgs -contains "-Turbo"
if ($Speed -eq "normal" -and $scTurbo) { Write-Output "SPEED CONFLICT: scenario $($sc.name) has -Turbo in its extra args, -Speed normal asks for normal. Nothing was run."; exit 5 }
$speed = if ($Speed -eq "turbo" -or $scTurbo) { "turbo" } else { "normal" }
$frameLimit = $null
if ($speed -eq "turbo") {
    if (-not $scTurbo) { $argv += @("-Turbo") }
    $frameLimit = 1000
    $li = [array]::IndexOf($extraArgs, "-TurboLimit")
    if ($li -ge 0 -and $li + 1 -lt $extraArgs.Count) { $frameLimit = [double]$extraArgs[$li + 1] }
}
foreach ($a in $argv) { if ($a -match '"') { Write-Output "ARG: a value has a double quote, not allowed: $a"; exit 5 } }
if (-not (Test-Path (Join-Path $outDir "pm_3ds.3dsx"))) { Write-Output "BUILD: $outDir\pm_3ds.3dsx is missing. Build first. Nothing was run."; exit 5 }
# AZT-1: take a slot (waits only when all are busy). run_azahar.ps1 gets -Slot with PM_SLOT_HELD=1 and does not take it again.
# AZT2: the slot with a kept Azahar of this lane 3dsx and turbo setting first (warm start), see azslot.ps1
$azX3 = if ($lane) { "pm_3ds_$lane.3dsx" } else { "pm_3ds.3dsx" }
$azTurbo = ($speed -eq "turbo")
$azTL = if ($azTurbo) { [int]$frameLimit } else { 0 }
$slotK = Get-Slot 0 900 $azX3 $azTurbo $azTL
if (-not $slotK) { Write-Output "LOCK TIMEOUT: no free Azahar slot after 900 s. Nothing was run."; exit 3 }
$slotInfo = Ensure-Slot $slotK
$sd = $slotInfo.sd; $logPath = Join-Path $sd "log.txt"; $pidFile = $slotInfo.pidFile
$lastLog = Join-Path $outDir "slot$slotK\last_log.txt"
$shotDir = Join-Path $outDir "slot$slotK\shots"
$env:PM_SLOT_HELD = "1"
$argv += @("-Slot", [string]$slotK)
if (-not $NoKeep) { $argv += @("-Keep") } # AZT-3b: kept by default. run_azahar keeps only a run that matched its until line.
$warmFile = Join-Path $slotInfo.dir "warm.start"
Remove-Item $warmFile -ErrorAction SilentlyContinue
$cmd = ($argv | ForEach-Object { if ($_ -match '^-[A-Za-z]+$') { $_ } else { '"' + $_ + '"' } }) -join " "

$env:PM_ROW = $Row
$env:PM_AZT_ATTEMPT = if ($env:PM_AZT_RETRY_RUN) { "2" } else { "1" } # AZFIX: run_azahar.ps1 writes the end line as retry on attempt 1 only
if ($lane) { $env:PM_LANE = $lane } else { Remove-Item Env:PM_LANE -ErrorAction SilentlyContinue }
if (Test-Path $lastLog) { Remove-Item $lastLog -Force -ErrorAction SilentlyContinue } # AZFIX: two azt runs may race here
$t0 = Get-Date
# No output redirect: with a redirect the child exit code is lost in this shell. The result is read from the budget file.
$child = Start-Process -FilePath (Get-Process -Id $PID).Path -ArgumentList $cmd -PassThru -WindowStyle Hidden
$script:childPid = $child.Id
Write-Output "AZT: scenario $($sc.name), row $Row, lane '$lane', slot $slotK, cap $cap s, deadline $deadline s after the Azahar start"

$azStart = $null
$markerSeen = $false
$stop = ""
$attract = 0
$capGuard = $cap + 120
while (-not $child.HasExited) {
    Start-Sleep -Milliseconds 500
    if (-not $azStart) {
        if (Get-AzaharProc $t0) { $azStart = Get-Date; $lastNew = $azStart; Write-Output "AZT: Azahar started, the deadline clock runs" }
        elseif (Test-Path $warmFile) {
            # AZT-2: run_azahar.ps1 restarts emulation in the kept Azahar and wrote warm.start. The old log.txt content does not count.
            $azStart = Get-Date; $lastNew = $azStart; $script:warm = $true
            if (Test-Path $logPath) { $script:pos = (Get-Item $logPath).Length; $script:logCT = (Get-Item $logPath).CreationTimeUtc }
            Write-Output "AZT: warm restart in the kept Azahar, the deadline clock runs"
        }
        continue
    }
    $chunk = Read-LogChunk
    if ($chunk) {
        $lastNew = Get-Date
        if (-not $markerSeen -and $chunk -match $sc.first_marker) { $markerSeen = $true; Write-Output ("AZT: first marker seen at {0:N1} s" -f ((Get-Date) - $azStart).TotalSeconds) }
        $attract += ([regex]::Matches($chunk, $attractRe)).Count
    }
    $el = ((Get-Date) - $azStart).TotalSeconds
    if (-not $markerSeen -and $el -gt $deadline) { $stop = "marker"; break }
    if ($expectsWarp -and $attract -gt 2) { $stop = "attract"; break }
    if ($el -gt $capGuard) { $stop = "guard"; break }
    if (Test-StallExpired $lastNew (Get-Date) $stallSec) { $stop = "stall"; break }
}
if ($stop) {
    Write-Output "AZT: stopping the run ($stop). Killing the Azahar of this run."
    $pr = Get-AzaharProc $t0
    if (-not $pr) { $pr = Get-KeptAzahar $slotK }
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
        if ($f.Count -ge 7 -and $f[1] -eq $Row -and ($f[4] -eq "end" -or $f[4] -eq "retry") -and $f[6] -like "slot${slotK}:*") {
            if ([datetime]$f[0] -ge $t0.AddSeconds(-5)) { $endRow = $f }
            break
        }
    }
}
if (-not $endRow) { Write-Output "NO END LINE: run_azahar.ps1 wrote no end line for row $Row after this start. It did not run (lock timeout, switch error or budget). Nothing was checked."; exit 1 }
if ($endRow[4] -eq "retry" -and -not $env:PM_AZT_RETRY_RUN) {
    # AZFIX: Azahar exited with an empty emulator log. One retry, its start line is retry-start (not counted).
    Write-Output "RETRY: Azahar exited with an empty emulator log. Retrying once. The retry is not counted in the row budget."
    $env:PM_AZT_RETRY_RUN = "1"
    Exit-Slot $slotK; $slotK = 0
    $ra = @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File", $PSCommandPath, $Scenario, "-Row", $Row, "-Try")
    if ($Build) { $ra += @("-Build", $Build) }
    if ($NoKeep) { $ra += "-NoKeep" }
    & (Get-Process -Id $PID).Path @ra
    exit $LASTEXITCODE
}
$rc = [int]$endRow[5]
Write-Output ("run: rc {0}: {1}" -f $rc, (($endRow[6..($endRow.Count - 1)]) -join ","))
if ($stop -eq "marker") {
    Write-Output "SCENARIO FAILED: marker missing: $($sc.first_marker)"
    Write-Output "Not in the log $deadline s after the Azahar start. Azahar was stopped. Log: $logPath"
    Exit-Run 2
}
if ($stop -eq "attract") {
    Write-Output "ATTRACT LOOP: $attract title or demo mode lines (limit 2) in a run that expects $($sc.expects). Stopped."
    Exit-Run 3
}
if ($stop -eq "stall") { Write-Output "SCENARIO FAILED: no new log line for $stallSec s. The run was stalled and Azahar was stopped."; Exit-Run 6 }
if ($stop -eq "guard") { Write-Output "SCENARIO FAILED: the run did not end within $capGuard s."; Exit-Run 1 }
if ($rc -eq 2) { Write-Output "SCENARIO FAILED: run_azahar reported crash symbols (rc 2). See $runOut"; Exit-Run 1 }
if ($rc -ne 0) { Write-Output "SCENARIO FAILED: run_azahar rc $rc. See $runOut"; Exit-Run 1 }

Show-Checks $lastLog $shotDir $t0
Exit-Run $script:checkRc
