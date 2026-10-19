<#
run_azahar.ps1 - run build3ds/pm_3ds.3dsx in Azahar, wait for a log line, then close Azahar.

  powershell -File 3ds/run_azahar.ps1 [-Until <regex>] [-Timeout <s>] [-Capture] [-Gdb] [-FaultStop]

Stops at the first of: a log line that matches -Until, a [CRASH] line, an Azahar CPU
exception dialog, Azahar exit,
no game log output for -Stall seconds, or -Timeout seconds. This script's own Azahar instance is always
force-killed at the end (by PID, never other azahar.exe processes - safe to run alongside another
concurrent instance), so this script cannot hang.
Output: build3ds/last_log.txt (game log), build3ds/last_shot.png (-Capture), and
[CRASH] addresses resolved to source lines (needs Docker and the pm3ds-work volume).
-Gdb: start Azahar with its GDB stub (port 24689) and attach arm-none-eabi-gdb (Docker) in
batch mode. gdb runs -GdbScript (gdb commands, e.g. "break boot_main"), then "continue".
When the game stops (breakpoint or fault), gdb prints a backtrace, the registers and the stack.
Output: build3ds/last_gdb.txt. The run ends when gdb exits or at -Timeout. Log-silence
detection is off, because a stopped game is silent.
-Turbo: (slice W01) raise Azahar's speed limit for this run only: frame_limit=1000 (percent) and use_vsync=false in
qt-config.ini. The four original lines are saved in build3ds/qt-config.turbo-backup and put back at the end (also after a
killed run: the next start restores first). The game then runs as fast as the PC allows. Game logic is frame driven, so the same
scans give the same maps. NOT valid for performance numbers: ms per frame, phase times, fps and the audio budget (the audio
thread works against real time) all change. -Timeout and -Until keep their normal-speed meaning: the run waits Timeout/TurboFactor s.
-TurboFactor <n> (default 3): the assumed speedup. The script prints the wall time, and the game log has t= in ms.
-Smoke: warp check. Adds the game switch `smoke`, waits for the log line "smoke done" (300 frames after the first frame in the
scene, needs warp, story or preset in -Debug), exit status 0 when it appears and no unmapped access was seen.
-FaultStop: stop at the first "UnmappedAccess" line in Azahar's log (Memory class, Info level, already on).
Every run writes build3ds/last_unmapped.txt: each distinct (PC, address) with a count, or nothing. For an interactive prompt use 3ds/gdb.sh.
#>
param(
    [string]$Until = "",
    [int]$Timeout = 60,
    [int]$Stall = 10,
    [switch]$Capture,
    [switch]$Gdb,
    [switch]$Turbo, # slice W01: no frame limit for this run, see above
    [double]$TurboFactor = 3,
    [int]$TurboLimit = 1000, # Azahar speed limit in percent for -Turbo
    [switch]$Smoke, # slice W01: stop 300 frames after the first scene frame (warp), exit 0
    [switch]$FaultStop, # stop at the first unmapped memory access that Azahar logs (a data abort on hardware)
    [int]$LockWait = 900, # seconds to wait for the Azahar lock held by another run
    [switch]$Interactive, # a person plays: no timeout, no log-silence stop, settings windows allowed
    [string]$GdbScript = "",
    [switch]$Milestone, # allows tour= with more than 40 maps (budget rule)
    [switch]$Force, # run an identical rerun anyway (counts double in the budget)
    [string]$ResetBudget = "", # orchestrator only: reset the run count of one row, then exit
    [string]$Debug = ""  # render debug switches for the game, see pm_main.c (nofog nolight notex texonly logtev)
)
$ErrorActionPreference = "Stop"
if ($Interactive) { $Timeout = [int]::MaxValue }
$repo = Split-Path -Parent $PSScriptRoot
$sd = "$env:APPDATA\Azahar\sdmc\3ds\PaperMario"
$cfg = "$env:APPDATA\Azahar\config\qt-config.ini"
$exe = "C:\Program Files\Azahar\azahar.exe"
$log = "$sd\log.txt"
$emuLog = "$env:APPDATA\Azahar\log\azahar_log.txt"
$out = "$repo\build3ds"
$vol = "pm3ds-work"
$x3dsx = "pm_3ds.3dsx"
if ($env:PM_LANE) { $out = "$repo\build3ds\lane-$env:PM_LANE"; $vol = "pm3ds-work-$env:PM_LANE"; $x3dsx = "pm_3ds_$($env:PM_LANE).3dsx" }

