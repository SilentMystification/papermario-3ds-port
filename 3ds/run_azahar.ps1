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
AZT-1 slots: a run takes one of N (env PM_AZ_SLOTS, default 2) private portable Azahar copies in build3ds/azslots/slot<k> (user folder
next to azahar.exe: config, nand, sdmc, logs stay in the slot, the user's install and %APPDATA% are only read). Waits only when all slots are busy.
-Slot <k> picks a slot. Outputs of the run (last_log.txt, shots, last_unmapped.txt) go to <build folder>/slot<k>/; last_log.txt is also copied to the
build folder (last writer wins). The end line in 3ds/logs/test_budget.csv starts with "slot<k>:".
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
    [switch]$Force, # run an identical rerun anyway (counts as one start, like every real Azahar start; a cached run that skips Azahar counts nothing)
    [switch]$Keep, # AZT-2: leave this slot's Azahar open (paused) after a run that did not crash or exit (AZT3-2: also a timeout or a missed until line); the next run on the slot restarts emulation in it (warm). Env PM_AZ_KEEP=1 does the same.
    [switch]$NoWarm, # AZT-2: do not restart in a kept Azahar, start a new one
    [int]$Slot = 0, # AZT-1: Azahar slot 1..N (build3ds\azslots\slot<k>); 0 = first free slot. azt.ps1 holds the slot lock and passes -Slot with PM_SLOT_HELD=1
    [string]$ResetBudget = "", # orchestrator only: reset the run count of one row, then exit
    [string]$Debug = ""  # render debug switches for the game, see pm_main.c (nofog nolight notex texonly logtev)
)
$ErrorActionPreference = "Stop"
. "$PSScriptRoot\tools\azslot.ps1"
$slotK = 0
$heldHere = $false
# The slot covers the later reads of the slot game log and emulator log too: released after the last read (Release-Lock below)
function Release-Lock { if ($script:heldHere) { $script:heldHere = $false; Exit-Slot $script:slotK } }
trap { Release-Lock; break }
if ($Interactive) { $Timeout = [int]::MaxValue }
$repo = Split-Path -Parent $PSScriptRoot
$sdMaster = "$env:APPDATA\Azahar\sdmc\3ds\PaperMario" # the user's game folder: only read (tour lists, copied to the slot)
# AZT-1: the paths below are set from the slot once it is taken (build3ds\azslots\slot<k>, portable Azahar)
$sd = $sdMaster; $cfg = ""; $exe = ""; $log = ""; $emuLog = ""
$out = "$repo\build3ds"
$vol = "pm3ds-work"
$x3dsx = "pm_3ds.3dsx"
if ($env:PM_LANE) { $out = "$repo\build3ds\lane-$env:PM_LANE"; $vol = "pm3ds-work-$env:PM_LANE"; $x3dsx = "pm_3ds_$($env:PM_LANE).3dsx" }

# Test budget (3ds/logs/test_budget.csv): columns time,row,sha256,args,kind,rc,result. kind = start or end.
$budgetFile = "$repo\3ds\logs\test_budget.csv"
if ($ResetBudget) {
    if (Test-Path $budgetFile) { $rowsLeft = @(Get-Content $budgetFile | Where-Object { $_.Split(",")[1] -ne $ResetBudget }); Set-Content $budgetFile $rowsLeft }
    Write-Output "Budget of row $ResetBudget reset."; exit 0
}
$row = if ($env:PM_ROW) { $env:PM_ROW } else { "adhoc" }
$max = if ($env:PM_BUDGET) { [int]$env:PM_BUDGET } elseif ($row -eq "adhoc") { 12 } else { 8 }
if ($row -eq "adhoc") { Write-Output "WARNING: PM_ROW is not set. Set PM_ROW to your row id. Runs count under row adhoc (12 per 3 hours)." }
$in3dsx = "$out\pm_3ds.3dsx"
$sha = (Get-FileHash "$out\pm_3ds.3dsx" -Algorithm SHA256).Hash.ToLower()
$argStr = (($PSBoundParameters.GetEnumerator() | Where-Object { $_.Key -notin "Force", "ResetBudget", "Milestone", "Slot", "Keep", "NoWarm" } | Sort-Object Key |
    ForEach-Object { "$($_.Key)=$($_.Value)" }) -join " ").Replace(",", ";")
