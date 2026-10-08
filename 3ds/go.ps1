<#
go.ps1 - single entry point for pushing to and pulling from O3DS, N3DS, or Azahar.

  powershell -File 3ds/go.ps1 push o|n|a|on|ona [-ip <addr>] [-NoBuild] [-Seconds <n>] [-Interactive]
                              [-GameDebug "switch1 switch2"] [extra args]
  powershell -File 3ds/go.ps1 pull o|n          [-ip <addr>]

push o: netload to O3DS, live-echo the game log for -Seconds (default 60).
push n: same, N3DS.
push a: run build3ds/pm_3ds.3dsx in Azahar, live-echo the game log.
  Extra args are forwarded to run_azahar.ps1, e.g.: go.ps1 push a -Capture -Timeout 30 (no --
  needed, PowerShell does not use that separator; a literal -- is itself parsed as a parameter).
-GameDebug "nolight calls": render/game debug switches (pm_main.c read_debug_switches), forwarded
  to every target (as -Debug to run_azahar.ps1, as a game arg to push_3ds.sh/3dslink for o/n).
  Deliberately not named -Debug - PowerShell gives every script using [Parameter()] an automatic
  common -Debug switch, which silently eats a real -Debug "..." before it reaches the game
  (confirmed directly: "A parameter with the name 'Debug' was defined multiple times").
push on: both O3DS and N3DS. push ona: O3DS, N3DS, and Azahar (everything).
  Multi-target: each target's result is printed as it finishes - one target failing to launch
  (console off, wrong IP, Azahar already stuck, etc.) does not stop or hide the others; every
  target gets its own clearly-labeled section and a one-line result in the final summary.

A build (unless -NoBuild) always runs once before pushing, shared by every target - but it is
skipped entirely, before touching Docker, when build3ds/pm_3ds.3dsx is already newer than every
file under 3ds/src, 3ds/include, 3ds/shaders, src/, and include/ (a real rebuild was not needed, not just a no-op ninja run).

-Interactive: a person is playing, not watching for a scripted stop condition.
  push a: no timeout, no log-silence stop, settings windows allowed (forwarded to run_azahar.ps1).
  push o/n: keeps echoing the hardware log for a long time (24h) instead of stopping at -Seconds.
  Either way, closing this script does not stop the game - run_azahar.ps1 kills only the Azahar
  process it started (other azahar.exe instances keep running), and real hardware keeps running
  regardless of whether this PC script is watching it.

pull o: FTP (port 5000) into O3DS's SD card. Downloads the game log and any Luma crash dumps to
  build3ds/hw_o3ds_log.txt and build3ds/hw_o3ds_dumps/.
pull n: same, N3DS.

Default IPs: O3DS 192.168.1.168, N3DS 192.168.1.223. -ip overrides either
(only meaningful for a single hardware target, not on/ona).
One attempt, short timeouts - this never waits indefinitely on a console (see the project's
never-block-on-live-device rule) unless -Interactive is explicitly given. push o/n/on/ona and
pull o/n touch real hardware; push a does not.
#>
param(
    [Parameter(Position = 0, Mandatory = $true)][ValidateSet('push', 'pull')] [string]$Verb,
    [Parameter(Position = 1, Mandatory = $true)][ValidateSet('o', 'n', 'a', 'on', 'ona')] [string]$Target,
    [string]$ip = "",
    [int]$Seconds = 60,
    [switch]$NoBuild,
    [switch]$Interactive,
    # Render/game debug switches (pm_main.c read_debug_switches: nofog nolight notex ... perfctr
    # calls), space-separated in one string. NOT named -Debug: PowerShell gives every script with a
    # [Parameter()] attribute an automatic common -Debug switch, and a script cannot redeclare a
    # parameter with that name ("A parameter with the name 'Debug' was defined multiple times" -
    # confirmed directly). Any -Debug "..." on this command line is silently eaten by that common
    # parameter, never reaches $Rest, and the game never sees it - this is why, not a typo.
    [string]$GameDebug = "",
    [Parameter(ValueFromRemainingArguments = $true)] [string[]]$Rest
)
$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $PSScriptRoot
Set-Location $repo