# Test budget (3ds/logs/test_budget.csv): columns time,row,sha256,args,kind,rc,result. kind = start or end.
$budgetFile = "$repo\3ds\logs\test_budget.csv"
if ($ResetBudget) {
    if (Test-Path $budgetFile) { $keep = @(Get-Content $budgetFile | Where-Object { $_.Split(",")[1] -ne $ResetBudget }); Set-Content $budgetFile $keep }
    Write-Output "Budget of row $ResetBudget reset."; exit 0
}
$row = if ($env:PM_ROW) { $env:PM_ROW } else { "adhoc" }
$max = if ($env:PM_BUDGET) { [int]$env:PM_BUDGET } elseif ($row -eq "adhoc") { 12 } else { 4 }
if ($row -eq "adhoc") { Write-Output "WARNING: PM_ROW is not set. Set PM_ROW to your row id. Runs count under row adhoc (12 per 3 hours)." }
$sha = (Get-FileHash "$out\pm_3ds.3dsx" -Algorithm SHA256).Hash.ToLower()
$argStr = (($PSBoundParameters.GetEnumerator() | Where-Object { $_.Key -notin "Force", "ResetBudget", "Milestone" } | Sort-Object Key |
    ForEach-Object { "$($_.Key)=$($_.Value)" }) -join " ").Replace(",", ";")
$budgetRows = @(if (Test-Path $budgetFile) { Get-Content $budgetFile | Where-Object { $_ } | ForEach-Object { $f = $_.Split(","); [pscustomobject]@{ t = [datetime]$f[0]; row = $f[1]; sha = $f[2]; args = $f[3]; kind = $f[4]; rc = $f[5]; res = ($f[6..($f.Count - 1)] -join ",") } } })
$used = @($budgetRows | Where-Object { $_.row -eq $row -and $_.kind -eq "start" -and ($row -ne "adhoc" -or $_.t -gt (Get-Date).AddHours(-3)) }).Count
if ($used -ge $max) { Write-Output "BUDGET: row $row used $used runs. Stop and report what blocks you."; exit 9 }
if ($Debug -and $Debug.Length -gt 255) { Write-Output "SWITCHES: -Debug is $($Debug.Length) characters. The game reads 255 and cuts the rest, so a map name of a tour list is cut (H06d). Use tour=@file for a long list."; exit 8 }
if ($Debug -match "tour=(\S+)") {
    $tv = $Matches[1]
    if ($tv.StartsWith("@")) { $tv = if (Test-Path "$sd\$($tv.Substring(1))") { (Get-Content "$sd\$($tv.Substring(1))" -Raw) -replace "\s+", "," } else { "" } }
    $nmaps = @($tv.Split(",") | Where-Object { $_ }).Count
    if ($nmaps -gt 40 -and -not $Milestone) { Write-Output "TOUR: $nmaps maps is more than 40. A tour this long needs -Milestone. Tour only the touched maps."; exit 8 }
}
if (-not $Force) {
    $old = $budgetRows | Where-Object { $_.row -eq $row -and $_.sha -eq $sha -and $_.args -eq $argStr -and $_.kind -eq "end" -and $_.rc -eq "0" } | Select-Object -Last 1
    if ($old) { Write-Output "cached ok (same row, 3dsx and args, rc 0): $($old.res)"; Write-Output "budget: row $row run $used/$max (not run, use -Force to rerun)"; exit 0 }
}
New-Item -ItemType Directory -Force -Path (Split-Path $budgetFile) | Out-Null
$stamp = (Get-Date).ToString("yyyy-MM-ddTHH:mm:ss")
$nStart = if ($Force) { 2 } else { 1 }
1..$nStart | ForEach-Object { Add-Content $budgetFile "$stamp,$row,$sha,$argStr,start,," }
$used += $nStart
function End-Run([int]$rc, [string]$res) {
    Add-Content $budgetFile "$((Get-Date).ToString('yyyy-MM-ddTHH:mm:ss')),$row,$sha,$argStr,end,$rc,$($res.Replace(',', ';'))"
    Write-Output "budget: row $row run $used/$max"
    exit $rc
}

