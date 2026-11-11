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
    [switch]$NoKeep, # AZT-3b: default is to leave the slot's Azahar open after a matched run (paused; a later run on the slot restarts in it, warm; azreap closes it after 15 min idle, AZT3-4). -NoKeep closes it after the run.
    [ValidateSet("", "turbo", "normal")][string]$Speed = "", # BENCH3: speed of this start (default: -Turbo in the scenario, else normal)
    [switch]$List,
    [switch]$Queue, # AZT11-3: print the run queue and the running runs (one line each), then exit
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
function Get-StartDeadline([string]$kind, [int]$scDeadline) {
    # AZT-DL: the first-marker deadline per start kind. A cold start (new Azahar process) gets at least 45 s under load; a warm restart keeps the scenario deadline.
    if ($kind -eq "cold") { return [Math]::Max($scDeadline, 45) }
    return $scDeadline
}
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
    $dlCases = @(
        @{ k = "cold"; s = 20; e = 45 }, @{ k = "cold"; s = 30; e = 45 }, @{ k = "cold"; s = 60; e = 60 },
        @{ k = "warm"; s = 20; e = 20 }, @{ k = "warm"; s = 30; e = 30 }
    )
    foreach ($d in $dlCases) {
        $got = Get-StartDeadline $d.k $d.s
        $dlOk = ($got -eq $d.e)
        if (-not $dlOk) { $fails++ }
        Write-Output ("{0} start deadline: {1} start, scenario {2} s -> {3} s (want {4} s)" -f $(if ($dlOk) { "PASS" } else { "FAIL" }), $d.k, $d.s, $got, $d.e)
    }
    # AZT11-1/2: the queue rules on fake entries in a temp folder (no Azahar, no slot mutex: the slot lane and the free slots are stubbed)
    $qdir = Join-Path $env:TEMP ("azq_selftest_" + [guid]::NewGuid().ToString("N"))
    $env:PM_AZQUEUE_DIR = $qdir
    $script:fakeLane = @{ 1 = ""; 2 = "" }
    $script:fakeFree = @{ 1 = $true; 2 = $true }
    function Get-SlotLane([int]$k) { return [string]$script:fakeLane[$k] }
    function Test-SlotFree([int]$k) { return [bool]$script:fakeFree[$k] }
    $now = Get-NowMs
    function New-FakeQ([string]$name, [long]$t, [string]$lane, [string]$state = "wait", [int]$owner = $PID, [int]$want = 0, [long]$hb = 0, [int]$skipped = 0) {
        if ($hb -eq 0) { $hb = $now }
        $o = [ordered]@{ t = $t; tree = "selftest"; lane = $lane; speed = "normal"; scenario = $name; row = "selftest"; want = $want; owner = $owner; ownerStart = (Get-ProcStart $owner)
            parent = 0; parentStart = [long]0; hb = $hb; state = $state; slot = 0; runner = 0; runnerStart = [long]0; cap = 30; runT = $now; skipped = $skipped }
        $path = Join-Path $qdir ("{0:D15}-{1}.json" -f $t, $name)
        Save-QEntry $path ([pscustomobject]$o)
        return $path
    }
    function Get-QState([string]$path) { $e = Read-QEntry $path; if ($e) { return $e } else { return $null } }
    $qFails = 0
    function QCheck([string]$name, [bool]$ok) {
        if (-not $ok) { $script:qFails++ }
        Write-Output ("{0} queue: {1}" -f $(if ($ok) { "PASS" } else { "FAIL" }), $name)
    }
    # T1 FIFO without a lane match: the oldest waiting entry gets slot 1
    New-Item -ItemType Directory -Force -Path $qdir | Out-Null
    $pA = New-FakeQ "A1" 1000 "x"; $pB = New-FakeQ "B1" 2000 "y"
    $script:fakeLane = @{ 1 = ""; 2 = "" }; $script:fakeFree = @{ 1 = $true; 2 = $false }
    Invoke-QGrant
    $a = Get-QState $pA; $b = Get-QState $pB
    QCheck "FIFO: oldest entry granted slot 1, the younger waits (skipped A 0)" (($a.state -eq "granted") -and ([int]$a.slot -eq 1) -and ($b.state -eq "wait") -and ([int]$a.skipped -eq 0))
    Remove-Item $qdir -Recurse -Force
    # T2 lane preference: slot 1 holds lane y, so the younger lane-y entry is granted, the older one is passed over once
    New-Item -ItemType Directory -Force -Path $qdir | Out-Null
    $pA = New-FakeQ "A2" 1000 "x"; $pB = New-FakeQ "B2" 2000 "y"
    $script:fakeLane = @{ 1 = "y|normal"; 2 = "" }; $script:fakeFree = @{ 1 = $true; 2 = $false }
    Invoke-QGrant
    $a = Get-QState $pA; $b = Get-QState $pB
    QCheck "lane: the lane-y entry is granted on the lane-y slot, the older lane-x entry gets skipped 1" (($b.state -eq "granted") -and ([int]$b.slot -eq 1) -and ([int]$a.skipped -eq 1))
    Remove-Item $qdir -Recurse -Force
    # T3 never more than 2 turns behind: an entry skipped twice is granted before a lane match
    New-Item -ItemType Directory -Force -Path $qdir | Out-Null
    $pA = New-FakeQ "A3" 1000 "x" -skipped 2; $pB = New-FakeQ "B3" 2000 "y"
    $script:fakeLane = @{ 1 = "y|normal"; 2 = "" }; $script:fakeFree = @{ 1 = $true; 2 = $false }
    Invoke-QGrant
    $a = Get-QState $pA; $b = Get-QState $pB
    QCheck "bypass limit: the entry skipped twice is granted, the lane match waits" (($a.state -eq "granted") -and ($b.state -eq "wait"))
    Remove-Item $qdir -Recurse -Force
    # T4 a GPU retry (want 2) is granted slot 2 only; slot 1 stays free for the next entry
    New-Item -ItemType Directory -Force -Path $qdir | Out-Null
    $pA = New-FakeQ "A4" 1000 "x" -want 2; $pB = New-FakeQ "B4" 2000 "y"
    $script:fakeLane = @{ 1 = ""; 2 = "" }; $script:fakeFree = @{ 1 = $true; 2 = $true }
    Invoke-QGrant
    $a = Get-QState $pA; $b = Get-QState $pB
    QCheck "want: the retry gets slot 2, the other entry gets slot 1" (([int]$a.slot -eq 2) -and ([int]$b.slot -eq 1))
    Remove-Item $qdir -Recurse -Force
    # T5 a reserved slot (granted or running entry) is not granted again
    New-Item -ItemType Directory -Force -Path $qdir | Out-Null
    $pR = New-FakeQ "R5" 500 "x" "running"; $pB = New-FakeQ "B5" 2000 "y"
    $r = Get-QState $pR; $r.slot = 1; Save-QEntry $pR $r
    $script:fakeFree = @{ 1 = $true; 2 = $false }
    Invoke-QGrant
    $b = Get-QState $pB
    QCheck "reserve: slot 1 held by a running entry is not granted" ($b.state -eq "wait")
    Remove-Item $qdir -Recurse -Force
    # T6 AZT11-2 (a): a dead owner's entry is removed (the owner pid is not alive)
    New-Item -ItemType Directory -Force -Path $qdir | Out-Null
    $pD = New-FakeQ "D6" 1000 "x" "wait" 2147000
    $pL = New-FakeQ "L6" 2000 "x"
    Invoke-QSweep
    QCheck "stale a: a waiting entry of a dead owner is removed, a live one stays" ((-not (Test-Path $pD)) -and (Test-Path $pL))
    Remove-Item $qdir -Recurse -Force
    # T7 AZT11-2 (b): a heartbeat older than 60 s
    New-Item -ItemType Directory -Force -Path $qdir | Out-Null
    $pH = New-FakeQ "H7" 1000 "x" "wait" $PID 0 ($now - 61000)
    $pF = New-FakeQ "F7" 2000 "x" "wait" $PID 0 ($now - 5000)
    Invoke-QSweep
    QCheck "stale b: heartbeat 61 s old is removed, 5 s old stays" ((-not (Test-Path $pH)) -and (Test-Path $pF))
    Remove-Item $qdir -Recurse -Force
    # T8 AZT11-2 (c): a running entry past its cap plus 30 s (cap 30, run 61 s ago; a run 20 s ago stays)
    New-Item -ItemType Directory -Force -Path $qdir | Out-Null
    $pC = New-FakeQ "C8" 1000 "x" "running"
    $c = Get-QState $pC; $c.runT = $now - 61000; Save-QEntry $pC $c
    $pO = New-FakeQ "O8" 2000 "x" "running"
    $o = Get-QState $pO; $o.runT = $now - 20000; Save-QEntry $pO $o
    Invoke-QSweep
    QCheck "stale c: running 61 s past start (cap 30) is removed, running 20 s stays" ((-not (Test-Path $pC)) -and (Test-Path $pO))
    Remove-Item $qdir -Recurse -Force
    # T9 AZT12: runT is the scenario clock. A process started 80 s ago (cap 45, past cap+30) whose clock restarted just now (a warm round
    # or a cold fallback, Set-QRunClock) is not stale; before the restart it is (c)
    New-Item -ItemType Directory -Force -Path $qdir | Out-Null
    $pK = New-FakeQ "K9" 1000 "x" "running"
    $k = Get-QState $pK; $k.runT = $now - 80000; $k.cap = 45; Save-QEntry $pK $k
    $kOld = Get-StaleReason (Get-QState $pK) (Get-NowMs)
    Set-QRunClock $pK
    $kNew = Get-StaleReason (Get-QState $pK) (Get-NowMs)
    QCheck "clock: process 80 s old with cap 45 is c before the clock restart, fresh after it (before '$kOld', after '$kNew')" (($kOld -eq "c") -and ($kNew -eq ""))
    Remove-Item $qdir -Recurse -Force
    Remove-Item Env:PM_AZQUEUE_DIR -ErrorAction SilentlyContinue
    if ($qFails) { Write-Output "QUEUE SELFTEST: $qFails FAILED" } else { Write-Output "QUEUE SELFTEST: all pass" }
    $fails += $qFails
    Remove-Item $tmp -ErrorAction SilentlyContinue
    if ($fails) { Write-Output "SELFTEST FAILED: $fails case(s)"; exit 1 }
    Write-Output "SELFTEST PASS"
    exit 0
}
if ($Queue) {
    Get-QueueSummary | ForEach-Object { Write-Output $_ }
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
    $listSlotFile = Join-Path $env:TEMP ("azt_list_slot_{0}.txt" -f $PID); Remove-Item $listSlotFile -ErrorAction SilentlyContinue
    $env:PM_AZT_LIST_SLOTFILE = $listSlotFile; $env:PM_AZT_LIST_SLOT = "" # TOOLB3: the first child writes its slot, the next children ask for it
    foreach ($n in $names) {
        $ca = @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File", $PSCommandPath, $n, "-Row", $Row)
        if ($Build) { $ca += @("-Build", $Build) }
        if ($Try) { $ca += "-Try" }
        if ($Speed) { $ca += @("-Speed", $Speed) }
        $t1 = Get-Date
        Write-Output ("LIST: start {0} at {1}" -f $n, $t1.ToString("HH:mm:ss"))
        & $childExe @ca
        $rc = $LASTEXITCODE
        if (-not $env:PM_AZT_LIST_SLOT -and (Test-Path $listSlotFile)) { $env:PM_AZT_LIST_SLOT = (Get-Content $listSlotFile -First 1).Trim() } # TOOLB3
        $sec = ((Get-Date) - $t1).TotalSeconds
        Write-Output ("LIST: {0} rc {1}, wall {2:N1} s" -f $n, $rc, $sec)
        $summary += ("{0} rc {1} {2:N1} s" -f $n, $rc, $sec)
        if ($rc -gt $worst) { $worst = $rc }
        if ($rc -eq 9) { Write-Output "LIST: row $Row has no starts left. The rest of the list did not run."; break }
    }
    Remove-Item $listSlotFile -ErrorAction SilentlyContinue; $env:PM_AZT_LIST_SLOTFILE = ""; $env:PM_AZT_LIST_SLOT = "" # TOOLB3
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
# SHOTLANE: the shot archive is per scenario AND build (build3ds\azt\shots\<scenario>\<build>): the lane name, or main without -Build
$buildName = if ($lane) { $lane } else { "main" }
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
    # SHOTCOPY1: a shot file is copied only when it is full (288015 B, 240 x 400 P6); a part-written file is not a shot of this run
    $shotFull = 288015
    if (Test-Path $shots) {
        # only the top-screen shots (NNNN_ma_mm.ppm, 288015 B) have the full-size rule; bottom shots (bot_*.ppm, 230415 B) are copied as before
        $shotFiles = @(Get-ChildItem $shots -File | Where-Object { $_.LastWriteTime -ge $since -and ($_.Name -notmatch '^\d{4}_' -or $_.Length -eq $shotFull) } | Sort-Object Name)
        $shotShort = @(Get-ChildItem $shots -File | Where-Object { $_.LastWriteTime -ge $since -and $_.Name -match '^\d{4}_' -and $_.Length -ne $shotFull })
        if ($shotShort.Count) { Write-Output ("SHOT FILES SHORT: {0} file(s) not {1} B: {2}" -f $shotShort.Count, $shotFull, (($shotShort | ForEach-Object { "$($_.Name)=$($_.Length)" }) -join " ")); $script:shotCopyBad = $true }
    }
    if ($since -gt [datetime]::MinValue -and $shotFiles.Count -gt 0) {
        # Keep this run's shots per scenario: the shared shots folder is overwritten by the next run
        $arch = Join-Path $repo "build3ds\azt\shots\$($sc.name)\$buildName"
        New-Item -ItemType Directory -Force -Path $arch | Out-Null
        Remove-Item (Join-Path $arch "*") -Force -ErrorAction SilentlyContinue
        foreach ($f in $shotFiles) { Copy-Item $f.FullName $arch }
        $shotFiles = @(Get-ChildItem $arch -File | Sort-Object Name)
    }
    if ($since -gt [datetime]::MinValue) {
        # The slot is free for the next run once this script ends, and that run overwrites the slot log: read this run's log from this copy
        $ld = Join-Path $repo "build3ds\azt\logs"
        New-Item -ItemType Directory -Force -Path $ld | Out-Null
        $dst = Join-Path $ld "$($sc.name).txt"
        $hdr = ""
        if ($script:azLine) { $hdr += $script:azLine + "`r`n" } # AZT2: the warm/cold line goes first in the copy
        if ($script:bootLine) { $hdr += $script:bootLine + "`r`n" } # AZT-DL: then the boot time line, then the game log unchanged
        if ($hdr) {
            [IO.File]::WriteAllText($dst, $hdr)
            $fsIn = [IO.File]::OpenRead($file); $fsOut = [IO.File]::Open($dst, [IO.FileMode]::Append)
            try { $fsIn.CopyTo($fsOut) } finally { $fsIn.Dispose(); $fsOut.Dispose() }
        }
        else { Copy-Item $file $dst -Force }
        Write-Output "LOG: $(Join-Path $ld "$($sc.name).txt") (copy of this run's log; slot $slotK)"
    }
    Write-Output "SUBJECT: $($sc.subject)"
    Write-Output "SHOTS: $($shotFiles.Count) from this run"
    foreach ($f in $shotFiles) { Write-Output "  $($f.FullName)" }
    if ($sc.needs_picture -and $shotFiles.Count -eq 0) { Write-Output "FAIL  picture: no shot file from this run"; $fails++ }
    if ($script:shotCopyBad) { Write-Output "FAIL  shot copy: a logged shot has no full file (SHOT COPY FAILED or SHOT FILES SHORT above)"; $fails++ }
    if ($fails) { Write-Output "SCENARIO FAILED: $fails check(s) failed."; $script:checkRc = 1; return }
    if ($script:checkRc -eq 0) { Write-Output "LOG CHECKS PASS ($(@($sc.expected_events).Count) of $(@($sc.expected_events).Count)). The script does not read the picture. A person must open the shot and confirm the subject." }
}

