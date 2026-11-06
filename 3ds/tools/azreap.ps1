<#
azreap.ps1 - closes the Azahar that a run kept open (-Keep) in slot <k> after -IdleSec seconds without a run (AZT-2).
Started hidden by run_azahar.ps1. One reaper per slot (mutex Global\pm_azahar_reaper<k>); it ends when the kept Azahar is gone.
#>
param([int]$Slot, [int]$IdleSec = 180)
$ErrorActionPreference = "Stop"
. "$PSScriptRoot\azslot.ps1"
$rm = New-Object Threading.Mutex($false, "Global\pm_azahar_reaper$Slot")
$got = $false
try { $got = $rm.WaitOne(0) } catch [Threading.AbandonedMutexException] { $got = $true }
if (-not $got) { exit 0 }
$s = Get-SlotPaths $Slot
while ($true) {
    Start-Sleep -Seconds 15
    $kept = Get-KeptAzahar $Slot
    if (-not $kept) { break }
    if (-not (Test-Path $s.keepFile)) { continue }
    $age = ((Get-Date) - (Get-Item $s.keepFile).LastWriteTime).TotalSeconds
    if ($age -gt $IdleSec -and (Enter-Slot $Slot 0)) {
        # slot free and idle: the kept Azahar closes
        $age2 = ((Get-Date) - (Get-Item $s.keepFile -ErrorAction SilentlyContinue).LastWriteTime).TotalSeconds
        if ($age2 -gt $IdleSec) { Stop-Process -Id $kept.Id -Force -ErrorAction SilentlyContinue; Remove-Item $s.keepFile -ErrorAction SilentlyContinue }
        Exit-Slot $Slot
        break
    }
}