$budgetRows = @(if (Test-Path $budgetFile) { Get-Content $budgetFile | Where-Object { $_ } | ForEach-Object { $f = $_.Split(","); [pscustomobject]@{ t = [datetime]$f[0]; row = $f[1]; sha = $f[2]; args = $f[3]; kind = $f[4]; rc = $f[5]; res = ($f[6..($f.Count - 1)] -join ",") } } })
$used = @($budgetRows | Where-Object { $_.row -eq $row -and $_.kind -eq "start" -and ($row -ne "adhoc" -or $_.t -gt (Get-Date).AddHours(-3)) }).Count
if ($used -ge $max -and $env:PM_AZT_ATTEMPT -ne "2") { Write-Output "BUDGET: row $row used $used runs. Stop and report what blocks you."; exit 9 }
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
# Several runs append at once now (one per slot): retry a sharing violation
function Add-BudgetLine([string]$line) {
    for ($i = 0; $i -lt 40; $i++) {
        try { Add-Content $budgetFile $line -ErrorAction Stop; return } catch { Start-Sleep -Milliseconds (50 + (Get-Random -Maximum 100)) }
    }
    Add-Content $budgetFile $line
}
New-Item -ItemType Directory -Force -Path (Split-Path $budgetFile) | Out-Null
$stamp = (Get-Date).ToString("yyyy-MM-ddTHH:mm:ss")
$startKind = if ($env:PM_AZT_ATTEMPT -eq "2") { "retry-start" } else { "start" } # azt retry (AZFIX): not counted in the budget
Add-BudgetLine "$stamp,$row,$sha,$argStr,$startKind,," # one real Azahar start, one line (-Force too)
if ($startKind -eq "start") { $used += 1 }
function End-Run([int]$rc, [string]$res, [string]$kind = "end") {
    if ($script:slotK) { $res = "slot$($script:slotK): $res" } # azt.ps1 finds its own end line by this prefix
    Add-BudgetLine "$((Get-Date).ToString('yyyy-MM-ddTHH:mm:ss')),$row,$sha,$argStr,$kind,$rc,$($res.Replace(',', ';'))"
    Write-Output "budget: row $row run $used/$max"
    exit $rc
}
# azahar.exe processes now: PID, start time, command line (the holder of a shared file is not named, this lists Azahar only)
function Get-AzaharList { return (@(Get-CimInstance Win32_Process -Filter "Name='azahar.exe'" -ErrorAction SilentlyContinue) | ForEach-Object { "pid $($_.ProcessId), started $($_.CreationDate), cmd: $($_.CommandLine)" }) -join " | " }