function Set-ScenarioStatus([string]$name, [string]$status, [string]$reason) {
    # AZT11-5: the status (and the reason, when the object has none) of one scenario in azt_scenarios.json, text edit only (the file keeps its layout)
    $p = Join-Path $PSScriptRoot "azt_scenarios.json"
    $t = [IO.File]::ReadAllText($p)
    $rx = [regex]('"name":\s*"' + [regex]::Escape($name) + '"')
    $m = $rx.Match($t)
    if (-not $m.Success) { return $false }
    $next = $t.IndexOf('"name":', $m.Index + $m.Length)
    $end = if ($next -gt 0) { $next } else { $t.Length }
    $seg = $t.Substring($m.Index, $end - $m.Index)
    $sm = [regex]::Match($seg, '"status":\s*"[^"]*"')
    if (-not $sm.Success) { return $false }
    $nl = if ($t.Contains("`r`n")) { "`r`n" } else { "`n" }
    $ind = [regex]::Match($seg, '
([ 	]*)"status"').Groups[1].Value
    $new = '"status": "' + $status + '"'
    if ($seg -notmatch '"reason"') { $new += ',' + $nl + $ind + '"reason": "' + $reason.Replace('"', "'") + '"' }
    $seg2 = $seg.Substring(0, $sm.Index) + $new + $seg.Substring($sm.Index + $sm.Length)
    [IO.File]::WriteAllText($p, $t.Substring(0, $m.Index) + $seg2 + $t.Substring($end), (New-Object Text.UTF8Encoding($false)))
    return $true
}
function Exit-Run([int]$code) {
    # Records this scenario start in 3ds/logs/perf/runs.jsonl (bench.py ingest-last): under the bench of this scenario, else adhoc:<scenario>.
    $ia = @((Join-Path $repo "3ds\tools\bench.py"), "ingest-last", "--log", $lastLog, "--scenario", $sc.name, "--row", $Row,
            "--rc", [string]$code, "--shots", (Join-Path $repo "build3ds\azt\shots\$($sc.name)\$buildName"))
    if ($env:PM_BENCH_LABEL) { $ia += @("--label", $env:PM_BENCH_LABEL) }
    if ($env:PM_BENCH_BUILD) { $ia += @("--build", $env:PM_BENCH_BUILD) }
    if ($env:PM_BENCH_COMMIT) { $ia += @("--commit", $env:PM_BENCH_COMMIT) }
    $ia += @("--speed", $speed)
    if ($null -ne $script:azWarm) { $ia += @("--warm", [string]$script:azWarm) } # AZT2: 1 warm, 0 cold (the azslot line)
    if ($null -ne $frameLimit) { $ia += @("--frame-limit", [string]$frameLimit) }
    if ($null -ne $wallS) { $ia += @("--wall-s", [string]$wallS) } # AZT10b-2: the run timing (bench record)
    if ($null -ne $untilS) { $ia += @("--until-s", [string]$untilS) }
    if ($null -ne $afterS) { $ia += @("--end-after-until-s", [string]$afterS) }
    if ($null -ne $qWaitS) { $ia += @("--wait-s", [string]$qWaitS) } # AZT12: queue wait, enqueue to grant
    if ($null -ne $script:bootS) { $ia += @("--boot-s", [string]$script:bootS) } # AZT12: Azahar start (cold launch or warm round) to the first game line of this boot
    & python @ia
    if ($LASTEXITCODE -ne 0) { Write-Output "BENCH: run not recorded (bench.py exit $LASTEXITCODE)" }
    if ($script:slotK) { Exit-Slot $script:slotK; $script:slotK = 0 } # the slot is free for the next run once the logs are read and recorded
    Leave-Queue # AZT11-1: the run leaves the queue (its slot is free)
    exit $code
}

if ($CheckLog) {
    Show-Checks $CheckLog $shotDir ([datetime]::MinValue)
    exit $script:checkRc
}

# Start the run
$dbgArg = [string]$sc.debug
$argv = @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File", (Join-Path $repo "3ds\run_azahar.ps1"),
          "-Until", [string]$sc.until, "-Timeout", [string]$cap, "-Force", "-Debug", $dbgArg)
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
$azTurbo = ($speed -eq "turbo")
$azTL = if ($azTurbo) { [int]$frameLimit } else { 0 }
# AZT11-1: the run waits in the machine-wide queue (azslot.ps1 Invoke-QGrant grants a slot). The GPU retry asks for its slot (want).
# The entry is the run's own file: it is removed at every exit (Leave-Queue), and a dead owner's entry is swept by any queue user (AZT11-2).
$wantSlot = if ($env:PM_AZT_RETRY_RUN -and $env:PM_AZT_RETRY_SLOT) { [int]$env:PM_AZT_RETRY_SLOT } elseif ($env:PM_AZT_LIST_SLOT) { [int]$env:PM_AZT_LIST_SLOT } else { 0 } # TOOLB3: a list child asks for the slot of the first child
$qTree = Split-Path -Leaf $script:AzTree
$qE = New-QEntry $sc.name $Row $lane $speed $qTree $wantSlot (Get-NowMs)
$qPath = Join-Path (Get-QueueDir) ("{0:D15}-{1}-{2}.json" -f [long]$qE.t, $PID, (Get-Random))
$script:qPath = $qPath
Invoke-QLocked { Save-QEntry $qPath $qE }
function Leave-Queue { if ($script:qPath) { Remove-Item $script:qPath -Force -ErrorAction SilentlyContinue; $script:qPath = "" } }
$qT0 = Get-Date; $qHb = Get-Date; $qSaid = $false; $slotK = 0; $qPosAtStart = 0
while ($true) {
    Invoke-QSweep
    if (-not $qSaid) {
        # the position at enqueue: waiting entries older than this one, plus this one
        $ws = @(Read-QEntries | Where-Object { [string]$_.state -eq "wait" })
        $qPosAtStart = 1 + @($ws | Where-Object { [long]$_.t -lt [long]$qE.t }).Count
        Write-Output ("queue: position {0} of {1} waiting (lane '{2}', speed {3}, scenario {4}, want slot {5})" -f $qPosAtStart, $ws.Count, $lane, $speed, $sc.name, $(if ($wantSlot) { $wantSlot } else { "any" }))
        $qSaid = $true
    }
    Invoke-QGrant
    $snap = Read-QEntry $qPath
    if (-not $snap) {
        # removed by a sweep while this run waited (its owner looked stale): queued again at its first time
        Invoke-QLocked { Save-QEntry $qPath $qE }
        Write-Output "queue: the entry was removed as stale, queued again at its first time"
        $snap = Read-QEntry $qPath
    }
    if ([string]$snap.state -eq "granted") { $slotK = [int]$snap.slot; break }
    if (((Get-Date) - $qHb).TotalSeconds -ge 2) { Set-QFields $qPath @{ hb = (Get-NowMs) }; $qHb = Get-Date }
    if (((Get-Date) - $qT0).TotalSeconds -gt 900) { Leave-Queue; Write-Output "LOCK TIMEOUT: no free Azahar slot after 900 s. Nothing was run."; exit 3 }
    Start-Sleep -Milliseconds 400
}
$qWaitS = [math]::Round(((Get-Date) - $qT0).TotalSeconds, 1)
# AZKEEP: a scenario whose extra args make run_azahar start cold (run_azahar.ps1:238) asks for a free slot without a kept Azahar first
$needCold = [bool]($extraArgs | Where-Object { $_ -in "-NoWarm", "-Interactive", "-Gdb", "-FaultStop" })
$slotOk = Get-Slot $slotK 900 $azTurbo $azTL $needCold
if (-not $slotOk) { Leave-Queue; Write-Output "LOCK TIMEOUT: granted slot $slotK is not free after 900 s. Nothing was run."; exit 3 }
Write-Output ("queue: granted slot {0} after {1} s (position {2} at enqueue)" -f $slotK, $qWaitS, $qPosAtStart)
if ($env:PM_AZT_LIST_SLOTFILE -and -not $env:PM_AZT_LIST_SLOT) { Set-Content $env:PM_AZT_LIST_SLOTFILE ([string]$slotK) } # TOOLB3
$queueInfo = "queue: position $qPosAtStart at enqueue; waited $qWaitS s; slot $slotK"
$env:PM_QUEUE_INFO = $queueInfo
$slotInfo = Ensure-Slot $slotK
$sd = $slotInfo.sd; $logPath = Join-Path $sd "log.txt"; $pidFile = $slotInfo.pidFile
$lastLog = Join-Path $outDir "slot$slotK\last_log.txt"
$shotDir = Join-Path $outDir "slot$slotK\shots"
# AZT7-1: this start's shot output starts empty: the slot's shots folder and this build's archive hold no file of an earlier run
Remove-Item $shotDir -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item (Join-Path $repo "build3ds\azt\shots\$($sc.name)\$buildName") -Recurse -Force -ErrorAction SilentlyContinue
$env:PM_SLOT_HELD = "1"
$argv += @("-Slot", [string]$slotK)
if (-not $NoKeep) { $argv += @("-Keep") } # AZT-3b: kept by default. AZT3-2: run_azahar keeps it after any run that did not crash or exit.
$warmFile = Join-Path $slotInfo.dir "warm.start"
Remove-Item $warmFile -ErrorAction SilentlyContinue
$stopFile = Join-Path $slotInfo.dir "stop.request"; Remove-Item $stopFile -ErrorAction SilentlyContinue # AZT3-2: this start's early stop only
$okFile = Join-Path $slotInfo.dir "warm.ok"; Remove-Item $okFile -ErrorAction SilentlyContinue # AZT3-3: run_azahar.ps1 writes it when the warm boot has started
$azLineFile = Join-Path $slotInfo.dir "azslot.line"
$warmReady = $true # AZT3-3: false while a warm start waits for warm.ok (or the cold fallback in azslot.line)
$warmStamp = "" # AZT12: the warm.start content (its ticks) of the last clock restart
Remove-Item (Join-Path $slotInfo.dir "azslot.line") -ErrorAction SilentlyContinue # AZT2: a line of this start only
$cmd = ($argv | ForEach-Object { if ($_ -match '^-[A-Za-z]+$') { $_ } else { '"' + $_ + '"' } }) -join " "