$defaultIp = @{ o = "192.168.1.168"; n = "192.168.1.223" }
$consoleName = @{ o = "O3DS"; n = "N3DS"; a = "Azahar" }
$multi = @{ on = @('o', 'n'); ona = @('o', 'n', 'a') }

if ($Verb -eq 'pull' -and $multi.ContainsKey($Target)) {
    Write-Output "pull ${Target}: not supported - pull one console at a time (pull o / pull n)"
    exit 1
}
if ($ip -and $multi.ContainsKey($Target)) {
    Write-Output "-ip is ignored for '$Target' (multiple targets) - use a single target to override an IP"
    $ip = ""
}
if ($Target -eq 'a' -and $ip) { Write-Output "-ip is ignored for Azahar (push a)"; $ip = "" }

# PowerShell's own PATH reliably has the Windows "py" launcher; bash (MSYS) does not always.
# Resolve it once here and hand it down, instead of letting push_3ds.sh guess.
function Resolve-Python {
    foreach ($name in @('py', 'python', 'python3')) {
        $cmd = Get-Command $name -ErrorAction SilentlyContinue
        if ($cmd) { return $cmd.Source }
    }
    return $null
}
$pythonExe = Resolve-Python
if ($pythonExe) { $env:PYTHON_EXE = $pythonExe }

# Bare "bash" is ambiguous on a machine with both Git Bash and WSL - System32's WSL launcher can
# win, and build.sh/push_3ds.sh need real Git Bash (MSYS path translation, Docker volume mounts
# with Windows-style paths). A background job's bash resolved to WSL even though the main session
# resolved correctly: confirmed directly (diagnostic job printed "C:\Windows\system32\bash.exe"),
# so this cannot be left to PATH order, in a job or not. Derive the real one from git.exe's
# location instead, which is unambiguous (git.exe only exists once).
function Resolve-GitBash {
    $gitCmd = Get-Command git -ErrorAction SilentlyContinue
    if ($gitCmd) {
        $candidate = Join-Path (Split-Path (Split-Path $gitCmd.Source)) "bin\bash.exe"
        if (Test-Path $candidate) { return $candidate }
    }
    foreach ($candidate in @("$env:ProgramFiles\Git\bin\bash.exe", "${env:ProgramFiles(x86)}\Git\bin\bash.exe",
                             "$env:LocalAppData\Programs\Git\bin\bash.exe")) {
        if (Test-Path $candidate) { return $candidate }
    }
    return "bash" # last resort - whatever PATH gives us
}
$gitBash = Resolve-GitBash

function Invoke-Build {
    if ($NoBuild) { return }
    $target = "build3ds\pm_3ds.3dsx"
    if (Test-Path $target) {
        $builtAt = (Get-Item $target).LastWriteTime
        # Only what actually feeds the compiled binary - not go.ps1/push_3ds.sh/run_azahar.ps1,
        # 3ds/tools/*.py, or docs, which never need a rebuild to take effect.
        $srcDirs = "3ds\src", "3ds\include", "3ds\shaders", "src", "include"
        $srcFiles = "3ds\CMakeLists.txt"
        $stale = $false
        foreach ($d in $srcDirs) {
            if (-not (Test-Path $d)) { continue }
            $m = Get-ChildItem $d -Recurse -File -ErrorAction SilentlyContinue |
                 Measure-Object -Property LastWriteTime -Maximum
            if ($m.Maximum -and $m.Maximum -gt $builtAt) { $stale = $true; break }
        }
        if (-not $stale) {
            foreach ($f in $srcFiles) {
                if ((Test-Path $f) -and (Get-Item $f).LastWriteTime -gt $builtAt) { $stale = $true; break }
            }
        }
        if (-not $stale) {
            Write-Output "--- build up to date (build3ds/pm_3ds.3dsx newer than all sources), skipping ---"
            return
        }
    }
    Write-Output "--- building ---"
    & $gitBash 3ds/build.sh
    if ($LASTEXITCODE -ne 0) { throw "build failed (exit $LASTEXITCODE)" }
}