function Kill-Azahar([int]$procId) {
    # Scoped to the PID this script itself launched - never kills other azahar.exe processes (a
    # concurrent agent/session may have its own instance open; a blanket "taskkill /IM azahar.exe"
    # here would silently terminate theirs too).
    Stop-Process -Id $procId -Force -ErrorAction SilentlyContinue
    for ($i = 0; $i -lt 50 -and (Get-Process -Id $procId -ErrorAction SilentlyContinue); $i++) {
        Start-Sleep -Milliseconds 100
    }
}

# Last $max bytes of a file that Azahar keeps open (the emulator log can reach 100 MB)
function Read-Tail([string]$path, [int]$max = 65536) {
    try {
        $fs = [IO.File]::Open($path, "Open", "Read", "ReadWrite")
        if ($fs.Length -gt $max) { $fs.Seek(-$max, "End") | Out-Null }
        $sr = New-Object IO.StreamReader($fs)
        $t = $sr.ReadToEnd()
        $sr.Close()
        return $t
    } catch { return "" }
}

# T02. Azahar reads only %APPDATA%\Azahar\config\qt-config.ini and takes no config argument, so a per-run copy
# cannot be used. Instead: one named mutex serialises all runs, the real file is saved byte for byte before the
# first edit and put back byte for byte in finally. A hard kill skips finally: the next run puts the file back
# from the saved copy (and kills the orphan Azahar) before it does anything.
$cfgBak = "$env:TEMP\pm_azahar_qt-config.orig"
$pidFile = "$env:TEMP\pm_azahar_run.pid"
function Restore-Cfg {
    if (Test-Path $cfgBak) {
        Copy-Item $cfgBak $cfg -Force
        Remove-Item $cfgBak -Force
    }
}

# Replace "key=value" lines. A missing key stops the run (no silent skip). $insertAfter: section header to add a missing key under.
function Set-CfgKeys([hashtable]$kv, [string]$insertAfter = "") {
    $text = Get-Content $cfg -Raw
    foreach ($k in $kv.Keys) {
        $re = "(?m)^" + [regex]::Escape($k) + "=.*$"
        $line = $k + "=" + $kv[$k]
        if ($text -match $re) { $text = $text -replace $re, $line.Replace('$', '$$') }
        elseif ($insertAfter -and $text -match "(?m)^\[$insertAfter\]\r?$") { $text = $text -replace "(?m)^\[$insertAfter\]\r?$", "[$insertAfter]`r`n$($line.Replace('$', '$$'))" }
        else { throw "qt-config.ini: key '$k' not found in $cfg. Run stopped, config restored." }
    }
    Set-Content $cfg $text -NoNewline
}
function Set-GdbStub([bool]$on) {
    $v = if ($on) { "true" } else { "false" }
    # instant_debug_log: Azahar flushes each log line, so an exception dump shows at once
    Set-CfgKeys ([ordered]@{ 'use_gdbstub\default' = "false"; "use_gdbstub" = $v; 'gdbstub_port\default' = "false"; "gdbstub_port" = "24689";
                             'instant_debug_log\default' = "false"; "instant_debug_log" = "true" }) "Debugging"
}
function Set-Turbo {
    Set-CfgKeys ([ordered]@{ 'frame_limit\default' = "false"; "frame_limit" = "$TurboLimit"; 'use_vsync\default' = "false"; "use_vsync" = "false" })
}