$env:PM_ROW = $Row
$env:PM_AZT_ATTEMPT = if ($env:PM_AZT_RETRY_RUN) { "2" } else { "1" } # AZFIX: run_azahar.ps1 writes the end line as retry on attempt 1 only
if ($lane) { $env:PM_LANE = $lane } else { Remove-Item Env:PM_LANE -ErrorAction SilentlyContinue }
if (Test-Path $lastLog) { Remove-Item $lastLog -Force -ErrorAction SilentlyContinue } # AZFIX: two azt runs may race here
$t0 = Get-Date
$runOut = Join-Path $outDir "last_crash.txt"; Remove-Item $runOut -ErrorAction SilentlyContinue # TOOLB1: run_azahar writes the crash symbols here
# No output redirect: with a redirect the child exit code is lost in this shell. The result is read from the budget file.
$child = Start-Process -FilePath (Get-Process -Id $PID).Path -ArgumentList $cmd -PassThru -WindowStyle Hidden
$script:childPid = $child.Id
Set-QFields $qPath @{ state = "running"; slot = $slotK; runner = $child.Id; runnerStart = (Get-ProcStart $child.Id); runT = (Get-NowMs); cap = $cap; hb = (Get-NowMs) }
$qHb = Get-Date
Write-Output "AZT: scenario $($sc.name), row $Row, lane '$lane', slot $slotK, cap $cap s, deadline $deadline s after the Azahar start"