# Runs one target, never throws - failures are captured and reported, not fatal to the others.
# Write-Host for banners, same reason as Push-Parallel: the call site is "$results = @{ k = Push-One
# $t }", and a function's Write-Output stream (not just `return`) becomes that value - mixing the
# two would make $results[$t] a non-empty array (banner strings + the real $true/$false), which is
# always truthy in PowerShell regardless of which boolean was actually returned. That means a real
# failure here could have silently reported as success. Not yet observed, caught while fixing the
# equivalent (observed, confirmed) bug in Push-Parallel.
function Push-One([string]$t) {
    Write-Host "=== push $t ($($consoleName[$t])) ==="
    try {
        if ($t -eq 'a') {
            $azArgs = @($Rest)
            if ($GameDebug) { $azArgs = @('-Debug', $GameDebug) + $azArgs }
            if ($Interactive) { $azArgs = @('-Interactive') + $azArgs }
            & powershell -NoProfile -File 3ds/run_azahar.ps1 @azArgs
            if ($LASTEXITCODE -ne 0) { throw "run_azahar.ps1 exited $LASTEXITCODE" }
        } else {
            $targetIp = if ($ip) { $ip } else { $defaultIp[$t] }
            $env:N3DS_IP = $targetIp
            $watchSecs = if ($Interactive) { 86400 } else { $Seconds }
            $extra = @($Rest); if ($GameDebug) { $extra += $GameDebug }
            & $gitBash 3ds/push_3ds.sh -t $watchSecs @extra
            if ($LASTEXITCODE -ne 0) { throw "push_3ds.sh exited $LASTEXITCODE" }
        }
        Write-Host "=== push ${t}: done ==="
        return $true
    } catch {
        Write-Host "=== push ${t}: FAILED - $($_.Exception.Message) ==="
        return $false
    }
}

