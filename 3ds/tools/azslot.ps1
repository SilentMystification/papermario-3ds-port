<#
azslot.ps1 - Azahar slot pool (AZT-1). Dot-sourced by 3ds/run_azahar.ps1 and 3ds/tools/azt.ps1.

A slot is a private portable copy of the Azahar install: build3ds/azslots/slot<k>/ (gitignored) with azahar.exe and a
`user` folder next to it. Azahar then keeps config, nand, sdmc, shaders and logs in slot<k>/user and nothing in %APPDATA%.
One named mutex per slot (Global\pm_azahar_slot<k>) lets one run use a slot at a time. N slots: env PM_AZ_SLOTS (default 2; the GPU is shared with other apps).
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
$script:AzIdleSec = 900

# AZT4-1: the cap on live azahar.exe processes of the slot pool, all slots of all worktrees and agents. A constant, not an env value.
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

# AZT8-1: the build id of a 3dsx (the string PM_BUILD_STR, e.g. 2500-off1), read from the file. "" when not found.
function Get-Build3dsx([string]$path) {
    $t = [Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($path))
    $m = [regex]::Match($t, '(\d{3,5}-[A-Za-z0-9_]+)\x00')
    if ($m.Success) { return $m.Groups[1].Value }
    return ""
}

# AZT9d-2: the value of the newest boot's line matching $pattern. A boot starts with the line "#0 log: writer" (the log is re-created or truncated).
# Before the restart the caller takes the file length and the marker count ($oldLen, $oldMarks). The new boot is: the text after the last
# marker when the file got shorter (truncated in place), else the text after the last marker once the marker count rises. $null after 15 s.
function Get-BootValue([string]$log, [string]$pattern, [long]$oldLen, [int]$oldMarks) {
    for ($w = 0; $w -lt 60; $w++) {
        if (Test-Path $log) {
            $t = ""
            try { $fs = [IO.File]::Open($log, 'Open', 'Read', 'ReadWrite'); try { $t = (New-Object IO.StreamReader($fs)).ReadToEnd() } finally { $fs.Close() } } catch { $t = "" }
            $marks = @([regex]::Matches($t, '(?m)^#0 log: writer')).Count
            $truncated = $t.Length -lt $oldLen
            if ($marks -gt 0 -and ($truncated -or $marks -gt $oldMarks)) {
                $last = [regex]::Matches($t, '(?m)^#0 log: writer') | Select-Object -Last 1
                $seg = $t.Substring($last.Index)
                $m = [regex]::Match($seg, $pattern)
                if ($m.Success) { return $m.Groups[1].Value }
            }
        }
        Start-Sleep -Milliseconds 250
    }
    return $null
}
function Get-LogState([string]$log) {
    # length and marker count of the log now (0, 0 when none)
    if (-not (Test-Path $log)) { return @{ len = [long]0; marks = 0 } }
    $t = ""
    try { $fs = [IO.File]::Open($log, 'Open', 'Read', 'ReadWrite'); try { $t = (New-Object IO.StreamReader($fs)).ReadToEnd() } finally { $fs.Close() } } catch { return @{ len = [long]0; marks = 0 } }
    return @{ len = [long]$t.Length; marks = @([regex]::Matches($t, '(?m)^#0 log: writer')).Count }
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

function Get-WarmState([int]$k, [bool]$turbo, [int]$turbolimit) {
    # AZT2: can a run restart in slot k? Returns "warm" or the reason it cannot:
    # "no kept" (no Azahar kept in the slot), "closed" (keep.json, but its Azahar is gone), "turbo" (other turbo setting).
    # AZT3-1: no lane check. Every lane's 3dsx is copied to the slot rom path and the restart loads that path.
    $kept = Get-KeptAzahar $k
    $info = Get-KeepInfo $k
    if (-not $kept) { if ($info) { return "closed" } else { return "no kept" } }
    if (-not $info -or [int]$info.pid -ne $kept.Id) { return "no kept" }
    if ([bool]$info.turbo -ne $turbo -or [int]$info.turbolimit -ne $turbolimit) { return "turbo" }
    return "warm"
}

function Get-Slot([int]$want, [int]$waitSec, [bool]$turbo = $false, [int]$turbolimit = 0) {
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
        $order = if ($capFull) { $warmK + $liveK + $coldK } else { $warmK + $coldK + $liveK }
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
if (-not ([System.Management.Automation.PSTypeName]'AzKey').Type) {
    Add-Type @"
using System; using System.Runtime.InteropServices;
public class AzKey {
  [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
  [DllImport("user32.dll")] public static extern uint MapVirtualKey(uint c, uint t);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int n);
  [DllImport("user32.dll")] public static extern bool IsIconic(IntPtr h);
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

# AZT3-1: Azahar's shortcuts (Restart Emulation, Pause) are window shortcuts: Qt fires them only while Azahar is the active window.
# A posted key alone did nothing when another window had the focus (the warm restart then fell back to a cold start).
if (-not ([System.Management.Automation.PSTypeName]'AzFg').Type) {
    Add-Type @"
using System; using System.Runtime.InteropServices;
public class AzFg {
  [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, IntPtr pid);
  [DllImport("user32.dll")] public static extern bool AttachThreadInput(uint a, uint b, bool f);
  [DllImport("kernel32.dll")] public static extern uint GetCurrentThreadId();
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool BringWindowToTop(IntPtr h);
}
"@
}

function Set-AzForeground([IntPtr]$h) {
    # Makes window h the foreground window (attach to the foreground thread: the foreground lock allows it). Returns $true when it is.
    $fg = [AzFg]::GetForegroundWindow()
    if ($fg -eq $h) { return $true }
    $fgT = [AzFg]::GetWindowThreadProcessId($fg, [IntPtr]::Zero)
    $me = [AzFg]::GetCurrentThreadId()
    [void][AzFg]::AttachThreadInput($me, $fgT, $true)
    [void][AzFg]::BringWindowToTop($h)
    $ok = [AzFg]::SetForegroundWindow($h)
    [void][AzFg]::AttachThreadInput($me, $fgT, $false)
    return $ok
}

function Send-AzHotkey($proc, [string]$seq) {
    # Posts the key to the Azahar main window. Azahar is made the foreground window for the keys, then the previous foreground window gets the focus back.
    $proc.Refresh()
    $h = $proc.MainWindowHandle
    if ($h -eq [IntPtr]::Zero -or -not $seq) { return $false }
    $prevFg = [AzFg]::GetForegroundWindow()
    $script:AzLastFg = [bool](Set-AzForeground $h) # AZT9-1: the key goes to a foreground window only (Qt window shortcuts)
    Start-Sleep -Milliseconds 150
    try { return (Send-AzHotkeyKeys $h $seq) }
    finally {
        Start-Sleep -Milliseconds 150
        if ($prevFg -ne [IntPtr]::Zero -and $prevFg -ne $h) { [void](Set-AzForeground $prevFg) }
    }
}

# AZT9-2: the key as keyboard input to the foreground window (keybd_event), for the warm restart tries after a posted key did not restart.
if (-not ([System.Management.Automation.PSTypeName]'AzKb').Type) {
    Add-Type @"
using System; using System.Runtime.InteropServices;
public class AzKb {
  [DllImport("user32.dll")] public static extern void keybd_event(byte bVk, byte bScan, uint dwFlags, UIntPtr dwExtraInfo);
}
"@
}
function Send-AzKeyEvent([IntPtr]$h, [string]$seq) {
    $mods = @(); $key = ""
    foreach ($part in $seq.Split("+")) {
        switch ($part.ToLower()) { "ctrl" { $mods += 0x11 } "shift" { $mods += 0x10 } "alt" { $mods += 0x12 } default { $key = $part } }
    }
    $vk = 0
    if ($key -match '^F(\d{1,2})$') { $vk = 0x6F + [int]$Matches[1] }
    elseif ($key -match '^[A-Za-z0-9]$') { $vk = [int][char]$key.ToUpper() }
    if (-not $vk) { return $false }
    $prevFg = [AzFg]::GetForegroundWindow()
    [void](Set-AzForeground $h)
    Start-Sleep -Milliseconds 150
    foreach ($m in $mods) { [AzKb]::keybd_event([byte]$m, 0, 0, [UIntPtr]::Zero) }
    [AzKb]::keybd_event([byte]$vk, 0, 0, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds 60
    [AzKb]::keybd_event([byte]$vk, 0, 2, [UIntPtr]::Zero)
    foreach ($m in $mods) { [AzKb]::keybd_event([byte]$m, 0, 2, [UIntPtr]::Zero) }
    Start-Sleep -Milliseconds 150
    if ($prevFg -ne [IntPtr]::Zero -and $prevFg -ne $h) { [void](Set-AzForeground $prevFg) }
    return $true
}
function Send-AzHotkeyKeys([IntPtr]$h, [string]$seq) {
    $mods = @(); $key = ""
    foreach ($part in $seq.Split("+")) {
        switch ($part.ToLower()) { "ctrl" { $mods += 0x11 } "shift" { $mods += 0x10 } "alt" { $mods += 0x12 } default { $key = $part } }
    }
    $vk = 0
    if ($key -match '^F(\d{1,2})$') { $vk = 0x6F + [int]$Matches[1] }
    elseif ($key -match '^[A-Za-z0-9]$') { $vk = [int][char]$key.ToUpper() }
    if (-not $vk) { return $false }
    foreach ($m in $mods) { [void][AzKey]::PostMessage($h, 0x100, [IntPtr]$m, [IntPtr][int64](1 + ([AzKey]::MapVirtualKey($m, 0) -shl 16))) }
    $sc = [AzKey]::MapVirtualKey($vk, 0)
    [void][AzKey]::PostMessage($h, 0x100, [IntPtr]$vk, [IntPtr][int64](1 + ($sc -shl 16)))
    Start-Sleep -Milliseconds 60
    [void][AzKey]::PostMessage($h, 0x101, [IntPtr]$vk, [IntPtr][int64](1 + ($sc -shl 16) + 0xC0000000))
    foreach ($m in $mods) { [void][AzKey]::PostMessage($h, 0x101, [IntPtr]$m, [IntPtr]0) }
    return $true
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