$azStart = $null
$markerSeen = $false
$mtext = "" # AZT3-3: the text of this boot (from its banner line) for the marker
$stop = ""
$attract = 0
$capGuard = $cap + 120
$startKind = ""
$deadlineEff = $deadline
$script:bootLine = $null
$script:bootS = $null
while (-not $child.HasExited) {
    Start-Sleep -Milliseconds 500
    if (((Get-Date) - $qHb).TotalSeconds -ge 2) { Set-QFields $qPath @{ hb = (Get-NowMs) }; $qHb = Get-Date } # AZT11-2: heartbeat
    if (-not $azStart) {
        if (Get-AzaharProc $t0) {
            $azStart = Get-Date; $lastNew = $azStart; $startKind = "cold"; $deadlineEff = Get-StartDeadline "cold" $deadline
            Set-QRunClock $qPath # AZT12: the run's cap clock starts with the cold launch
            Write-Output "AZT: Azahar started, cold start, deadline kind cold: $deadlineEff s (scenario $deadline s, cold minimum 45 s). The deadline clock runs"
        }
        elseif (Test-Path $warmFile) {
            # AZT-2: run_azahar.ps1 restarts emulation in the kept Azahar and wrote warm.start. The old log.txt content does not count.
            $azStart = Get-Date; $lastNew = $azStart; $script:warm = $true; $startKind = "warm"; $deadlineEff = Get-StartDeadline "warm" $deadline
            $warmStamp = (Get-Content $warmFile -Raw).Trim(); Set-QRunClock $qPath # AZT12: the run's cap clock starts with this warm round
            # AZT3-3: the restart truncates log.txt (the creation time stays), so the old file length is no mark for the new boot.
            # The log is read from its first byte once warm.ok says the new boot has started (below).
            $warmReady = $false
            Write-Output "AZT: warm restart in the kept Azahar, deadline kind warm: $deadlineEff s (scenario deadline). The deadline clock runs"
        }
        continue
    }
    if (-not $warmReady) {
        # AZT3-3: a warm start reads nothing until the new boot has started (warm.ok), so no line of the old boot is read.
        # If run_azahar.ps1 fell back to a cold start (azslot.line says cold), this start is a cold start with its own deadline.
        # AZT12: a second warm round (run_azahar.ps1 rewrites warm.start) restarts the run's cap clock too, as run_azahar.ps1 does.
        if (Test-Path $warmFile) {
            try { $ws = (Get-Content $warmFile -Raw).Trim() } catch { $ws = $warmStamp }
            if ($ws -and $ws -ne $warmStamp) { $warmStamp = $ws; Set-QRunClock $qPath; $azStart = Get-Date; $lastNew = $azStart } # AZT12-5: run_azahar.ps1 writes warm.start at each restart try: the deadline clock restarts too
        }
        if (Test-Path $okFile) { $warmReady = $true; $script:pos = 0; $script:carry = ""; $script:logCT = $null }
        elseif (Test-Path $azLineFile) {
            $warmReady = $true; $script:pos = 0; $script:carry = ""; $script:logCT = $null
            $startKind = "cold"; $azStart = Get-Date; $lastNew = $azStart; $deadlineEff = Get-StartDeadline "cold" $deadline
            Set-QRunClock $qPath # AZT12: the cold launch after a failed warm restart starts the run's cap clock
            Write-Output "AZT: warm restart failed, cold start in this slot, deadline kind cold: $deadlineEff s. The deadline clock runs"
        }
        continue
    }
    $chunk = Read-LogChunk
    if ($chunk) {
        $lastNew = Get-Date
        if ($null -eq $script:bootS) { $script:bootS = [math]::Round(($lastNew - $azStart).TotalSeconds, 1) } # AZT12: boot_s
        # AZT3-3: a boot starts at its banner line. The marker counts only after the newest banner, so an old boot's line is never a marker.
        $bi = $chunk.LastIndexOf("#0 log: writer id")
        if ($bi -ge 0) { $mtext = $chunk.Substring($bi) } else { $mtext += $chunk }
        if (-not $markerSeen -and $mtext -match $sc.first_marker) {
            $markerSeen = $true
            $script:bootLine = ("AZT: boot time {0} start, first marker after {1:N1} s (deadline kind {2}, {3} s)" -f $startKind, ((Get-Date) - $azStart).TotalSeconds, $startKind, $deadlineEff)
            Write-Output $script:bootLine
        }
        $attract += ([regex]::Matches($chunk, $attractRe)).Count
    }
    $el = ((Get-Date) - $azStart).TotalSeconds
    if (-not $markerSeen -and $el -gt $deadlineEff) { $stop = "marker"; break }
    if ($expectsWarp -and $attract -gt 2) { $stop = "attract"; break }
    if ($el -gt $capGuard) { $stop = "guard"; break }
    if (Test-StallExpired $lastNew (Get-Date) $stallSec) { $stop = "stall"; break }
}
if ($stop) {
    # AZT3-2: run_azahar.ps1 ends the run at its next poll and keeps the Azahar (paused), so the next start is warm. -NoKeep closes it there.
    Write-Output "AZT: stopping the run ($stop). run_azahar.ps1 ends it; the Azahar is kept unless -NoKeep."
    Set-Content $stopFile $stop
}
# AZT12-5: the wait for run_azahar.ps1 keeps the queue heartbeat (a silent wait over 60 s looked stale to the sweep of another run)
for ($wi = 0; $wi -lt 75 -and -not $child.WaitForExit(2000); $wi++) { Set-QFields $qPath @{ hb = (Get-NowMs) } }
if (-not $child.HasExited) {
    Stop-Process -Id $child.Id -Force -ErrorAction SilentlyContinue
    Write-Output "NOT ENDED: run_azahar.ps1 (PID $($child.Id)) did not end. It was stopped by force. Check for an Azahar with Get-Process azahar."
}
$rc = $child.ExitCode
# AZT2: the warm/cold line run_azahar.ps1 wrote for this start (none when the run ended before the decision)
if (Test-Path $azLineFile) {
    $script:azLine = (Get-Content $azLineFile -Raw).Trim()
    Write-Output $script:azLine
    if ($script:azLine -match ' warm$') { $script:azWarm = 1 } elseif ($script:azLine -match ' cold \(') { $script:azWarm = 0 }
}
if ($rc -eq 9) { Leave-Queue; Write-Output "BUDGET: row $Row has no starts left. Nothing more was run. The orchestrator resets a row with run_azahar.ps1 -ResetBudget <row>."; exit 9 }
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
if (-not $endRow -and -not (Test-Path $qPath)) {
    # AZT12-5: the queue entry is gone while this run ran: the sweep of another run booted it as stale (pool.log "stale") and killed run_azahar.ps1
    Leave-Queue; Write-Output "BOOTED: the queue sweep of another run stopped this run as stale (build3ds\azslots\pool.log, event stale). run_azahar.ps1 wrote no end line. Rerun it."; exit 1
}
if (-not $endRow) { Leave-Queue; Write-Output "NO END LINE: run_azahar.ps1 wrote no end line for row $Row after this start. It did not run (lock timeout, switch error or budget). Nothing was checked."; exit 1 }
# AZT-GPU: Azahar died before the first marker with the GPU stream buffer failure (Vulkan "Failed to allocate Stream buffer") or exit 0xC000001D (-1073741795).
$emuTxt = if (Test-Path $slotInfo.emuLog) { Get-Content $slotInfo.emuLog -Raw } else { "" }
$gpuFault = (-not $markerSeen) -and (($emuTxt -match 'Failed to allocate Stream buffer') -or (($endRow -join ",") -match '-1073741795'))
if ($gpuFault -and -not $env:PM_AZT_RETRY_RUN) {
    # AZT4-2: the kept Azahars that hold GPU memory are closed (idle ones only, any worktree), then the same slot retries once after 2 s
    Write-Output "AZT: GPU memory fault, retry. Same slot $slotK (no marker before the Azahar exit)."
    $env:PM_AZT_RETRY_RUN = "1"; $env:PM_AZT_RETRY_SLOT = [string]$slotK
    Exit-Slot $slotK; $slotK = 0; Leave-Queue # the retry queues again: this entry must not hold the slot it asks for
    $closedIdle = Close-IdleKept
    Write-Output "azslot: gpu fault, closed $closedIdle idle, retry"
    Start-Sleep -Seconds 2
    $ga = @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File", $PSCommandPath, $Scenario, "-Row", $Row, "-Try")
    if ($Build) { $ga += @("-Build", $Build) }
    if ($NoKeep) { $ga += "-NoKeep" }
    & (Get-Process -Id $PID).Path @ga
    exit $LASTEXITCODE
}
if ($endRow[4] -eq "retry" -and -not $env:PM_AZT_RETRY_RUN) {
    # AZFIX: Azahar exited with an empty emulator log. One retry, its start line is retry-start (not counted).
    Write-Output "RETRY: Azahar exited with an empty emulator log. Retrying once. The retry is not counted in the row budget."
    $env:PM_AZT_RETRY_RUN = "1"
    Exit-Slot $slotK; $slotK = 0; Leave-Queue # the retry queues again: this entry must not hold the slot it asks for
    $ra = @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File", $PSCommandPath, $Scenario, "-Row", $Row, "-Try")
    if ($Build) { $ra += @("-Build", $Build) }
    if ($NoKeep) { $ra += "-NoKeep" }
    & (Get-Process -Id $PID).Path @ra
    exit $LASTEXITCODE
}
$rc = [int]$endRow[5]
Write-Output ("run: rc {0}: {1}" -f $rc, (($endRow[6..($endRow.Count - 1)]) -join ","))
# AZT10b-2: the run's timing from the run result (run_azahar End-Run text): wall seconds, until seconds, end after the until line
$runTxt = (($endRow[6..($endRow.Count - 1)]) -join ",")
$script:shotCopyBad = $runTxt -match 'SHOT COPY FAILED'; # SHOTCOPY1: run_azahar counts the logged shots against the full files
if ($script:shotCopyBad) {
    # BENCHFIX: a logged shot without its full file fails the run (rc 1, the record is fail:1, never log_pass), whatever the log checks say
    Write-Output "SCENARIO FAILED: SHOT COPY FAILED: a logged shot has no full file after 30 s. Its picture is not in the shots folder. Not a pass."
    Exit-Run 1
}
$tmW = [regex]::Match($runTxt, 'stopped after (\d+)s'); $tmU = [regex]::Match($runTxt, 'until at ([\d.]+) s'); $tmA = [regex]::Match($runTxt, '([\d.]+) s after the until line')
$wallS = if ($tmW.Success) { [double]$tmW.Groups[1].Value } else { $null }
$untilS = if ($tmU.Success) { [double]$tmU.Groups[1].Value } else { $null }
$afterS = if ($tmA.Success) { [double]$tmA.Groups[1].Value } else { $null }
# AZT10b-3: a scenario with a cap above 25 s and a measured until names itself, so its cap gets fixed as it runs
if ($null -ne $untilS -and [double]$sc.cap -gt 25) { Write-Output ("cap high: {0} cap {1} until {2}" -f $sc.name, $sc.cap, $untilS) }
if ($runTxt -match 'stopped after \d+s: timeout \(' -and $stop -eq "") {
    # AZT11-5: the until line never came in the timeout. A cold start spends its first 12 to 17 s on the boot: a run whose timeout was mostly
    # boot is cap short (exit 1, rerun or raise the cap), not broken. Broken: the until line did not come in 60 % of the timeout after the first marker.
    $tmoM = [regex]::Match($runTxt, 'timeout \((\d+) s\)')
    $tmoS = if ($tmoM.Success) { [double]$tmoM.Groups[1].Value } else { [double]$cap }
    $bootM = [regex]::Match([string]$script:bootLine, 'first marker after ([\d.]+) s')
    $bootS = if ($bootM.Success) { [double]$bootM.Groups[1].Value } else { 0.0 }
    $afterS = [double]$wallS - $bootS
    if ($afterS -lt (0.6 * $tmoS)) {
        Write-Output ("SCENARIO CAP SHORT: {0}: the timeout {1} s had {2} s of {3} start before the first marker; {4} s left for the until line. Not marked broken: rerun on a warm slot or raise the cap." -f $sc.name, $tmoS, $bootS, $startKind, [math]::Round($afterS, 1))
        Exit-Run 1
    }
    # AZT11-5: the until line never came in the cap. The scenario is marked unverified (its subject and until need a fix), bench.py records rc 10 as broken.
    Write-Output "SCENARIO BROKEN: $($sc.name) no until in $cap s (until /$($sc.until)/). Marked unverified in azt_scenarios.json."
    if (Set-ScenarioStatus $sc.name "unverified" ("broken: no until in $cap s (AZT11-5, " + (Get-Date -Format "yyyy-MM-dd") + ")")) { Write-Output "azt_scenarios.json: $($sc.name) status unverified" }
    Exit-Run 10
}
if ($stop -eq "marker") {
    Write-Output "SCENARIO FAILED: marker missing: $($sc.first_marker)"
    Write-Output "Not in the log $deadlineEff s after the Azahar start ($startKind start). Azahar was stopped. Log: $logPath"
    Exit-Run 2
}
if ($stop -eq "attract") {
    Write-Output "ATTRACT LOOP: $attract title or demo mode lines (limit 2) in a run that expects $($sc.expects). Stopped."
    Exit-Run 3
}
if ($stop -eq "stall") { Write-Output "SCENARIO FAILED: no new log line for $stallSec s. The run was stalled and Azahar was stopped."; Exit-Run 6 }
if ($stop -eq "guard") { Write-Output "SCENARIO FAILED: the run did not end within $capGuard s."; Exit-Run 1 }
if ($rc -eq 2) { Get-Content $runOut -ErrorAction SilentlyContinue; Write-Output "SCENARIO FAILED: run_azahar reported crash symbols (rc 2). See $runOut"; Exit-Run 1 }
if ($rc -ne 0) { Write-Output "SCENARIO FAILED: run_azahar rc $rc. See $runOut"; Exit-Run 1 }

Show-Checks $lastLog $shotDir $t0
Exit-Run $script:checkRc
