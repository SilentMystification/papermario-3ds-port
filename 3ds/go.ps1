<#
go.ps1 - single entry point for pushing to and pulling from O3DS, N3DS, or Azahar.

  powershell -File 3ds/go.ps1 push o|n|a|on|ona [-ip <addr>] [-NoBuild] [-Seconds <n>] [-Interactive]
                              [-GameDebug "switch1 switch2"] [-Profile prof|log|none] [extra args]
  powershell -File 3ds/go.ps1 pull o|n          [-ip <addr>]

push o: netload to O3DS, live-echo the game log for -Seconds (default 60).
push n: same, N3DS.
-Profile prof|log|none: the named switch words of bench_defs.json "profiles" (prof = profiling on, log = profiling off,
  debug log), sent after the game args on o/n (push_3ds.sh --profile). Game args given by hand win (first match).
  Not used by push a.
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
-IdleSeconds <n> (push o/n, default 300): the log echo also ends when no complete line came for n seconds (a console
  that is off or sits at the HOME menu). -Seconds / -Interactive stay the hard limit. push_3ds.sh --idle.
Stop: the echo and the netload tail end with this script (push_3ds.sh watches its owner pids); a push that ends
  or is stopped while push_3ds.sh runs has its bash processes stopped in a finally block. A send in progress is never killed.
  Either way, closing this script does not stop the game - run_azahar.ps1 kills only the Azahar
  process it started (other azahar.exe instances keep running), and real hardware keeps running
  regardless of whether this PC script is watching it.

pull o: FTP (port 5000) into O3DS's SD card. Downloads the game log, crash.txt and any Luma crash dumps to
  build3ds/hw_o3ds_log.txt, build3ds/hw_o3ds_crash.txt and build3ds/hw_o3ds_dumps/.
pull n: same, N3DS.
Without FTP: python 3ds/tools/pm_logrecv.py --ip <addr> --label o3ds (game running, live log on).

No default IP. push o/n without -ip takes the console address from the netloader broadcast (push_3ds.sh).
pull o/n needs -ip. push on/ona is refused: two consoles give one broadcast answer, so push o and push n one
at a time (each with -ip <addr> when more than one console is on the network).
One attempt, short timeouts - this never waits indefinitely on a console (see the project's
never-block-on-live-device rule) unless -Interactive is explicitly given. push o/n/on/ona and
pull o/n touch real hardware; push a does not.
#>
param(
    [Parameter(Position = 0, Mandatory = $true)][ValidateSet('push', 'pull')] [string]$Verb,
    [Parameter(Position = 1, Mandatory = $true)][ValidateSet('o', 'n', 'a', 'on', 'ona')] [string]$Target,
    [string]$ip = "",
    [int]$Seconds = 60,
    # push o/n: the log echo ends after this many seconds with no complete line (push_3ds.sh --idle).
    [int]$IdleSeconds = 300,
    [switch]$NoBuild,
    [switch]$Interactive,
    # MEMGATE: push an O3DS-applet memory OVER build anyway (push_3ds.sh --mem-ok; the override is logged in pushed_builds.txt).
    [switch]$MemOk,
    # PUSHFIX: a push runs detached by default (a child powershell with its whole output in build3ds/out/push_<Target>_<stamp>.txt);
    # -Fg runs it in this process (the child). -Interactive stays in the foreground.
    [switch]$Fg,
    # Render/game debug switches (pm_main.c read_debug_switches: nofog nolight notex ... perfctr
    # calls), space-separated in one string. NOT named -Debug: PowerShell gives every script with a
    # [Parameter()] attribute an automatic common -Debug switch, and a script cannot redeclare a
    # parameter with that name ("A parameter with the name 'Debug' was defined multiple times" -
    # confirmed directly). Any -Debug "..." on this command line is silently eaten by that common
    # parameter, never reaches $Rest, and the game never sees it - this is why, not a typo.
    [string]$GameDebug = "",
    [ValidateSet('', 'prof', 'log', 'none')] [string]$Profile = "",
    [Parameter(ValueFromRemainingArguments = $true)] [string[]]$Rest
)
$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $PSScriptRoot
Set-Location $repo

$consoleName = @{ o = "O3DS"; n = "N3DS"; a = "Azahar" }
$multi = @{ on = @('o', 'n'); ona = @('o', 'n', 'a') }

