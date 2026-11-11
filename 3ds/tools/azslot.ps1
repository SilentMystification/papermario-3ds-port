<#
azslot.ps1 - Azahar slot pool (AZT-1). Dot-sourced by 3ds/run_azahar.ps1 and 3ds/tools/azt.ps1.

A slot is a private portable copy of the Azahar install: build3ds/azslots/slot<k>/ (gitignored) with azahar.exe and a
`user` folder next to it. Azahar then keeps config, nand, sdmc, shaders and logs in slot<k>/user and nothing in %APPDATA%.
One named mutex per slot (Global\pm_azahar_slot<k>) lets one run use a slot at a time. N slots: env PM_AZ_SLOTS (default 3, AZT12-4; the GPU is shared with other apps).
The user's C:\Program Files\Azahar and %APPDATA%\Azahar are only read (copied from), never written.
#>
$script:AzRepo = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$script:AzTree = Split-Path -Parent (Split-Path -Parent $PSScriptRoot) # AZT6b: the worktree that runs this copy of the tools
# AZT6-1: one slot pool for every worktree. The slots live in the main folder (the parent of the git common dir), whichever worktree calls.
$gcd = & git -C $script:AzRepo rev-parse --path-format=absolute --git-common-dir 2>$null
if ($LASTEXITCODE -eq 0 -and $gcd) { $script:AzRepo = Split-Path -Parent ([string]$gcd).Trim() }
$script:AzInstall = "C:\Program Files\Azahar"
$script:AzMaster = "$env:APPDATA\Azahar"
$script:SlotMutex = @{}
# AZT3-4: the one idle time of a kept Azahar. azreap.ps1 (dot-sources this file) closes it after this many seconds without a run.
# AZT13 (user, 2026-11-08: reuse instances): 60 min, so a session keeps its Azahars between runs (was 15 min).
$script:AzIdleSec = 3600

# AZT4-1: the cap on live azahar.exe processes of the slot pool, all slots of all worktrees and agents. A constant, not an env value.
# AZT12-4 (2026-11-08, Ryzen 3950X, RTX 2070 SUPER 8 GB, 64 GB RAM, with the user's apps open): a turbo load Azahar ran 4.2 % slower per
# 300 frames with 3 active Azahars than with 2 (3.805 s vs 3.651 s); a 4th live Azahar failed at start ("Failed to allocate Stream buffer",
# the GPU host-visible heap). Cap 2 (user, 2026-11-08): with 3 live Azahars cold starts failed with 0xC000001D (GPU fault) and the
# fault retry closed the other kept instances.
$script:AzCap = 2
$script:CapSaid = $false

function Get-SlotCount {
    $n = if ($env:PM_AZ_SLOTS) { [int]$env:PM_AZ_SLOTS } else { 2 }
    if ($n -gt $script:AzCap) {
        if (-not $script:CapSaid) { Write-Host "azslot: PM_AZ_SLOTS=$n is above the cap $($script:AzCap) live Azahars: using $($script:AzCap) slots"; $script:CapSaid = $true }
        $n = $script:AzCap
    }
    return $n
}

function Get-LiveAzahar {
    # AZT4-1: every azahar.exe that runs from a slot folder (build3ds\azslots\slot<k>), of any worktree. Returns the CIM processes.
    return @(Get-CimInstance Win32_Process -Filter "Name='azahar.exe'" -ErrorAction SilentlyContinue | Where-Object { $_.ExecutablePath -match '\\azslots\\slot\d+\\azahar\.exe$' })
}

# AZT6b-1: one line per pool event in build3ds\azslots\pool.log (shared by all worktrees): time, tree, slot, pid old -> new, event, reason.
function Write-PoolLog([int]$slot, [string]$event, $old, $new, [string]$reason) {
    try {
        $f = Join-Path $script:AzRepo "build3ds\azslots\pool.log"
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $f) | Out-Null
        $line = "{0} {1} slot{2} pid {3} -> {4} {5} {6}" -f (Get-Date -Format "yyyy-MM-ddTHH:mm:ss"), $script:AzTree, $slot, $old, $new, $event, $reason
        Add-Content -Path $f -Value $line
    } catch { }
}

