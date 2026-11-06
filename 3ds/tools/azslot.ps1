<#
azslot.ps1 - Azahar slot pool (AZT-1). Dot-sourced by 3ds/run_azahar.ps1 and 3ds/tools/azt.ps1.

A slot is a private portable copy of the Azahar install: build3ds/azslots/slot<k>/ (gitignored) with azahar.exe and a
`user` folder next to it. Azahar then keeps config, nand, sdmc, shaders and logs in slot<k>/user and nothing in %APPDATA%.
One named mutex per slot (Global\pm_azahar_slot<k>) lets one run use a slot at a time. N slots: env PM_AZ_SLOTS (default 3).
The user's C:\Program Files\Azahar and %APPDATA%\Azahar are only read (copied from), never written.
#>
$script:AzRepo = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$script:AzInstall = "C:\Program Files\Azahar"
$script:AzMaster = "$env:APPDATA\Azahar"
$script:SlotMutex = @{}

function Get-SlotCount { if ($env:PM_AZ_SLOTS) { return [int]$env:PM_AZ_SLOTS } else { return 3 } }

function Get-SlotPaths([int]$k) {
    $d = Join-Path $script:AzRepo "build3ds\azslots\slot$k"
    return @{
        k = $k; dir = $d; exe = "$d\azahar.exe"; user = "$d\user"
        cfg = "$d\user\config\qt-config.ini"; sd = "$d\user\sdmc\3ds\PaperMario"
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

function Get-Slot([int]$want, [int]$waitSec) {
    # $want > 0 takes that slot. 0 takes the first free slot, a slot with a kept Azahar first. Waits only when all slots are busy. Returns the slot number, 0 on timeout.
    $n = Get-SlotCount
    $t0 = Get-Date
    $said = $false
    while ($true) {
        $order = @(if ($want -gt 0) { $want } else {
            $kept = @(1..$n | Where-Object { Get-KeptAzahar $_ })
            $kept + @(1..$n | Where-Object { $kept -notcontains $_ })
        })
        foreach ($k in $order) { if (Enter-Slot $k 0) { return $k } }
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

function Send-AzHotkey($proc, [string]$seq) {
    # Posts the key to the Azahar main window (no focus change). Returns $false when the key or the window is unknown.
    $proc.Refresh()
    $h = $proc.MainWindowHandle
    if ($h -eq [IntPtr]::Zero -or -not $seq) { return $false }
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
    # A hidden process closes the kept Azahar of slot k after 10 idle minutes (one reaper per slot: a mutex)
    $ps = (Get-Process -Id $PID).Path
    Start-Process $ps -ArgumentList "-NoProfile -ExecutionPolicy Bypass -File `"$PSScriptRoot\azreap.ps1`" -Slot $k" -WindowStyle Hidden | Out-Null
}