if ($Verb -eq 'pull' -and $multi.ContainsKey($Target)) {
    Write-Output "pull ${Target}: not supported - pull one console at a time (pull o / pull n)"
    exit 1
}
if ($Verb -eq 'pull' -and -not $ip) {
    Write-Output "pull ${Target}: needs -ip <address> (no default IP)"
    exit 1
}
if ($multi.ContainsKey($Target) -and ($multi[$Target] | Where-Object { $_ -ne 'a' })) {
    Write-Output "push ${Target}: refused - two hardware consoles cannot be told apart by the netloader broadcast. Run push o and push n one at a time."
    exit 1
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
    # build.sh is the only stale check (content key, BSTALE): it prints "build up to date" and returns when nothing changed.
    & $gitBash 3ds/build.sh
    if ($LASTEXITCODE -ne 0) { throw "build failed (exit $LASTEXITCODE)" }
}

# Runs one target, never throws - failures are captured and reported, not fatal to the others.
# Banners use Write-Host: the caller stores the return value of this function, so Write-Output lines would end up in it.
# RANDWARP: a push that names no spot (no warp=, tour=, tourlist=, preset=, loadslot=, savefile=) boots to a random spot: warp=random.
# The run log line `warp: random seed <n>` repeats it: give warp=random:<n> in the game args.
$gameSwitches = (@($GameDebug) + @($Rest)) -join ' '
$randWarp = @(); if ($gameSwitches -notmatch '(^|\s)(warp|tour|tourlist|preset|loadslot|savefile)=') { $randWarp = @('warp=random') }

# Stop-PushShell: stops the push_3ds.sh bash processes under this script (the Git Bash launcher and the bash it starts), used
# when the push ends early. Their live log and netload tail end with them (they watch their owner pids). A 3dslink upload
# already running is not killed here: it runs to its own end, the rule that a sent netload is never force-killed.
function Stop-PushShell {
    $pending = @($PID); $bashes = @()
    while ($pending.Count -gt 0) {
        $parent = $pending[0]; $pending = @($pending | Select-Object -Skip 1)
        foreach ($c in @(Get-CimInstance Win32_Process -Filter "ParentProcessId=$parent" -ErrorAction SilentlyContinue)) {
            if ($c.Name -eq 'bash.exe') { $bashes += $c.ProcessId; $pending += $c.ProcessId }
        }
    }
    foreach ($b in $bashes) { Stop-Process -Id $b -Force -ErrorAction SilentlyContinue }
}

function Push-One([string]$t) {
    Write-Host "=== push $t ($($consoleName[$t])) ==="
    try {
        if ($t -eq 'a') {
            $azArgs = @($Rest)
            $azDebug = (@($GameDebug) + $randWarp) -join ' '
            if ($azDebug.Trim()) { $azArgs = @('-Debug', $azDebug.Trim()) + $azArgs }
            if ($Interactive) { $azArgs = @('-Interactive') + $azArgs }
            & powershell -NoProfile -File 3ds/run_azahar.ps1 @azArgs
            if ($LASTEXITCODE -ne 0) { throw "run_azahar.ps1 exited $LASTEXITCODE" }
        } else {
            # No -ip: N3DS_IP stays empty and push_3ds.sh takes the netloader broadcast answer.
            $env:N3DS_IP = $ip
            $watchSecs = if ($Interactive) { 86400 } else { $Seconds }
            $extra = @($Rest); if ($GameDebug) { $extra += $GameDebug }
            $extra += $randWarp
            $profArgs = @('--idle', "$IdleSeconds"); if ($Profile) { $profArgs += @('--profile', $Profile) }
            if ($MemOk) { $profArgs += '--mem-ok' }
            # PUSHFIX: -NoBuild means no build here either (push_3ds.sh --no-build), else push_3ds.sh runs build.sh itself.
            if ($NoBuild) { $profArgs += '--no-build' }
            # PM_OWNER_PID: push_3ds.sh ends its live log and netload when this script is gone.
            $env:PM_OWNER_PID = "$PID"
            $pushDone = $false
            try {
                # PUSHFIX: every line of push_3ds.sh is printed as it comes (Out-Host), so nothing lands in this function's return value.
                & $gitBash 3ds/push_3ds.sh -t $watchSecs @profArgs @extra | Out-Host
                $pushDone = $true
            } finally {
                if (-not $pushDone) { Stop-PushShell }
            }
            if ($LASTEXITCODE -ne 0) { throw "push_3ds.sh exited $LASTEXITCODE" }
        }
        Write-Host "=== push ${t}: done ==="
        return 0
    } catch {
        Write-Host "=== push ${t}: FAILED - $($_.Exception.Message) ==="
        return 1
    }
}