function Close-IdleKept([int]$except = 0) {
    # AZT4-2: close every kept Azahar that no run holds (its slot mutex is free), of any worktree. $except is a slot number that the caller holds.
    # Returns the number closed. A kept Azahar is one with a keep.json in its slot folder. Waits up to 5 s for each process to end.
    $closed = 0
    foreach ($pr in @(Get-LiveAzahar)) {
        $dir = Split-Path -Parent $pr.ExecutablePath
        $num = [int]([regex]::Match($dir, 'slot(\d+)$').Groups[1].Value)
        if ($num -eq $except -or -not (Test-Path (Join-Path $dir "keep.json"))) { continue }
        if (-not (Enter-Slot $num 0)) { continue }
        try {
            Stop-Process -Id $pr.ProcessId -Force -ErrorAction SilentlyContinue
            for ($i = 0; $i -lt 50 -and (Get-Process -Id $pr.ProcessId -ErrorAction SilentlyContinue); $i++) { Start-Sleep -Milliseconds 100 }
            Remove-Item (Join-Path $dir "keep.json") -ErrorAction SilentlyContinue
            Write-PoolLog $num "close" $pr.ProcessId "-" "close kept (cap or GPU fault retry)"
            $closed++
        } finally { Exit-Slot $num }
    }
    return $closed
}

function Get-SlotPaths([int]$k) {
    $d = Join-Path $script:AzRepo "build3ds\azslots\slot$k"
    return @{
        k = $k; dir = $d; exe = "$d\azahar.exe"; user = "$d\user"
        cfg = "$d\user\config\qt-config.ini"; sd = "$d\user\sdmc\3ds\PaperMario"
        rom = "$d\rom\pm_3ds.3dsx"; romDir = "$d\rom" # AZT3-1: every lane's 3dsx is copied here, so a kept Azahar always restarts this one path
        emuLog = "$d\user\log\azahar_log.txt"; pidFile = "$d\slot.pid"; keepFile = "$d\keep.json"
        mutexName = "Global\pm_azahar_slot$k"
    }
}