# Multiple targets push at the same time (background jobs, not sequential) - each line a job
# prints is echoed live, tagged "[t] ", as soon as it arrives, interleaved with the others. A job
# never throws out of this function: a sentinel line ("GO_PS1_RESULT:...") carries its outcome, so
# one target failing never stops or hides the others.
function Push-Parallel([string[]]$targets) {
    $jobs = @{}
    foreach ($t in $targets) {
        $targetIp = if ($ip) { $ip } else { $defaultIp[$t] }
        $jobs[$t] = Start-Job -ScriptBlock {
            param($t, $repo, $targetIp, $pythonExe, $gitBashPath, $interactive, $seconds, $rest, $gameDebug)
            Set-Location $repo
            if ($pythonExe) { $env:PYTHON_EXE = $pythonExe }
            Write-Output "=== push $t starting ==="
            try {
                if ($t -eq 'a') {
                    $azArgs = @($rest)
                    if ($gameDebug) { $azArgs = @('-Debug', $gameDebug) + $azArgs }
                    if ($interactive) { $azArgs = @('-Interactive') + $azArgs }
                    & powershell -NoProfile -File 3ds/run_azahar.ps1 @azArgs 2>&1
                    if ($LASTEXITCODE -ne 0) { throw "run_azahar.ps1 exited $LASTEXITCODE" }
                } else {
                    $env:N3DS_IP = $targetIp
                    $watchSecs = if ($interactive) { 86400 } else { $seconds }
                    $extra = @($rest); if ($gameDebug) { $extra += $gameDebug }
                    & $gitBashPath 3ds/push_3ds.sh -t $watchSecs @extra 2>&1
                    if ($LASTEXITCODE -ne 0) { throw "push_3ds.sh exited $LASTEXITCODE" }
                }
                Write-Output "GO_PS1_RESULT:OK"
            } catch {
                Write-Output "GO_PS1_RESULT:FAILED:$($_.Exception.Message)"
            }
        } -ArgumentList $t, $repo, $targetIp, $pythonExe, $gitBash, $Interactive.IsPresent, $Seconds, $Rest, $GameDebug
    }

    # Write-Host, not Write-Output, for every progress line in here: this function's return value
    # is "$results = Push-Parallel $targets" in the caller, and in PowerShell a function's entire
    # Write-Output stream - not just its `return` - becomes that assignment. Using Write-Output for
    # live progress here silently swallowed every line into $results instead of the console,
    # which also corrupted $results itself (a mix of strings and the hashtable, not a clean
    # hashtable) - the exact bug that made a fully successful push on report FAILED for both
    # targets. Confirmed and fixed 2026-10-07, not a guess: the live echo was completely absent
    # (6 lines total over a 90+ second real-hardware run) and the summary was wrong despite both
    # targets' capture files being complete and correct.
    $results = @{}
    while (($jobs.Values | Where-Object { $_.State -eq 'Running' }).Count -gt 0) {
        foreach ($t in $jobs.Keys) {
            Receive-Job $jobs[$t] | ForEach-Object {
                if ($_ -match '^GO_PS1_RESULT:OK$') { $results[$t] = $true }
                elseif ($_ -match '^GO_PS1_RESULT:FAILED:(.*)$') { $results[$t] = $false; Write-Host "[$t] FAILED - $($Matches[1])" }
                else { Write-Host "[$t] $_" }
            }
        }
        Start-Sleep -Milliseconds 300
    }
    # Drain anything written between the last poll and the job finishing.
    foreach ($t in $jobs.Keys) {
        Receive-Job $jobs[$t] | ForEach-Object {
            if ($_ -match '^GO_PS1_RESULT:OK$') { $results[$t] = $true }
            elseif ($_ -match '^GO_PS1_RESULT:FAILED:(.*)$') { $results[$t] = $false; Write-Host "[$t] FAILED - $($Matches[1])" }
            else { Write-Host "[$t] $_" }
        }
        if (-not $results.ContainsKey($t)) { $results[$t] = $false; Write-Host "[$t] FAILED - job ended with no result (job state: $($jobs[$t].State))" }
        Remove-Job $jobs[$t] -Force
    }
    return $results
}

function Pull-One([string]$t) {
    $targetIp = if ($ip) { $ip } else { $defaultIp[$t] }
    Write-Output "--- pulling from $($consoleName[$t]) at $targetIp (FTP 5000) ---"
    if ($pythonExe) { & $pythonExe -3 -u 3ds/tools/ftppull.py $targetIp $t build3ds }
    else { & py -3 -u 3ds/tools/ftppull.py $targetIp $t build3ds }
    exit $LASTEXITCODE
}

if ($Verb -eq 'push') {
    Invoke-Build
    $targets = if ($multi.ContainsKey($Target)) { $multi[$Target] } else { @($Target) }
    if ($targets.Count -gt 1) {
        Write-Output "--- pushing to $($targets.Count) targets at the same time ---"
        $results = Push-Parallel $targets
        Write-Output "--- summary ---"
        foreach ($t in $targets) { Write-Output "  $t ($($consoleName[$t])): $(if ($results[$t]) { 'OK' } else { 'FAILED' })" }
    } else {
        $results = @{ $targets[0] = Push-One $targets[0] }
    }
    exit $(if (($results.Values | Where-Object { -not $_ }).Count -gt 0) { 1 } else { 0 })
}

# pull
if ($Target -eq 'a') {
    Write-Output "pull a: not applicable - Azahar's SD card is local (see build3ds/last_log.txt)"
    exit 1
}
Pull-One $Target