function Pull-One([string]$t) {
    $targetIp = $ip
    Write-Output "--- pulling from $($consoleName[$t]) at $targetIp (FTP 5000) ---"
    if ($pythonExe) { & $pythonExe -3 -u 3ds/tools/ftppull.py $targetIp $t build3ds }
    else { & py -3 -u 3ds/tools/ftppull.py $targetIp $t build3ds }
    exit $LASTEXITCODE
}

if ($Verb -eq 'push') {
    if (-not $Fg -and -not $Interactive) {
        # PUSHFIX: detached. The same command runs again as a hidden child with -Fg; its whole output goes to a log file,
        # and this call returns at once. Blocking the orchestrator chat was the reason.
        $stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
        $logDir = Join-Path $repo 'build3ds/out'
        New-Item -ItemType Directory -Force -Path $logDir | Out-Null
        $log = Join-Path $logDir "push_${Target}_$stamp.txt"
        # Bound args back into a command line; values with spaces are quoted (Start-Process joins the array with spaces).
        $quote = { param($a) if ($a -match '\s' -or $a -eq '') { '"' + $a + '"' } else { $a } }
        $childArgs = @('-NoProfile', '-File', $PSCommandPath)
        foreach ($k in $PSBoundParameters.Keys) {
            if ($k -in @('Rest', 'Fg')) { continue }
            $v = $PSBoundParameters[$k]
            if ($v -is [System.Management.Automation.SwitchParameter] -or $v -is [bool]) { if ($v) { $childArgs += "-$k" } }
            else { $childArgs += "-$k"; $childArgs += (& $quote ([string]$v)) }
        }
        foreach ($r in @($Rest)) { if ($null -ne $r) { $childArgs += (& $quote ([string]$r)) } }
        $childArgs += '-Fg'
        # PUSHFIX second try: Start-Process passes the caller's inheritable handles to the child (CreateProcess bInheritHandles),
        # so the caller's stdout pipe stayed open until the child ended (test case d: 10 s). Win32_Process.Create inherits no handles;
        # cmd does the redirect into the same log and err files. ShowWindow 0: no console window.
        $cmdLine = 'powershell.exe ' + ($childArgs -join ' ') + ' > "' + $log + '" 2> "' + $log + '.err"'
        $startup = New-CimInstance -ClassName Win32_ProcessStartup -Property @{ ShowWindow = [uint16]0 } -ClientOnly
        $created = Invoke-CimMethod -ClassName Win32_Process -MethodName Create -Arguments @{ CommandLine = "cmd.exe /d /c `"$cmdLine`""; ProcessStartupInformation = $startup }
        if ($created.ReturnValue -ne 0) { Write-Output "push ${Target}: could not start the detached push (Win32_Process.Create $($created.ReturnValue))"; exit 1 }
        Write-Output "PUSH STARTED pid $($created.ProcessId) log $log"
        exit 0
    }
    $rc = 1
    # PUSHFIX3: a push always builds and sends the default output (build3ds/pm_3ds.3dsx), never a lane's output.
    Remove-Item Env:PM_LANE -ErrorAction SilentlyContinue
    Write-Output "push: default output build3ds/pm_3ds.3dsx"
    try {
        Invoke-Build
        # PUSHFIX: Push-One returns only its exit code (0 ok, 1 failed); the text went to the console above.
        $results = @{ $Target = Push-One $Target }
        $rc = [int]$results[$Target]
    } catch {
        Write-Output "push ${Target}: $($_.Exception.Message)"
    } finally {
        if ($Fg) {
            # PUSHFIX: the child's end line, in the log (stdout) and in build3ds/out/push_last.txt.
            $endLine = "PUSH END rc $rc $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"
            Write-Output $endLine
            New-Item -ItemType Directory -Force -Path (Join-Path $repo 'build3ds/out') | Out-Null
            Set-Content -Path (Join-Path $repo 'build3ds/out/push_last.txt') -Value $endLine
        }
    }
    exit $rc
}

# pull
if ($Target -eq 'a') {
    Write-Output "pull a: not applicable - Azahar's SD card is local (see build3ds/last_log.txt)"
    exit 1
}
Pull-One $Target
