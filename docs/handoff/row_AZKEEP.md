# Row AZKEEP: keep and reuse the Azahar sessions (option runs take a free slot, one key resend, causes named)

(Model: Haiku. Base: main. Tool only: 3ds/tools/azslot.ps1 (Get-Slot), 3ds/tools/azt.ps1 (its Get-Slot call), 3ds/run_azahar.ps1
(:238 rule, the warm wait loop :352-359). Disjoint from every code row. Reads this file, test_catalog.md lines 56-60 and CLAUDE.md.)

## Facts (azstat since 2026-11-11: 373 warm, 18 cold; boot 14.7 s cold vs 2.6 s warm) and causes (oracle code read)
- "other lane" (20 entries in the slot logs): the current code has no lane check (azslot.ps1:161-162 Get-WarmState "AZT3-1: no lane
  check. Every lane's 3dsx is copied to the slot rom path"). The entries come from tool copies older than AZT3-1. No code change; the gate
  shows none on the current tools.
- "option" (7, "kill: kept replaced"): run_azahar.ps1:238 makes a run with -NoWarm, -Interactive, -Gdb or -FaultStop cold, and the slot
  it got may hold a kept Azahar, which is then killed. Get-Slot (azslot.ps1:171-199) orders warm slots first, so such a run lands on a
  kept one. Fix: a cold-needing run asks Get-Slot for a slot without a kept Azahar first.
- "restart failed" / "warm restart key not taken" (6): the key (Send-AzHotkey, azslot.ps1:230: focus messages and the key to the kept
  pid's window) gave no BootGame entry in the emulator file within 8 s (run_azahar.ps1:356). The code read cannot name why Qt dropped
  it; the slot's warmfail_emu.txt and warmfail_log.txt (written at :372) hold the evidence. Fix: one resend of the key at 3 s without a
  boot (a late first key boots once more; the wait looks for a boot and a rotation after the last key).
- "no kept" (5): the first start of a slot, a slot whose Azahar the reaper closed after 60 min idle (azreap.ps1, AzIdleSec 3600), or a
  run that crashed or exited (no keep). Expected; the gate names each.

## Start
`sh 3ds/tools/wt_new.sh AZKEEP`. PM_ROW=AZKEEP on every call.

## Exact edits
1. azslot.ps1 Get-Slot: add the parameter `[bool]$needCold = $false`; when it is true the order is `$coldK + $liveK + $warmK` (a free slot
   without a kept Azahar first), else as now. Comment `AZKEEP: a run that must start cold does not replace a kept session when a free slot has none`.
2. azt.ps1: at its Get-Slot call pass `-needCold` true when the scenario's extra args or the run's switches hold -NoWarm, -Interactive,
   -Gdb or -FaultStop (the list of run_azahar.ps1:238). run_azahar.ps1:210 passes the same for a direct call.
3. run_azahar.ps1 warm wait loop: `$resent = $false` before it; inside, after the boot test: `if (-not $booted -and -not $resent -and $el -gt 3)
   { $sent = Send-AzHotkey $kept $hk; $resent = $true; Write-PoolLog $slotK "warm resend" $kept.Id $kept.Id "no BootGame 3 s after the key" }`.
4. When a warm restart fails, copy warmfail_emu.txt and warmfail_log.txt of the slot to build3ds/out/azkeep_warmfail_<slot>_<n>.txt.

## Gate
- `python 3ds/tools/bench.py azstat --row AZKEEP` before (read only) and after one list on all slots: `bench.py run spot_mac01_det,mdl5_iwa10_det,
  spot_mac00_det,spot_mac01_det,mdl5_iwa10_det,spot_mac00_det --row AZKEEP --why "warm reuse"` (6 starts): at most one cold start per slot
  (its reason printed), no "kill: kept replaced", no "other lane". Hand back both azstat outputs and any warmfail copies.

## Stop and end
A tool fault: commit, hand back the exact output (CLAUDE.md 4). Delete this row file in the completing commit. Hand back the hash and the
azstat output.