function Kill-Azahar([int]$procId, [string]$why = "kill") {
    # Scoped to the PID this script itself launched - never kills other azahar.exe processes (a
    # concurrent agent/session may have its own instance open; a blanket "taskkill /IM azahar.exe"
    # here would silently terminate theirs too).
    Write-PoolLog $slotK "kill" $procId "-" $why # AZT6b-1
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
$cfgBak = ""; $pidFile = "" # set from the slot below
function Restore-Cfg {
    if (Test-Path $cfgBak) {
        Copy-Item $cfgBak $cfg -Force
        Remove-Item $cfgBak -Force
    }
}
# UI1a: the game's options.ini (sdmc). A saved profmode=prof makes the game log only prof lines (debug lines drop), so a run
# without a prof word writes profmode=off into the file, and the file is put back byte for byte the same way as the config.
$optFile = ""; $optBak = "" # set from the slot below
function Restore-Opt {
    if (Test-Path $optBak) {
        Copy-Item $optBak $optFile -Force
        Remove-Item $optBak -Force
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

# U2 (user decision): a run with profiling writes its prof lines to log.txt too (profdisk=1), so the scenario lines and the until stay on the disk copy.
if ($Debug -match '\b(prof|phase)\b' -and $Debug -notmatch 'profdisk') { $Debug = "$Debug profdisk=1" }
if ($Smoke) { $Until = "smoke done"; $Debug = ("$Debug smoke").Trim() }
# L02: the game logs only warnings and errors by default. The tools read info and debug lines, so a run asks for all of them.
if ($Debug -notmatch "loglevel=" -and ($Debug.Length + 12) -le 255) { $Debug = ("$Debug loglevel=debug").Trim() }
if ($Turbo) {
    $Debug = ("$Debug turbo").Trim()
    $Timeout = [Math]::Max(25, [int]($Timeout / $TurboFactor))
    $Stall = [Math]::Max(10, [int]($Stall / $TurboFactor))
    if ($Until -or $Smoke) { Write-Output "TURBO run: timeout $Timeout s. Not valid for performance numbers." }
}

# AZT-1 slot pool: a run takes one of N private portable Azahar copies (azslot.ps1), so runs on different slots go in parallel.
# azt.ps1 takes the slot itself and passes -Slot with PM_SLOT_HELD=1. A direct call takes the first free slot (waits only when all are busy).
$p = $null
$gdbProc = $null
if ($env:PM_SLOT_HELD -and $Slot -gt 0) { $slotK = $Slot }
else {
    $slotK = Get-Slot $Slot $LockWait $Turbo.IsPresent $(if ($Turbo) { $TurboLimit } else { 0 })
    if (-not $slotK) { End-Run 3 "LOCK TIMEOUT: no free Azahar slot after $LockWait s. Nothing was run." }
    $heldHere = $true
    Write-Output "Azahar slot $slotK taken."
}
$sl = Ensure-Slot $slotK
Sync-Slot $slotK
$sd = $sl.sd; $cfg = $sl.cfg; $exe = $sl.exe; $emuLog = $sl.emuLog; $log = "$sd\log.txt"; $pidFile = $sl.pidFile
# AZT3-1: the 3dsx of every lane is copied to the slot rom path, and Azahar boots that path (cold) or restarts it (warm)
$romFile = $sl.rom; $stopReq = "$($sl.dir)\stop.request" # stopReq: azt.ps1 ends this run early (marker, stall, attract, guard); the Azahar is kept (AZT3-2)
$cfgBak = "$($sl.dir)\qt-config.orig"; $optFile = "$sd\options.ini"; $optBak = "$($sl.dir)\options.orig"
if (Test-Path "$sdMaster\options.ini") { Copy-Item "$sdMaster\options.ini" $optFile -Force } # the user's game settings, the slot copy is edited below
$outLane = $out
$out = "$outLane\slot$slotK" # outputs of this slot: no file shared with a run on another slot
New-Item -ItemType Directory -Force -Path $out | Out-Null
# AZT-2: an Azahar kept by an earlier run (-Keep) is reused when it is the same build file and the same speed settings
$keepOn = $Keep.IsPresent -or [bool]$env:PM_AZ_KEEP
$kept = Get-KeptAzahar $slotK
# AZT2: the warm decision and its reason (azslot line below). Reasons: no kept, closed, turbo, option, restart failed (AZT3-1: no lane reason)
$warmWhy = if ($NoWarm -or $Interactive -or $Gdb -or $FaultStop) { "option" } else { Get-WarmState $slotK $Turbo.IsPresent $(if ($Turbo) { $TurboLimit } else { 0 }) }
$warm = ($warmWhy -eq "warm")
if (-not $warm) { Remove-Item $sl.keepFile -ErrorAction SilentlyContinue }
# From here this script owns the slot. Any earlier run on this slot that was killed: kill its Azahar, put the config back.
if (-not $warm -and (Test-Path $pidFile)) {
    $old = [int](Get-Content $pidFile -ErrorAction SilentlyContinue)
    $op = Get-Process -Id $old -ErrorAction SilentlyContinue
    if ($op -and $op.ProcessName -eq "azahar") { Write-Output "Recovery: killing orphan Azahar $old."; Kill-Azahar $old "orphan of a killed run" }
    Remove-Item $pidFile -ErrorAction SilentlyContinue
}
if (Test-Path $cfgBak) { Write-Output "Recovery: restoring qt-config.ini saved by a killed run."; Restore-Cfg }
if (Test-Path $optBak) { Write-Output "Recovery: restoring options.ini saved by a killed run."; Restore-Opt }
# An azahar.exe of this slot that no run owns (a kill left it): the slot is ours, so stop it.
foreach ($sp in @(if (-not $warm) { Get-CimInstance Win32_Process -Filter "Name='azahar.exe'" -ErrorAction SilentlyContinue | Where-Object { $_.ExecutablePath -eq $exe } })) {
    Write-Output "Slot ${slotK}: stopping a stray azahar.exe (pid $($sp.ProcessId))."
    Kill-Azahar ([int]$sp.ProcessId) "stray azahar of this slot" 
}
try {
    Set-Content $pidFile $PID # the script PID until Azahar starts, then the Azahar PID
    Copy-Item $cfg $cfgBak -Force # every run edits the config (Set-GdbStub), so every run saves it first
    if (Test-Path $optFile) { Copy-Item $optFile $optBak -Force } # UI1a: saved before any edit of options.ini
    if ($Turbo) { Set-Turbo }

New-Item -ItemType Directory -Force -Path $sd, $sl.romDir | Out-Null
# AZT8-3: the ROM copy is checked by hash. A copy that did not land (or a stale ROM) is a cold start, never a warm restart.
Set-Content "$($sl.dir)\lane.txt" ("{0}|{1}" -f [string]$env:PM_LANE, $(if ($Turbo) { "turbo" } else { "normal" })) # AZT11-1: the lane and speed this slot now holds (the queue prefers the same key: no reload, no cold start)
try {
    Copy-Item $in3dsx $romFile -Force -ErrorAction Stop
    if ((Get-FileHash $romFile -Algorithm SHA256).Hash -ne (Get-FileHash $in3dsx -Algorithm SHA256).Hash) { throw "ROM copy differs from the lane 3dsx" }
}
catch { if ($warm) { Write-Output "Warm: the ROM copy failed ($($_.Exception.Message)). Cold start."; $warm = $false; $warmWhy = "restart failed"; Kill-Azahar $kept.Id "3dsx locked or ROM copy failed"; Copy-Item $in3dsx $romFile -Force } else { throw } }
# AZT11-4: this start's boot nonce, the first switches word. The game cuts a switch text over 255 bytes at its end, so the nonce goes first.
$bootNonce = [guid]::NewGuid().ToString("N").Substring(0, 10)
if ($Debug.Length -gt 230) { Release-Lock; End-Run 8 "SWITCHES: -Debug is over 230 characters: the boot nonce (AZT11-4) would be cut. Use tour=@file for a long list." }
$Debug = ("bootid=$bootNonce " + $Debug).Trim()
if (-not $warm) { Remove-Item $log -ErrorAction SilentlyContinue } # a cold start deletes the old game log (a warm restart keeps the kept game's file)
if (-not $warm -and (Test-Path $log)) { Release-Lock; End-Run 5 "LOG LOCKED: $log is still there after the delete, another process holds it. Nothing was run. azahar.exe open: $(Get-AzaharList)" }
if ($Smoke -and $Debug -notmatch "fixedseed") { $Debug = "$Debug fixedseed" } # R-SEED: smoke runs use the fixed RNG seed
if ($Debug) { Set-Content "$sd\debug3ds.txt" $Debug } else { Remove-Item "$sd\debug3ds.txt" -ErrorAction SilentlyContinue }
if ($Debug -notmatch '\bprof\b' -and (Test-Path $optFile)) { # UI1a: no prof word, so the saved profmode must not apply
    $optText = Get-Content $optFile -Raw
    if ($optText -match '(?m)^profmode=[^\r\n]*') { $optText = $optText -replace '(?m)^profmode=[^\r\n]*', 'profmode=off' }
    else { $optText = $optText.TrimEnd() + "`r`nprofmode=off`r`n" }
    Set-Content $optFile $optText -NoNewline
    Write-Output "options.ini: profmode=off for this run (no prof word in the debug switches)"
}
Remove-Item "$sd\shots", "$out\shots" -Recurse -ErrorAction SilentlyContinue # "shots" switch output
$emuOff = 0L
if ($warm) { if (Test-Path $emuLog) { $emuOff = (Get-Item $emuLog).Length } } # the emulator log of a kept Azahar stays open: only text after this offset counts
else { Remove-Item $emuLog -ErrorAction SilentlyContinue } # an old exception dump must not stop this run
if (-not $warm -and (Test-Path $emuLog)) { Release-Lock; End-Run 5 "LOG LOCKED: $emuLog is still there after the delete, another process holds it. Nothing was run. azahar.exe open: $(Get-AzaharList)" }
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

$p = $null
$warmDone = $false
$start = Get-Date
if ($warm) {
    # AZT-2 warm restart: Azahar > Emulation > Restart (hotkey of the slot config) boots the same 3dsx again.
    # AZT11-4: the boot is found by this run's nonce (bootid=, the first word of the switches line). The old boot never prints it, whatever the
    # log file does on a restart (AZT9d-2 and AZT11-4a: a log overwritten in place from offset 0, same banner count and size, was never seen as new).
    # Four key methods per round (posted, keybd_event, keybd_event again, SwitchToThisWindow + keybd_event). A boot that is not this run
    # (other switches, or another build than this lane 3dsx) gets a second round in the kept Azahar with a new nonce (switches only);
    # a stale build is cold at once. A cold start is the last resort.
    $hk = Get-SlotHotkey $slotK "Restart Emulation"
    $warmDone = $false
    for ($round = 1; $round -le 2; $round++) {
        $start = Get-Date
        Set-Content "$($sl.dir)\warm.start" $start.Ticks # azt.ps1 starts its clock on this file
        Set-Content $pidFile $kept.Id
        $boot = $null
        for ($try = 1; $try -le 4 -and -not $boot; $try++) {
            $script:AzLastFg = $false
            $kept.Refresh()
            $method = @("posted", "keybd_event", "keybd_event", "switch_to_window")[$try - 1]
            if ($try -eq 1) { $sent = Send-AzHotkey $kept $hk }
            elseif ($try -eq 4) { $sent = Send-AzKeyEventSwitch $kept.MainWindowHandle $hk }
            else { $sent = Send-AzKeyEvent $kept.MainWindowHandle $hk }
            $fgTxt = if ($script:AzLastFg) { "foreground" } else { "no foreground" }
            Write-PoolLog $slotK "warm try" $kept.Id $kept.Id ("round {0} try {1} {2} hotkey {3} sent={4} {5} title '{6}'" -f $round, $try, $method, $hk, $sent, $fgTxt, $kept.MainWindowTitle)
            if (-not $sent) { continue }
            for ($w = 0; $w -lt 60 -and -not $boot; $w++) {
                Start-Sleep -Milliseconds 250
                $boot = Find-BootNonce $log $bootNonce
            }
            Write-PoolLog $slotK "warm wait" $kept.Id $kept.Id ("round {0} try {1} nonce {2} after {3:N1} s" -f $round, $try, $(if ($boot) { "found" } else { "none" }), ((Get-Date) - $start).TotalSeconds)
        }
        if (-not $boot) { $warmWhy = "restart failed"; break }
        $buildOk = Test-Build3dsx $in3dsx ([string]$boot.build) # AZT11-4a: the lane 3dsx holds the boot's build id
        $swOk = Test-SwitchWords ([string]$boot.switches) $Debug.Trim() # AZT11-4b: every switch word of this run is in the boot
        if ($buildOk -and $swOk) {
            $p = $kept; $warmDone = $true
            Write-Output ("Warm restart ok after {0:N1} s (hotkey {1}, round {2})." -f ((Get-Date) - $start).TotalSeconds, $hk, $round)
            Set-Content "$($sl.dir)\warm.ok" "ok" # AZT3-3: azt.ps1 reads this boot's log from its first byte after this line
            Write-PoolLog $slotK "warm" $kept.Id $kept.Id "restart, this run's switches"
            break
        }
        $warmWhy = if (-not $buildOk) { "stale rom" } else { "switch mismatch" }
        Write-Output ("Warm restart: the boot is not this run ({0}; boot build '{1}', boot switches '{2}'), round {3} of 2." -f $warmWhy, $boot.build, $boot.switches, $round)
        if (-not $buildOk -or $round -eq 2) { break }
        # round 2: the old boot prints the old nonce, so the next boot gets a new one (the switches file is rewritten with it)
        $bootNonce = [guid]::NewGuid().ToString("N").Substring(0, 10)
        $Debug = ("bootid=$bootNonce " + ($Debug -replace '^bootid=\S+\s*', '')).Trim()
        Set-Content "$sd\debug3ds.txt" $Debug
    }
    if (-not $warmDone) {
        Write-Output "Warm restart failed ($warmWhy): cold start."
        Kill-Azahar $kept.Id $warmWhy
        Remove-Item $log, $emuLog -ErrorAction SilentlyContinue
        $emuOff = 0L
        $warm = $false
    }
}
if (-not $p) {
    $p = Start-Process $exe -ArgumentList "`"$romFile`"" -PassThru
    Set-Content $pidFile $p.Id
    Write-PoolLog $slotK "start" "-" $p.Id "cold: $warmWhy"
}
# AZT2: one line per start: warm, or cold with the reason. AZT6b-2: written after the switch check and the start, with the pid. azt.ps1 reads it from the slot folder.
$azLine = if ($warm) { "azslot: slot $slotK pid $($p.Id) warm" } else { "azslot: slot $slotK pid $($p.Id) cold ($warmWhy)" } # azt.ps1 matches the end: " warm" or " cold ("
Set-Content "$($sl.dir)\azslot.line" $azLine
Write-Output $azLine
if ($Gdb) {
    # Azahar waits for gdb before it runs the game. winhost = the Windows host (IPv4) seen from Docker.
    $cmds = @("set pagination off", "set confirm off", "set width 0", "target remote winhost:24689")
    if ($GdbScript) { $cmds += Get-Content $GdbScript }
    $cmds += @("continue", "echo \n--- stopped ---\n", "bt 30", "info registers", 'x/32wx $sp', "kill")
    Set-Content "$out\gdb_cmds.txt" $cmds
    Start-Sleep -Seconds 2
    $gdbName = "pm3ds-gdb-$PID"  # named, so the cleanup below can kill this container and no other
    $gdbArgs = "run --rm --name $gdbName --add-host=winhost:host-gateway -v ${vol}:/work -v `"${out}:/out`" " +
               "devkitpro/devkitarm:latest /opt/devkitpro/devkitARM/bin/arm-none-eabi-gdb -q -batch " +
               "-x /out/gdb_cmds.txt /work/build/pm_3ds.elf"
    $gdbProc = Start-Process docker -ArgumentList $gdbArgs -NoNewWindow -PassThru `
        -RedirectStandardOutput "$out\last_gdb.txt" -RedirectStandardError "$out\last_gdb_err.txt"
}
if (-not $warmDone) { $start = Get-Date }
$reason = "timeout ($Timeout s)"
$lastSize = -1
$lastGrow = Get-Date
$echoed = 0
$marks = @()
    while (((Get-Date) - $start).TotalSeconds -lt $Timeout) {
        Start-Sleep -Milliseconds 250
        if ($p.HasExited) { $reason = "Azahar exited"; break }
        if ($gdbProc -and $gdbProc.HasExited) { $reason = "gdb finished"; break }
        # Azahar shows a dialog on a guest CPU exception and the game stops: kill at once
        if (-not $Interactive -and [AzEnum]::Count([uint32]$p.Id) -gt 1) { Start-Sleep -Seconds 4; $reason = "Azahar dialog (exception or error), windows: $([AzEnum]::Titles)"; break }
        if (Test-Path $stopReq) { $reason = "stopped by azt: $(Get-Content $stopReq -First 1)"; break } # AZT3-2: azt ends the run early, the Azahar stays (see keepNow)
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
        if ($Until -and $text -match $Until) { $reason = "matched '$Until'"; $untilAt = ((Get-Date) - $start).TotalSeconds; break } # AZT10-1: until time (s from the start)
    }
    # S10C-1: the until line ends the game as well. Without a pause the emulator runs on after the until line (the shots debug dumps a shot every
    # 30 frames to 200) and the shot wait below waited for shots the run does not need: nok11_det ran 28 s past its until line. Pause at the
    # until line (the keep pause below then is not sent again): no frame and no shot after it. The shot wait only settles files already written.
    $pausedAtUntil = $false
    if ($reason -like "matched*" -and $p -and -not $p.HasExited -and -not $Interactive -and -not $Gdb) {
        [void](Send-AzHotkey $p (Get-SlotHotkey $slotK "Continue\Pause Emulation")); $pausedAtUntil = $true
    }
    # SHOTCOPY1: the emulator closes a shot file after its shot line, but the host can see part of it for a while (DET saw 155648 of 288015 B).
    # The run waits (up to 30 s) until every shot line of this start has its full file: 288015 B (header "P6 240 400 255" + LF = 15 B, then 240 * 400 * 3),
    # the same size on two polls 250 ms apart. Shot lines before this start (a warm restart keeps the log) are not counted: only the lines after this run's bootid.
    # A logged shot without a full file is a failure: "SHOT COPY FAILED", and azt.ps1 fails the scenario.
    $shotFull = 288015
    $shotSfx = ""
    if (Test-Path $log) {
        $logged = @()
        try {
            $logTxt = Read-Tail $log 4194304
            $anch = $logTxt.LastIndexOf("bootid=$bootNonce") # SHOTCOPY1: the shot lines of this start follow this run's switches line (warm starts keep the file)
            if ($anch -ge 0) { $logTxt = $logTxt.Substring($anch) } else { $logTxt = "" }
            $logged = @([regex]::Matches($logTxt, '(?m)^#\d+ shot (\d+) frame \d+') | ForEach-Object { [int]$_.Groups[1].Value } | Sort-Object -Unique)
        } catch { Write-Output "SHOT COPY FAILED: the game log could not be read for its shot lines: $($_.Exception.Message)" }
        # The burst writer (shotmode) gives every logged frame the same counter, so a logged number is checked against the files on disk, not one file per line.
        $prevLen = @{}; $okN = @{}; $nFiles = 0
        $shotWaitN = if ($pausedAtUntil) { 20 } else { 120 } # 20 polls of 250 ms: 5 s after the pause at the until line, else 30 s
        for ($w = 0; $w -lt $shotWaitN; $w++) {
            $files = @(Get-ChildItem "$sd\shots" -File -ErrorAction SilentlyContinue | Where-Object { $_.Name -match '^\d{4}_.*\.ppm$' })
            $have = @{}; foreach ($f1 in $files) { $have[[int]$f1.Name.Substring(0, 4)] = $true }
            $nFiles = $files.Count
            $allOk = $true
            foreach ($f1 in $files) {
                $len = $f1.Length
                if ($len -eq $shotFull -and $prevLen[$f1.Name] -eq $len) { $okN[$f1.Name] = $true } else { $prevLen[$f1.Name] = $len; $allOk = $false; $okN.Remove($f1.Name) }
            }
            $missingNow = @($logged | Where-Object { -not $have.ContainsKey($_) })
            if ($allOk -and $missingNow.Count -eq 0 -and $files.Count -gt 0) { break }
            Start-Sleep -Milliseconds 250
        }
        $files = @(Get-ChildItem "$sd\shots" -File -ErrorAction SilentlyContinue | Where-Object { $_.Name -match '^\d{4}_.*\.ppm$' })
        $have = @{}; foreach ($f1 in $files) { $have[[int]$f1.Name.Substring(0, 4)] = $true }
        $missing = @($logged | Where-Object { -not $have.ContainsKey($_) })
        $short = @($files | Where-Object { -not $okN.ContainsKey($_.Name) } | ForEach-Object { "$($_.Name)=$($_.Length)" })
        $bad = @($missing) + @($short)
        Write-Output ("SHOT FILES: {0} logged this run, {1} top-screen files, {2} full ({3} B)" -f $logged.Count, $files.Count, $okN.Count, $shotFull)
        $shotSfx = "; shot files {0} of {1} full" -f $okN.Count, $files.Count
        if ($bad.Count) { $shotSfx += "; SHOT COPY FAILED: missing " + (($missing) -join " ") + " short " + ($short -join " ") + " after 30 s"; Write-Output ("SHOT COPY FAILED after 30 s: missing numbers [{0}], short files [{1}]" -f (($missing) -join " "), ($short -join " ")) }
    }
    # AZFIX: the Azahar exit code is recorded in the result text. An early exit with no game log and an empty emulator log is retried once by azt (kind retry)
    $azState = if ($p.HasExited) { "exit code $($p.ExitCode)" } else { "running, stopped by this run" }
    if ($reason -eq "crash" -or $reason -eq "Azahar exited") { Write-PoolLog $slotK "exit" $p.Id "-" "$reason ($azState)" } # AZT6b-1
    $retryable = $reason -eq "Azahar exited" -and (-not (Test-Path $emuLog) -or (Get-Item $emuLog).Length -eq 0)
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
    # AZT3-2: a run keeps its Azahar unless it exited or crashed (a [CRASH] line, an emulator exception, an exception dialog). A run that
    # timed out, missed its until line or was stopped by azt is kept too (paused). -NoKeep, Gdb and Interactive close it.
    $crashed = $reason -match '^(crash|emulator exception|Azahar dialog)'
    $keepNow = $keepOn -and $p -and -not $p.HasExited -and -not $crashed -and -not $Gdb -and -not $Interactive
    Remove-Item $stopReq -ErrorAction SilentlyContinue
    if ($keepNow) {
        # AZT-2: pause the game (no CPU while idle) and leave Azahar open; the idle reaper closes it after AzIdleSec (900 s, AZT3-4)
        if (-not $pausedAtUntil) { [void](Send-AzHotkey $p (Get-SlotHotkey $slotK "Continue\Pause Emulation")) } # S10C-1: already paused at the until line
        @{ pid = $p.Id; x3dsx = $x3dsx; turbo = $Turbo.IsPresent; turbolimit = $(if ($Turbo) { $TurboLimit } else { 0 }); time = (Get-Date).ToString("s") } | ConvertTo-Json | Set-Content $sl.keepFile
        Start-SlotReaper $slotK
    }
    elseif ($p) { Kill-Azahar $p.Id; Remove-Item $sl.keepFile -ErrorAction SilentlyContinue }
    if ($gdbProc -and -not $gdbProc.HasExited) { Stop-Process -Id $gdbProc.Id -Force -ErrorAction SilentlyContinue }
    if ($gdbName) { docker kill $gdbName 2>$null | Out-Null }  # the docker client does not stop its container when stopped
    Restore-Cfg # byte for byte copy of the real config
    if (Test-Path $optBak) { Restore-Opt } # UI1a: options.ini back byte for byte
    if (-not $keepNow) { Remove-Item $pidFile -ErrorAction SilentlyContinue }
    # A later manual launch must not inherit this run's switches (autoa, shots, fast).
    Remove-Item "$sd\debug3ds.txt" -ErrorAction SilentlyContinue
}

# AZLOCK1: the shot move and the log copy run inside the lock (released at the last read below). A terminating error here would reach the trap, which releases the lock before the move: a caught error keeps the lock and reports instead.
try {
    if (Test-Path "$sd\shots") { Move-Item "$sd\shots" "$out\shots" -Force -ErrorAction Stop } # move, not copy: the frames can be hundreds of MB
} catch { Write-Output "SHOTS MOVE FAILED: $($_.Exception.Message). The shots stay in $sd\shots and the next run deletes them." }
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
        if ($cur -eq $prev -and $k -ge 1) { break } # AZT10-3: the log copy waits 0.3 s for a flush, not 1.2 s
        $prev = $cur
        Start-Sleep -Milliseconds 300
    }
    try { Copy-Item $log "$out\last_log.txt" -Force -ErrorAction Stop; Copy-Item $log "$outLane\last_log.txt" -Force -ErrorAction SilentlyContinue; Get-Content "$out\last_log.txt" | Select-Object -Last 15 } catch { Write-Output "LOG COPY FAILED: $($_.Exception.Message)" }
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
    if ($emuOff -gt 0) { $sr.BaseStream.Seek($emuOff, "Begin") | Out-Null }
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
    & docker run --rm -v ${vol}:/work devkitpro/devkitarm:latest sh -c "$DEVKITARM/bin/arm-none-eabi-addr2line -f -C -p -e /work/build/pm_3ds.elf $($addrs -join ' ')" |
        Where-Object { $_ -notmatch "^\?\?" }
    End-Run 2 "crash symbols; stopped: $reason"
}
if ($Smoke -and $reason -notmatch "^matched") { Write-Output "SMOKE FAILED: $reason"; End-Run 1 "smoke failed: $reason" }
$endKind = if ($retryable -and $env:PM_AZT_ATTEMPT -eq "1") { "retry" } else { "end" }
$endAt = ((Get-Date) - $start).TotalSeconds # AZT10-1: end of the run (after the pause, the log copy and the shot move)
$timing = if ($null -ne $untilAt) { "; until at {0:N1} s, end at {1:N1} s, {2:N1} s after the until line" -f $untilAt, $endAt, ($endAt - $untilAt) } else { "; no until line, end at {0:N1} s (cap or stop)" -f $endAt }
$warmSfx = if ($warmDone) { " warm" } else { "" }
$keepSfx = if ($keepNow) { " kept" } else { "" }
$queueSfx = if ($env:PM_QUEUE_INFO) { " | " + $env:PM_QUEUE_INFO } else { "" } # AZT11-1: queue position and wait of this start (set by azt.ps1)
End-Run 0 ("stopped after ${secs}s: $reason (Azahar $azState)" + $warmSfx + $keepSfx + $timing + $queueSfx + $shotSfx) $endKind
