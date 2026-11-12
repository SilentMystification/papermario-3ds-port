# TOOLB5 host test: Invoke-QGrant grants a free slot k to a waiting entry that can use only slot k (want = k) before the lane match.
# No Azahar. Temp queue folder (PM_AZQUEUE_DIR), PM_AZ_SLOTS=3 (Get-SlotCount caps it to the live Azahar cap), one Invoke-QGrant call.
# Entries: A oldest (want 0, lane and speed of slot k's lane.txt), B (want 0, other lane), C newest (want k, other lane).
# Expected: C granted slot k. Prints "qgrant PASS" (exit 0), "qgrant FAIL <states>" (exit 1), or "qgrant: no free slot, rerun" (exit 2).
$ErrorActionPreference = "Stop"
$tmp = Join-Path ([IO.Path]::GetTempPath()) ("qgrant_" + [guid]::NewGuid().ToString("N"))
$env:PM_AZQUEUE_DIR = $tmp
$env:PM_AZ_SLOTS = "3"
. (Join-Path $PSScriptRoot "..\..\tools\azslot.ps1")

function Remove-QTmp { Remove-Item $tmp -Recurse -Force -ErrorAction SilentlyContinue }

$n = Get-SlotCount
$k = 0
for ($i = 1; $i -le $n -and $k -eq 0; $i++) { if (Test-SlotFree $i) { $k = $i } }
if ($k -eq 0) { Write-Host "qgrant: no free slot, rerun"; Remove-QTmp; exit 2 }

$lane = Get-SlotLane $k
$cut = $lane.LastIndexOf("|")
$laneName = if ($cut -ge 0) { $lane.Substring(0, $cut) } else { $lane }
$speed = if ($cut -ge 0) { $lane.Substring($cut + 1) } else { "" }
$t0 = Get-NowMs
$A = New-QEntry "qgrantA" "A" $laneName $speed "qgrant" 0 $t0
$B = New-QEntry "qgrantB" "B" "qgrantOther" "normal" "qgrant" 0 ($t0 + 1000)
$C = New-QEntry "qgrantC" "C" "qgrantOther" "normal" "qgrant" $k ($t0 + 2000)
New-Item -ItemType Directory -Force -Path $tmp | Out-Null
Save-QEntry (Join-Path $tmp "a.json") $A
Save-QEntry (Join-Path $tmp "b.json") $B
Save-QEntry (Join-Path $tmp "c.json") $C

Invoke-QGrant

$ea = Read-QEntry (Join-Path $tmp "a.json")
$eb = Read-QEntry (Join-Path $tmp "b.json")
$ec = Read-QEntry (Join-Path $tmp "c.json")
$st = "slot k={0} A={1}/{2} B={3}/{4} C={5}/{6}" -f $k, $ea.state, $ea.slot, $eb.state, $eb.slot, $ec.state, $ec.slot
$ok = ([string]$ec.state -eq "granted") -and ([int]$ec.slot -eq $k)
Remove-QTmp
if ($ok) { Write-Host "qgrant PASS"; exit 0 }
Write-Host ("qgrant FAIL " + $st)
exit 1