function Ensure-Slot([int]$k) {
    # Copy once: Azahar install, qt-config.ini, nand, sysdata, shaders, dsp firmware and the ROM image. Serialised by a mutex.
    $s = Get-SlotPaths $k
    if ((Test-Path "$($s.user)\config\qt-config.ini") -and (Test-Path "$($s.sd)\papermario.z64") -and (Test-Path $s.exe)) { return $s }
    $m = New-Object Threading.Mutex($false, "Global\pm_azahar_slotsetup")
    try { [void]$m.WaitOne() } catch [Threading.AbandonedMutexException] {}
    try {
        if (-not (Test-Path $s.exe)) {
            Write-Output "slot ${k}: creating $($s.dir) (copy of the Azahar install, once)"
            & robocopy $script:AzInstall $s.dir /E /XF uninst.exe tests.exe /NFL /NDL /NJH /NJS /NP | Out-Null
            if ($LASTEXITCODE -ge 8) { throw "robocopy of the Azahar install failed ($LASTEXITCODE)" }
        }
        New-Item -ItemType Directory -Force -Path "$($s.user)\config", "$($s.user)\log", "$($s.user)\sdmc\3ds\PaperMario" | Out-Null
        foreach ($sub in @("nand", "sysdata", "shaders")) {
            if ((Test-Path "$script:AzMaster\$sub") -and -not (Test-Path "$($s.user)\$sub")) {
                & robocopy "$script:AzMaster\$sub" "$($s.user)\$sub" /E /NFL /NDL /NJH /NJS /NP | Out-Null
                if ($LASTEXITCODE -ge 8) { throw "robocopy of $sub failed ($LASTEXITCODE)" }
            }
        }
        if (-not (Test-Path "$($s.user)\sdmc\3ds\dspfirm.cdc") -and (Test-Path "$script:AzMaster\sdmc\3ds\dspfirm.cdc")) { Copy-Item "$script:AzMaster\sdmc\3ds\dspfirm.cdc" "$($s.user)\sdmc\3ds\" -Force }
        if (-not (Test-Path "$($s.sd)\papermario.z64")) { Copy-Item "$script:AzMaster\sdmc\3ds\PaperMario\papermario.z64" $s.sd -Force }
        if (-not (Test-Path $s.cfg)) {
            # the slot config points to the slot folders, not to %APPDATA%
            $t = Get-Content "$script:AzMaster\config\qt-config.ini" -Raw
            $u = $s.user.Replace("\", "/")
            $t = $t -replace '(?m)^sdmc_directory=.*$', "sdmc_directory=$u/sdmc/"
            $t = $t -replace '(?m)^nand_directory=.*$', "nand_directory=$u/nand/"
            $t = $t -replace '(?m)^Paths\\screenshotPath=.*$', "Paths\screenshotPath=$u/screenshots/"
            Set-Content $s.cfg $t -NoNewline
        }
    } finally { $m.ReleaseMutex() }
    return $s
}

function Sync-Slot([int]$k) {
    # Small helper files of the master game folder (pad scripts, tour lists, routes, options.ini) to the slot sdmc. Never the 3dsx, logs, shots or dumps.
    $s = Get-SlotPaths $k
    $src = "$script:AzMaster\sdmc\3ds\PaperMario"
    & robocopy $src $s.sd /E /MAX:2000000 /XF *.3dsx log.txt debug3ds.txt save.flash texsrc_* texgpu_* papermario.z64 *.bmp *.ppm *.png *.bin /XD shots shots_* old_shots texdump sweep /NFL /NDL /NJH /NJS /NP | Out-Null
    if ($LASTEXITCODE -ge 8) { throw "slot ${k}: sync of the game folder failed ($LASTEXITCODE)" }
    $global:LASTEXITCODE = 0
    foreach ($f in @("ndref.bin")) { if ((Test-Path "$src\$f") -and -not (Test-Path "$($s.sd)\$f")) { Copy-Item "$src\$f" $s.sd -Force } }
}

function Get-KeptAzahar([int]$k) {
    # A live azahar.exe of this slot that a run left open (-Keep): the process, else $null
    $s = Get-SlotPaths $k
    $procs = @(Get-CimInstance Win32_Process -Filter "Name='azahar.exe'" -ErrorAction SilentlyContinue | Where-Object { $_.ExecutablePath -eq $s.exe })
    if ($procs.Count) { return Get-Process -Id $procs[0].ProcessId -ErrorAction SilentlyContinue }
    return $null
}

function Enter-Slot([int]$k, [int]$waitMs) {
    # Try to take the mutex of slot k. $true when held.
    if (-not $script:SlotMutex.ContainsKey($k)) { $script:SlotMutex[$k] = New-Object Threading.Mutex($false, (Get-SlotPaths $k).mutexName) }
    try { return $script:SlotMutex[$k].WaitOne($waitMs) } catch [Threading.AbandonedMutexException] { return $true }
}

function Exit-Slot([int]$k) {
    if ($script:SlotMutex.ContainsKey($k)) { try { $script:SlotMutex[$k].ReleaseMutex() } catch {} }
}

# AZT13: the speed an Azahar of slot k runs at: the frame_limit and use_vsync it starts with (qt-config.ini, read only at the Azahar start).
# Normal is the slot config. Turbo (W01) is vsync off and the limit TurboLimit, unless the slot config limit is already 0: Azahar's 0 is
# "unthrottled" (2026-11-08: a normal cold spot_mac01_det ran the 300-frame window in about 5 s, 2x the 3DS rate), which no limit
# raises, so turbo keeps 0. With the user's config (frame_limit=0, use_vsync=false) turbo and normal are the same speed and one kept Azahar
# restarts both (the old turbo flag forced a cold start at each change). Returns "fl<limit> vs<0|1>".
function Get-SlotSpeed([int]$k, [bool]$turbo, [int]$turbolimit) {
    $t = Get-Content (Get-SlotPaths $k).cfg -Raw
    $fl = [int][regex]::Match($t, '(?m)^frame_limit=(\d+)').Groups[1].Value
    $vs = [regex]::Match($t, '(?m)^use_vsync=(\w+)').Groups[1].Value -eq "true"
    if ($turbo) { $vs = $false; if ($fl -ne 0) { $fl = $turbolimit } }
    return ("fl{0} vs{1}" -f $fl, [int]$vs)
}

function Get-WarmState([int]$k, [bool]$turbo, [int]$turbolimit) {
    # AZT2: can a run restart in slot k? Returns "warm" or the reason it cannot:
    # "no kept" (no Azahar kept in the slot), "closed" (keep.json, but its Azahar is gone), "speed" (AZT13: the kept Azahar runs another
    # frame_limit or vsync, Get-SlotSpeed). AZT3-1: no lane check. Every lane's 3dsx is copied to the slot rom path and the restart loads that path.
    $kept = Get-KeptAzahar $k
    $info = Get-KeepInfo $k
    if (-not $kept) { if ($info) { return "closed" } else { return "no kept" } }
    if (-not $info -or [int]$info.pid -ne $kept.Id) { return "no kept" }
    if ([string]$info.speed -ne (Get-SlotSpeed $k $turbo $turbolimit)) { return "speed" }
    return "warm"
}

function Get-Slot([int]$want, [int]$waitSec, [bool]$turbo = $false, [int]$turbolimit = 0, [bool]$needCold = $false) {
    # $want > 0 takes that slot. 0 takes the first free slot in this order (AZT2 affinity):
    #   1. a free slot whose kept Azahar restarts this run (same turbo setting, any lane: AZT3-1): warm
    #   2. a free slot with no kept Azahar (cold start, no other lane is disturbed)
    #   3. any other free slot (its kept Azahar belongs to another lane or turbo setting)
    # Waits only when all slots are busy. Returns the slot number, 0 on timeout.
    # AZT4-1: a start that needs a new Azahar process (cold) takes room under the cap first. At the cap, a free slot whose Azahar is
    # live (restart, no new process) is taken before a cold slot.
    # AZT6-2: at the cap a cold slot is not taken and no kept Azahar is closed (Close-IdleKept runs on a GPU fault only): the start waits.
    $n = Get-SlotCount
    $t0 = Get-Date
    $said = $false
    while ($true) {
        $all = @(if ($want -gt 0) { $want } else { 1..$n })
        $warmK = @($all | Where-Object { (Get-WarmState $_ $turbo $turbolimit) -eq "warm" })
        $liveK = @($all | Where-Object { $warmK -notcontains $_ -and (Get-KeptAzahar $_) })
        $coldK = @($all | Where-Object { $warmK -notcontains $_ -and $liveK -notcontains $_ })
        $capFull = (@(Get-LiveAzahar).Count -ge $script:AzCap)
        # AZKEEP: a run that must start cold does not replace a kept session when a free slot has none
        $order = if ($needCold) { $coldK + $liveK + $warmK } elseif ($capFull) { $warmK + $liveK + $coldK } else { $warmK + $coldK + $liveK }
        foreach ($k in $order) {
            if (-not (Enter-Slot $k 0)) { continue }
            if ($coldK -contains $k -and $capFull) { Exit-Slot $k; continue }
            return $k
        }
        if (-not $said) { Write-Host $(if ($want -gt 0) { "Azahar slot $want is busy." } else { "All $n Azahar slots are busy." }) "Waiting up to $waitSec s..."; $said = $true }
        if (((Get-Date) - $t0).TotalSeconds -gt $waitSec) { return 0 }
        Start-Sleep -Milliseconds 400
    }
}

# ---- AZT-2: keep an Azahar open after a run and restart emulation in it (warm start) ----
# AZT13: Azahar's shortcuts (Restart Emulation F6, Continue\Pause F4) have Context=2 in the slot qt-config.ini (Qt::ApplicationShortcut).
# Qt's shortcut map fires one only while the application has an active window, and Qt's Windows plugin sets that from WM_SETFOCUS and
# WM_KILLFOCUS, not from the OS foreground. The old path made Azahar the OS foreground window, posted the key, and 150 ms later gave the
# foreground back; every agent's run did the same for its own Azahar (restart and pause keys). Focus changes are sent messages: Azahar
# handles them before any posted message. So when the foreground moved again before Azahar's GUI thread read the posted key (the restore
# 150 ms later, or another agent's swap), Qt saw no active window and dropped the key (pool.log 2026-11-08: try 1 "foreground" and "nonce
# none" in about 40 % of rounds). The key now goes with its own focus in the same queue: post WM_SETFOCUS, the key down and up, then
# WM_KILLFOCUS, all to the hwnd of this pid. Azahar handles them in that order whatever window has the OS focus, and no other window moves.
if (-not ([System.Management.Automation.PSTypeName]'AzKey').Type) {
    Add-Type @"
using System; using System.Runtime.InteropServices;
public class AzKey {
  [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
  [DllImport("user32.dll")] public static extern uint MapVirtualKey(uint c, uint t);
  [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
}
"@
}

function Get-SlotHotkey([int]$k, [string]$action) {
    # The key sequence of a Main Window shortcut in the slot qt-config.ini ("F6", "Ctrl+R")
    $s = Get-SlotPaths $k
    $name = $action.Replace(" ", "%20")
    $m = [regex]::Match((Get-Content $s.cfg -Raw), "(?m)^" + [regex]::Escape("Shortcuts\Main%20Window\$name\KeySeq=") + "([^\r\n]*)")
    if ($m.Success) { return $m.Groups[1].Value.Trim() }
    return ""
}

function Send-AzHotkey($proc, [string]$seq) {
    # Posts the key (one key, no modifier: Qt reads Ctrl, Shift and Alt from the real keyboard state, which a posted message does not set)
    # between a posted WM_SETFOCUS and WM_KILLFOCUS to the Azahar main window of this process. $true when posted.
    $proc.Refresh()
    $h = $proc.MainWindowHandle
    if ($h -eq [IntPtr]::Zero) { return $false }
    $vk = 0
    if ($seq -match '^F(\d{1,2})$') { $vk = 0x6F + [int]$Matches[1] }
    elseif ($seq -match '^[A-Za-z0-9]$') { $vk = [int][char]$seq.ToUpper() }
    if (-not $vk) { Write-Host "Send-AzHotkey: key '$seq' is not one F key, letter or digit (set a plain key in the slot qt-config.ini)"; return $false }
    $sc = [AzKey]::MapVirtualKey($vk, 0)
    $ok = [AzKey]::PostMessage($h, 0x0007, [IntPtr]::Zero, [IntPtr]::Zero) # WM_SETFOCUS: Qt's active window is Azahar's main window
    $ok = $ok -and [AzKey]::PostMessage($h, 0x0100, [IntPtr]$vk, [IntPtr][int64](1 + ($sc -shl 16))) # WM_KEYDOWN
    $ok = $ok -and [AzKey]::PostMessage($h, 0x0101, [IntPtr]$vk, [IntPtr][int64](1 + ($sc -shl 16) + 0xC0000000)) # WM_KEYUP
    # WM_KILLFOCUS gives Qt's active window back, unless Azahar is the OS foreground window (the user works in it)
    if ($ok -and [AzKey]::GetForegroundWindow() -ne $h) { $ok = [AzKey]::PostMessage($h, 0x0008, [IntPtr]::Zero, [IntPtr]::Zero) }
    return [bool]$ok
}

function Get-KeepInfo([int]$k) {
    $s = Get-SlotPaths $k
    if (-not (Test-Path $s.keepFile)) { return $null }
    try { return (Get-Content $s.keepFile -Raw | ConvertFrom-Json) } catch { return $null }
}

function Start-SlotReaper([int]$k) {
    # A hidden process closes the kept Azahar of slot k after $script:AzIdleSec idle seconds (one reaper per slot: a mutex)
    # AZT10-2: a reaper already holds its slot mutex: no new process (a start costs about a second on every keep)
    $held = $false
    try { $rm = New-Object Threading.Mutex($false, "Global\pm_azahar_reaper$k"); if ($rm.WaitOne(0)) { $rm.ReleaseMutex() } else { $held = $true } } catch [Threading.AbandonedMutexException] { $held = $false }
    if ($held) { return }
    $ps = (Get-Process -Id $PID).Path
    Start-Process $ps -ArgumentList "-NoProfile -ExecutionPolicy Bypass -File `"$PSScriptRoot\azreap.ps1`" -Slot $k" -WindowStyle Hidden | Out-Null
}

# ---- AZT11-1..3: one machine-wide run queue ----
# One json file per run that waits or runs: build3ds\azslots\queue\<t>-<pid>-<rnd>.json (shared by all worktrees; the slot pool is in the main folder).
# Every read-modify-write runs under one named mutex (Global\pm_azqueue). A run is: wait (FIFO position), granted (slot k reserved for it),
# running (runner = its run_azahar.ps1 process, runT = start of the scenario clock (Set-QRunClock), cap = scenario cap). Stale entries are removed and their runs booted (Invoke-QSweep).
# PM_AZQUEUE_DIR overrides the folder (self-test only).
$script:QMutex = "Global\pm_azqueue"
$script:QStaleBootSec = 60  # AZT11-2 (b): heartbeat older than this
$script:QCapGraceSec = 30   # AZT11-2 (c): running past scenario cap plus this

function Get-QueueDir {
    $d = if ($env:PM_AZQUEUE_DIR) { $env:PM_AZQUEUE_DIR } else { Join-Path $script:AzRepo "build3ds\azslots\queue" }
    New-Item -ItemType Directory -Force -Path $d | Out-Null
    return $d
}
function Get-NowMs { return [long]([DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds()) }
function Get-ProcStart([int]$id) {
    if ($id -le 0) { return [long]0 }
    try { return (Get-Process -Id $id -ErrorAction Stop).StartTime.Ticks } catch { return [long]0 }
}
function Test-ProcAlive([int]$id, [long]$start) {
    # alive, and (when start is known) the same process as when it was recorded (a reused pid is gone)
    if ($id -le 0) { return $false }
    $ticks = Get-ProcStart $id
    if ($ticks -eq 0) { return $false }
    return ($start -eq 0 -or $ticks -eq $start)
}
function Get-ParentPid([int]$id) {
    try { return [int](Get-CimInstance Win32_Process -Filter "ProcessId=$id" -ErrorAction Stop).ParentProcessId } catch { return 0 }
}

function Invoke-QLocked([scriptblock]$body) {
    # Runs $body under the queue mutex. Results go to $script: variables. A lock wait over 20 s skips the body.
    $m = New-Object Threading.Mutex($false, $script:QMutex)
    $got = $false
    try { $got = $m.WaitOne(20000) } catch [Threading.AbandonedMutexException] { $got = $true }
    try { if ($got) { & $body } else { Write-Host "queue: lock wait over 20 s, skipped" } }
    finally { if ($got) { try { $m.ReleaseMutex() } catch {} }; $m.Dispose() }
}

function Save-QEntry([string]$path, $e) {
    $tmp = "$path.$PID.tmp"
    ($e | ConvertTo-Json -Depth 4) | Set-Content $tmp -Encoding UTF8
    Move-Item $tmp $path -Force
}
function Read-QEntry([string]$path) {
    if (-not (Test-Path $path)) { return $null }
    try { $e = Get-Content $path -Raw | ConvertFrom-Json; $e | Add-Member -NotePropertyName path -NotePropertyValue $path -Force; return $e } catch { return $null }
}
function Read-QEntries {
    # All entries (no lock), oldest first (t, then file name)
    $out = @()
    foreach ($f in @(Get-ChildItem (Get-QueueDir) -Filter "*.json" -File -ErrorAction SilentlyContinue)) {
        $e = Read-QEntry $f.FullName
        if ($e) { $out += $e }
    }
    return @($out | Sort-Object @{ e = { [long]$_.t } }, @{ e = { [string]$_.path } })
}
function Set-QFields([string]$path, [hashtable]$fields) {
    # Under the queue mutex: sets fields of one entry (read, change, write). A missing entry: nothing.
    Invoke-QLocked {
        $e = Read-QEntry $path
        if (-not $e) { return }
        foreach ($k in $fields.Keys) { $e | Add-Member -NotePropertyName $k -NotePropertyValue $fields[$k] -Force }
        Save-QEntry $path $e
    }
}
function Set-QRunClock([string]$path) {
    # AZT12: runT is the start of the scenario clock, the clock run_azahar.ps1 cuts its cap from ($start: a warm round's start (warm.start),
    # or the cold launch after a failed warm restart). Not the process start (azt.ps1 Set-QFields at the Start-Process): a failed warm restart
    # takes up to 66 s per round before the cold start, and that time is not in the run's cap. Called by azt.ps1 at each of those starts.
    Set-QFields $path @{ runT = (Get-NowMs) }
}
function New-QEntry([string]$scenario, [string]$row, [string]$lane, [string]$speed, [string]$tree, [int]$want, [long]$t) {
    $par = Get-ParentPid $PID
    return [pscustomobject][ordered]@{
        t = $t; tree = $tree; lane = $lane; speed = $speed; scenario = $scenario; row = $row; want = $want
        owner = [int]$PID; ownerStart = (Get-ProcStart $PID); parent = $par; parentStart = (Get-ProcStart $par)
        hb = (Get-NowMs); state = "wait"; slot = 0; runner = 0; runnerStart = [long]0; cap = 0; runT = [long]0; skipped = 0
        slots = (Get-SlotCount) # AZT12-4: the slot count of the enqueuer's tools; a slot above it is not granted to this entry (older tools: 2)
    }
}

function Get-StaleReason($e, [long]$now) {
    # AZT11-2: "a" owner (or its parent) gone; "b" heartbeat older than 60 s; "c" running past its cap plus 30 s. "" when fresh.
    if (-not (Test-ProcAlive ([int]$e.owner) ([long]$e.ownerStart))) { return "a" }
    if ([int]$e.parent -gt 0 -and -not (Test-ProcAlive ([int]$e.parent) ([long]$e.parentStart))) { return "a" }
    if (($now - [long]$e.hb) -gt ($script:QStaleBootSec * 1000)) { return "b" }
    if ([string]$e.state -eq "running" -and ($now - [long]$e.runT) -gt (([int]$e.cap + $script:QCapGraceSec) * 1000)) { return "c" }
    return ""
}

function Stop-QRunner($e, [string]$code) {
    # Boots a stale run: stop.request in its slot (run_azahar.ps1 ends the run and keeps a healthy Azahar, paused), then the
    # run_azahar process is killed when it has not ended in 10 s. Written to pool.log.
    $slot = [int]$e.slot
    $rid = [int]$e.runner; $rst = [long]$e.runnerStart
    if (-not (Test-ProcAlive $rid $rst)) { return }
    $sdir = if ($slot -gt 0) { (Get-SlotPaths $slot).dir } else { "" }
    if ($sdir) { New-Item -ItemType Directory -Force -Path $sdir | Out-Null; Set-Content (Join-Path $sdir "stop.request") "stale run booted: $code" }
    $how = "ended on stop request"
    for ($i = 0; $i -lt 40 -and (Test-ProcAlive $rid $rst); $i++) { Start-Sleep -Milliseconds 250 }
    if (Test-ProcAlive $rid $rst) { Stop-Process -Id $rid -Force -ErrorAction SilentlyContinue; $how = "killed after 10 s" }
    if ($sdir) { Remove-Item (Join-Path $sdir "stop.request") -Force -ErrorAction SilentlyContinue }
    if ($slot -gt 0) { Write-PoolLog $slot "stale" $rid "-" ("stale run booted: {0} ({1}), {2} {3}" -f $code, $how, $e.scenario, $e.row) }
    Write-Host ("queue: stale run booted ({0}): {1} row {2} slot {3}, {4}" -f $code, $e.scenario, $e.row, $slot, $how)
}

function Invoke-QSweep {
    # AZT11-2: removes stale entries and boots their runs. Called on every enqueue and every wait poll (azt.ps1) and by azreap.ps1.
    Invoke-QLocked {
        $now = Get-NowMs
        foreach ($e in @(Read-QEntries)) {
            $r = Get-StaleReason $e $now
            if (-not $r) { continue }
            if ([string]$e.state -ne "wait") { Stop-QRunner $e $r }
            Remove-Item $e.path -Force -ErrorAction SilentlyContinue
            if ([string]$e.state -eq "wait") { Write-Host ("queue: stale waiting entry removed ({0}): {1} row {2}" -f $r, $e.scenario, $e.row) }
        }
    }
}

function Get-SlotLane([int]$k) {
    # The key of the ROM and speed the slot's Azahar has loaded: "<lane>|<turbo|normal>" ("" = never run). Written by run_azahar.ps1.
    $f = Join-Path (Get-SlotPaths $k).dir "lane.txt"
    if (Test-Path $f) { return ([string](Get-Content $f -Raw)).Trim() }
    return ""
}
function Test-SlotFree([int]$k) {
    if (-not (Enter-Slot $k 0)) { return $false }
    Exit-Slot $k
    return $true
}

function Invoke-QGrant {
    # AZT11-1: for every free slot (no granted or running entry on it, mutex free), grant the oldest waiting entry that may use it:
    #  1. the oldest entry that was passed over twice already (skipped >= 2): never more than 2 turns behind a younger one
    #  2. else the oldest entry whose lane and speed are the ROM and speed the slot holds (saves a reload and a cold start)
    #  3. else the oldest entry (FIFO)
    # Each entry older than the granted one gets skipped + 1. An entry with want > 0 (a GPU retry) is granted only its slot.
    Invoke-QLocked {
        $n = Get-SlotCount
        $ents = @(Read-QEntries)
        for ($k = 1; $k -le $n; $k++) {
            if (@($ents | Where-Object { ([string]$_.state -eq "granted" -or [string]$_.state -eq "running") -and [int]$_.slot -eq $k }).Count) { continue }
            # AZT12-4: an entry of older tools (no slots field: cap 2) is not granted slot 3, its Get-Slot would wait for its own cap
            $waiting = @($ents | Where-Object { [string]$_.state -eq "wait" -and ([int]$_.want -eq 0 -or [int]$_.want -eq $k) -and $k -le $(if ($_.PSObject.Properties.Name -contains "slots") { [int]$_.slots } else { 2 }) })
            if (-not $waiting.Count) { continue }
            if (-not (Test-SlotFree $k)) { continue }
            $key = Get-SlotLane $k
            $pick = @($waiting | Where-Object { [int]$_.skipped -ge 2 } | Select-Object -First 1)
            if (-not $pick.Count) { $pick = @($waiting | Where-Object { ([string]$_.lane + "|" + [string]$_.speed) -eq $key } | Select-Object -First 1) }
            if (-not $pick.Count) { $pick = @($waiting | Select-Object -First 1) }
            $pick = $pick[0]
            foreach ($w in $waiting) {
                if ([long]$w.t -lt [long]$pick.t) { $w.skipped = [int]$w.skipped + 1; Save-QEntry $w.path $w }
            }
            $pick.state = "granted"; $pick.slot = $k
            Save-QEntry $pick.path $pick
        }
    }
}

function Get-QueueSummary {
    # AZT11-3: the queue and the running runs, one line each (azt.ps1 -Queue; bench.py prints the first line on a start)
    $ents = @(Read-QEntries)
    $now = Get-NowMs
    $run = @($ents | Where-Object { [string]$_.state -eq "running" -or [string]$_.state -eq "granted" })
    $wait = @($ents | Where-Object { [string]$_.state -eq "wait" })
    $lines = @()
    $lines += ("QUEUE: {0} waiting, {1} running or granted, {2} Azahar slots" -f $wait.Count, $run.Count, (Get-SlotCount))
    foreach ($e in $run) {
        $age = [math]::Round((($now - [long]$e.runT) / 1000), 0)
        $alive = Test-ProcAlive ([int]$e.owner) ([long]$e.ownerStart)
        $ownerTxt = if ($alive) { "alive" } else { "gone" }
        $lines += ("  run   slot {0} lane '{1}' {2} row {3} age {4} s state {5} owner {6}" -f $e.slot, $e.lane, $e.scenario, $e.row, $age, $e.state, $ownerTxt)
    }
    $i = 0
    foreach ($e in $wait) {
        $i++
        $waited = [math]::Round((($now - [long]$e.t) / 1000), 0)
        $alive = Test-ProcAlive ([int]$e.owner) ([long]$e.ownerStart)
        $ownerTxt = if ($alive) { "alive" } else { "gone" }
        $lines += ("  wait  {0} lane '{1}' {2} row {3} waited {4} s owner {5}" -f $i, $e.lane, $e.scenario, $e.row, $waited, $ownerTxt)
    }
    return $lines
}


# AZT13: did the Azahar of this emulator log boot a game after byte offset $off? Its BootGame line ("Azahar starting...") proves that a
# restart key was taken, before the game's own log shows anything.
function Test-EmuBoot([string]$emuLog, [long]$off) {
    try {
        $fs = [IO.File]::Open($emuLog, 'Open', 'Read', 'ReadWrite')
        try {
            if ($fs.Length -le $off) { return $false }
            [void]$fs.Seek($off, 'Begin')
            return (New-Object IO.StreamReader($fs)).ReadToEnd().Contains(":BootGame:")
        } finally { $fs.Close() }
    } catch { return $false }
}