if ($Smoke) { $Until = "smoke done"; $Debug = ("$Debug smoke").Trim() }
if ($Turbo) {
    $Debug = ("$Debug turbo").Trim()
    $Timeout = [Math]::Max(25, [int]($Timeout / $TurboFactor))
    $Stall = [Math]::Max(10, [int]($Stall / $TurboFactor))
    if ($Until -or $Smoke) { Write-Output "TURBO run: timeout $Timeout s. Not valid for performance numbers." }
}

# Named mutex: one Azahar run at a time on this PC (all trees, all agents)
$p = $null
$gdbProc = $null
$mutex = New-Object Threading.Mutex($false, "Global\pm_azahar_run")
$held = $false
# The lock covers the later reads of the shared game log and emulator log too: released after the last read (Release-Lock below)
function Release-Lock { if ($script:held) { $script:held = $false; $mutex.ReleaseMutex() } }
trap { Release-Lock; break }
try {
    if (-not $mutex.WaitOne(0)) {
        Write-Output "Another Azahar run holds the lock (Global\pm_azahar_run). Waiting up to $LockWait s..."
        $held = $mutex.WaitOne([int]($LockWait * 1000))
        if (-not $held) { Write-Output "LOCK TIMEOUT: no free slot after $LockWait s. Nothing was run."; exit 3 }
        Write-Output "Lock acquired."
    } else { $held = $true }
} catch [Threading.AbandonedMutexException] { $held = $true; Write-Output "Lock was abandoned by a killed run. Recovering." }
# From here this script owns the lock. Any earlier run that was killed: kill its Azahar, put the config back.
if (Test-Path $pidFile) {
    $old = [int](Get-Content $pidFile -ErrorAction SilentlyContinue)
    $op = Get-Process -Id $old -ErrorAction SilentlyContinue
    if ($op -and $op.ProcessName -eq "azahar") { Write-Output "Recovery: killing orphan Azahar $old."; Kill-Azahar $old }
    Remove-Item $pidFile -ErrorAction SilentlyContinue
}
if (Test-Path $cfgBak) { Write-Output "Recovery: restoring qt-config.ini saved by a killed run."; Restore-Cfg }
try {
    Set-Content $pidFile $PID # the script PID until Azahar starts, then the Azahar PID
    Copy-Item $cfg $cfgBak -Force # every run edits the config (Set-GdbStub), so every run saves it first
    if ($Turbo) { Set-Turbo }

New-Item -ItemType Directory -Force -Path $sd | Out-Null
Copy-Item "$out\pm_3ds.3dsx" "$sd\$x3dsx" -Force
Remove-Item $log -ErrorAction SilentlyContinue
if ($Debug) { Set-Content "$sd\debug3ds.txt" $Debug } else { Remove-Item "$sd\debug3ds.txt" -ErrorAction SilentlyContinue }
Remove-Item "$sd\shots", "$out\shots" -Recurse -ErrorAction SilentlyContinue # "shots" switch output
Remove-Item $emuLog -ErrorAction SilentlyContinue # an old exception dump must not stop this run
Set-GdbStub $Gdb.IsPresent

# Visible top-level windows of a process: Azahar's error dialog is a second one
Add-Type @"
using System; using System.Runtime.InteropServices;
public class AzEnum {
  delegate bool CB(IntPtr h, IntPtr p);
  [DllImport("user32.dll")] static extern bool EnumWindows(CB cb, IntPtr p);
  [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll", CharSet = CharSet.Unicode)] static extern int GetWindowText(IntPtr h, System.Text.StringBuilder s, int n);
  public static string Titles = "";
  public static int Count(uint pid) {
    int n = 0;
    var t = new System.Text.StringBuilder();
    EnumWindows((h, p) => {
      uint q; GetWindowThreadProcessId(h, out q);
      if (q == pid && IsWindowVisible(h)) {
        n++;
        var sb = new System.Text.StringBuilder(256); GetWindowText(h, sb, 256);
        t.Append("'" + sb.ToString() + "' ");
      }
      return true;
    }, IntPtr.Zero);
    Titles = t.ToString();
    return n;
  }
}
"@

$p = Start-Process $exe -ArgumentList "`"$sd\$x3dsx`"" -PassThru
Set-Content $pidFile $p.Id
if ($Gdb) {
    # Azahar waits for gdb before it runs the game. winhost = the Windows host (IPv4) seen from Docker.
    $cmds = @("set pagination off", "set confirm off", "set width 0", "target remote winhost:24689")
    if ($GdbScript) { $cmds += Get-Content $GdbScript }
    $cmds += @("continue", "echo \n--- stopped ---\n", "bt 30", "info registers", 'x/32wx $sp', "kill")
    Set-Content "$out\gdb_cmds.txt" $cmds
    Start-Sleep -Seconds 2
    $gdbArgs = "run --rm --add-host=winhost:host-gateway -v ${vol}:/work -v `"${out}:/out`" " +
               "devkitpro/devkitarm:latest /opt/devkitpro/devkitARM/bin/arm-none-eabi-gdb -q -batch " +
               "-x /out/gdb_cmds.txt /work/build/pm_3ds.elf"
    $gdbProc = Start-Process docker -ArgumentList $gdbArgs -NoNewWindow -PassThru `
        -RedirectStandardOutput "$out\last_gdb.txt" -RedirectStandardError "$out\last_gdb_err.txt"
}
$start = Get-Date
$reason = "timeout ($Timeout s)"
$lastSize = -1
$lastGrow = Get-Date
$echoed = 0
$marks = @()
    while (((Get-Date) - $start).TotalSeconds -lt $Timeout) {
        Start-Sleep -Milliseconds 500
        if ($p.HasExited) { $reason = "Azahar exited"; break }
        if ($gdbProc -and $gdbProc.HasExited) { $reason = "gdb finished"; break }
        # Azahar shows a dialog on a guest CPU exception and the game stops: kill at once
        if (-not $Interactive -and [AzEnum]::Count([uint32]$p.Id) -gt 1) { Start-Sleep -Seconds 4; $reason = "Azahar dialog (exception or error), windows: $([AzEnum]::Titles)"; break }
        if ($FaultStop -and (Read-Tail $emuLog) -match "UnmappedAccess") { Start-Sleep -Seconds 1; $reason = "unmapped access"; break }
        if ((Read-Tail $emuLog) -match "Exception Type:") { Start-Sleep -Seconds 4; $reason = "emulator exception"; break }
        if (-not (Test-Path $log)) {
            # No game log yet: a modal dialog (usage, first-run, error) or a hang before the first frame stops an unattended run
            if (-not $Gdb -and -not $Interactive -and ((Get-Date) - $start).TotalSeconds -gt ($Stall + 30)) { $reason = "modal dialog suspected: no game log $($Stall + 30) s after the start (windows: $([AzEnum]::Titles))"; break }
            continue
        }
        # The game logs at least once per second; silence means a hang or an exception dialog
        $size = (Get-Item $log).Length
        $text0 = Get-Content $log -Raw -ErrorAction SilentlyContinue
        if ($size -ne $lastSize) { $lastSize = $size; $lastGrow = Get-Date }
        elseif (-not $Gdb -and -not $Interactive -and ((Get-Date) - $lastGrow).TotalSeconds -gt $Stall) { $reason = "game log silent for $Stall s" + $(if ($text0 -notmatch "game mode") { " before the first frame: modal dialog suspected" } else { "" }); break }
        $text = Get-Content $log -Raw -ErrorAction SilentlyContinue
        if (-not $text) { continue }
        # W01: wall seconds at each map load, scene start and smoke end (for normal versus turbo comparisons)
        $mk = [regex]::Matches($text, "(?m)^#\d+ (load map area \d+ map \d+ scan \d+|warp: scene frame0.*|warp: smoke done.*)")
        for ($mi = $marks.Count; $mi -lt $mk.Count; $mi++) { $marks += ("{0,6:N1} s  {1}" -f ((Get-Date) - $start).TotalSeconds, $mk[$mi].Groups[1].Value) }
        if ($text.Length -gt $echoed) { Write-Host -NoNewline $text.Substring($echoed); $echoed = $text.Length }
        if ($text -match "\[CRASH\]") { Start-Sleep -Seconds 1; $reason = "crash"; break }
        if ($Until -and $text -match $Until) { $reason = "matched '$Until'"; break }
    }
    if ($Capture -and -not $p.HasExited) {
        Add-Type -AssemblyName System.Drawing
        Add-Type @"
using System; using System.Runtime.InteropServices;
public class AzWin {
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint f);
  public struct RECT { public int L, T, R, B; }
}
"@
        $p.Refresh()
        $r = New-Object AzWin+RECT
        [AzWin]::GetWindowRect($p.MainWindowHandle, [ref]$r) | Out-Null
        $bmp = New-Object System.Drawing.Bitmap ($r.R - $r.L), ($r.B - $r.T)
        $g = [System.Drawing.Graphics]::FromImage($bmp)
        $hdc = $g.GetHdc()
        [AzWin]::PrintWindow($p.MainWindowHandle, $hdc, 2) | Out-Null
        $g.ReleaseHdc($hdc)
        $bmp.Save("$out\last_shot.png")
        $g.Dispose(); $bmp.Dispose()
    }
} finally {
    if ($p) { Kill-Azahar $p.Id }
    if ($gdbProc -and -not $gdbProc.HasExited) { Stop-Process -Id $gdbProc.Id -Force -ErrorAction SilentlyContinue }
    Restore-Cfg # byte for byte copy of the real config
    Remove-Item $pidFile -ErrorAction SilentlyContinue
    # A later manual launch must not inherit this run's switches (autoa, shots, fast).
    Remove-Item "$sd\debug3ds.txt" -ErrorAction SilentlyContinue
}

if (Test-Path "$sd\shots") { Copy-Item "$sd\shots" "$out\shots" -Recurse -Force }
$secs = [int]((Get-Date) - $start).TotalSeconds
Write-Output "stopped after $secs s: $reason"
if ($marks.Count) { Write-Output "--- wall time at map loads (run start = 0) ---"; $marks | ForEach-Object { Write-Output $_ } }
if ($Turbo) { Write-Output "TURBO run: times and fps in this log are not valid for performance measurements" }
if ($Gdb) {
    Write-Output "--- gdb (build3ds/last_gdb.txt) ---"
    Get-Content "$out\last_gdb.txt", "$out\last_gdb_err.txt" -ErrorAction SilentlyContinue | Select-Object -Last 80
}
if (Test-Path $log) {
    # Azahar writes SD files late: copy when the size has not changed for 1 s (max 6 s)
    $prev = -1
    for ($k = 0; $k -lt 20; $k++) {
        $cur = (Get-Item $log).Length
        if ($cur -eq $prev -and $k -ge 3) { break }
        $prev = $cur
        Start-Sleep -Milliseconds 300
    }
    Copy-Item $log "$out\last_log.txt" -Force
    Get-Content "$out\last_log.txt" | Select-Object -Last 15
} else {
    Write-Output "no log.txt written"
}

# Crash addresses: the game's [CRASH] lines (hardware) or Azahar's exception dump (emulator)
$addrs = @()
$lines = if (Test-Path "$out\last_log.txt") { Get-Content "$out\last_log.txt" } else { @() }
foreach ($l in ($lines | Where-Object { $_ -match "^\[CRASH\]" -and $_ -match "pc=|stack" })) {
    $addrs += [regex]::Matches($l, "(?:pc=|lr=| )([0-9a-f]{8})") | ForEach-Object { $_.Groups[1].Value }
}
$emu = ""
for ($k = 0; $k -lt 10 -and $emu -notmatch "Exception Type:"; $k++) { # the log can stay locked briefly after the kill
    $emu = Read-Tail $emuLog 262144
    if ($emu -notmatch "Exception Type:") { Start-Sleep -Milliseconds 300 }
}
$i = $emu.LastIndexOf("Exception Type:") # Azahar repeats the dump: keep the last one
if ($i -ge 0) {
    $block = $emu.Substring([Math]::Max(0, $emu.LastIndexOf("Thread:", $i)))
    Set-Content "$out\last_exception.txt" $block
    Write-Output "--- Azahar exception (build3ds/last_exception.txt) ---"
    ($block -split "`n") | Where-Object { $_ -match "Exception Type|PC |LR " } | ForEach-Object { $_.Trim() }
    foreach ($m in [regex]::Matches($block, "(?:LR|PC)\s+0x([0-9A-Fa-f]{8})")) { $addrs += $m.Groups[1].Value }
    foreach ($m in [regex]::Matches($block, "(?m)^\s*0x[0-9A-Fa-f]{8}:((?:\s+[0-9A-Fa-f]{2})+)")) {
        $b = ($m.Groups[1].Value.Trim() -split "\s+")
        for ($k = 0; $k + 3 -lt $b.Count; $k += 4) { $addrs += ($b[$k + 3] + $b[$k + 2] + $b[$k + 1] + $b[$k]) }
    }
}
# Unmapped accesses: Azahar returns 0 and continues; real hardware faults. Report the top sites.
$bad = @{}
$badFull = @{}
try {
    $sr = New-Object IO.StreamReader([IO.File]::Open($emuLog, "Open", "Read", "ReadWrite"))
    while (($line = $sr.ReadLine()) -ne $null) {
        if ($line -match "UnmappedAccess.* at PC 0x([0-9A-F]{8})") {
            $bad[$Matches[1]] = 1 + [int]$bad[$Matches[1]]
            $key = ($line -replace "^\[[^\]]*\]\s*", "") -replace "^\S+\s+<\w+>\s*", ""
            $badFull[$key] = 1 + [int]$badFull[$key]
        }
    }
    $sr.Close()
} catch {}
Release-Lock # the last read of the shared game and emulator logs is done
Set-Content "$out\last_unmapped.txt" ($badFull.GetEnumerator() | Sort-Object Value -Descending | ForEach-Object { "{0,8}x {1}" -f $_.Value, $_.Key })
if ($bad.Count) {
    $top = $bad.GetEnumerator() | Sort-Object Value -Descending | Select-Object -First 3
    Write-Output "--- WARNING: unmapped memory accesses (a fault on hardware) ---"
    foreach ($t in $top) { Write-Output ("{0,8}x at PC 0x{1}" -f $t.Value, $t.Key) }
    $addrs += $top | ForEach-Object { $_.Key }
}
$addrs = $addrs | ForEach-Object { [Convert]::ToUInt32($_, 16) } |
    Where-Object { $_ -ge 0x100000 -and $_ -lt 0x1000000 } | ForEach-Object { "0x{0:x8}" -f $_ } | Select-Object -Unique
if ($addrs) {
    Write-Output "--- crash symbols (pc, lr, then stack words that are code) ---"
    $env:MSYS_NO_PATHCONV = "1"
    & docker run --rm -v ${vol}:/work devkitpro/devkitarm:latest sh -c "`$DEVKITARM/bin/arm-none-eabi-addr2line -f -C -p -e /work/build/pm_3ds.elf $($addrs -join ' ')" |
        Where-Object { $_ -notmatch "^\?\?" }
    End-Run 2 "crash symbols; stopped: $reason"
}
if ($Smoke -and $reason -notmatch "^matched") { Write-Output "SMOKE FAILED: $reason"; End-Run 1 "smoke failed: $reason" }
End-Run 0 "stopped after ${secs}s: $reason"
