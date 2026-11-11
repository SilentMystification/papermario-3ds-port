# Row TOOLB3: the two runs of a base (and every azt list) stay on one slot

(Model: Haiku. The agent reads only this file, CLAUDE.md and what this file names.)
## Facts (HC4f3, 2026-11-10)
`bench.py base spot_mac01_det --row HC4f3` at 5d61fda6a: "two runs differ, not stored (0000_1_2.ppm differs)"; both runs LOG CHECKS PASS.
Run 212906 was on slot 2 (cold, log build3ds/lane-base_5d61fda6a/slot2/last_log.txt); run 213727 was on slot 1 (warm, .../slot1/...).
Row ESCAPE2 ran an Azahar start (slot 1) at the same time.
## Cause (oracle code read)
- bench.py base_run (:1476-1509) runs the bench twice as one azt list and assumes one kept slot (docstring :1477; azt.ps1:273-274, :289).
- azt.ps1:290-297 starts one child azt.ps1 per list item. Each child enters the machine-wide queue again (azt.ps1:518-545) with
  `$wantSlot` 0 (azt.ps1:520: only a GPU retry asks for a slot), so it gets any free slot. When another worktree holds a slot, the
  second run lands on another slot: another Azahar instance and another slot sdmc (save.flash stays per slot: azslot.ps1:122 never
  copies it), so the two shots can differ. base_run (:1498-1503) compares them with no slot check.
## Start
`sh 3ds/tools/wt_new.sh TOOLB3` (build3ds/wt/TOOLB3, branch TOOLB3 from main). PM_ROW=TOOLB3 PM_LANE=TOOLB3 on every bench call.
## Touch only
3ds/tools/azt.ps1, 3ds/tools/bench.py (base_run only).
## Exact edits
1. azt.ps1 list block: before `foreach ($n in $names) {` (line 290) add
   `$listSlotFile = Join-Path $env:TEMP ("azt_list_slot_{0}.txt" -f $PID); Remove-Item $listSlotFile -ErrorAction SilentlyContinue`
   `$env:PM_AZT_LIST_SLOTFILE = $listSlotFile; $env:PM_AZT_LIST_SLOT = "" # TOOLB3: the first child writes its slot, the next children ask for it`
   In the loop, after `$rc = $LASTEXITCODE`: `if (-not $env:PM_AZT_LIST_SLOT -and (Test-Path $listSlotFile)) { $env:PM_AZT_LIST_SLOT = (Get-Content $listSlotFile -First 1).Trim() }`
   After the loop (before the LIST DONE line): `Remove-Item $listSlotFile -ErrorAction SilentlyContinue; $env:PM_AZT_LIST_SLOTFILE = ""; $env:PM_AZT_LIST_SLOT = ""`
2. azt.ps1 line 520 becomes
   `$wantSlot = if ($env:PM_AZT_RETRY_RUN -and $env:PM_AZT_RETRY_SLOT) { [int]$env:PM_AZT_RETRY_SLOT } elseif ($env:PM_AZT_LIST_SLOT) { [int]$env:PM_AZT_LIST_SLOT } else { 0 } # TOOLB3: a list child asks for the slot of the first child`
3. azt.ps1 after the line `Write-Output ("queue: granted slot {0} after ...` (line 553) add
   `if ($env:PM_AZT_LIST_SLOTFILE -and -not $env:PM_AZT_LIST_SLOT) { Set-Content $env:PM_AZT_LIST_SLOTFILE ([string]$slotK) } # TOOLB3`
4. bench.py base_run, after `first, last = recs[-2], recs[-1]` (line 1498) add:
   `sa, sb = re.search(r"slot(\d+)", first.get("log_path") or ""), re.search(r"slot(\d+)", last.get("log_path") or "")`
   `if not sa or not sb or sa.group(1) != sb.group(1):`
   `    print("base %s: TOOL FAULT: the two runs ran on slots %s and %s, not stored." % (bd["name"], sa.group(1) if sa else "?", sb.group(1) if sb else "?"))`
   `    bad += 1`
   `    continue`
## Gate (no game code changes: no byte compare of a picture change)
- `powershell -NoProfile -File 3ds/tools/azt.ps1 -SelfTest`: the same PASS lines as on main (run it on main first, 0 Azahar starts).
- Slot pin: PASSED at 9f223e5ac. `bench.py base spot_mac01_det --row TOOLB3` (base e18160cad, output build3ds/out/toolb3_base.txt):
  "want slot 1" on the second child, runs 215370 and 216191 both on slot 1. Both were cold starts ("warm restart failed").
  The shots still differ (0000_1_2.ppm). Same slot, both cold, same build, so this is not a slot or tool fault of this row: row DETBASE.
- Next: no more runs. Commit the runs.jsonl lines, delete this row file, hand back the ready hash (9f223e5ac + that commit).
## Stop and end
A failed gate, an edit that does not fit the code, or a test-tool fault: commit and hand back the exact output (CLAUDE.md 4-5).
At the end delete this row file in the completing commit. Hand back: the ready hash and the gate results.
