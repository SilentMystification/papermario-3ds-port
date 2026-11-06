# Row AZT2: Azahar slot affinity and warm/cold report

## Goal
AZT-1/2 (e6fd9ca2c, 0feff92a3): a warm restart works only when the slot's kept Azahar ran the same lane 3dsx (keep.json x3dsx) and turbo setting. Runs take the first free slot, so with 5 lanes on 3 slots most runs find another lane's Azahar and start cold (13 to 16 s to the first marker, warm 1.1 s). A closed window falls back to a cold start already (Get-KeptAzahar finds no process).

## Rows
AZT2-1 AFFINITY: 3ds/tools/azslot.ps1 slot choice (the pool function that picks the first free slot): first a free slot whose keep.json x3dsx and turbo match this run, then a free slot with no kept Azahar, then any free slot. AZT2-2 REPORT: run_azahar.ps1 prints `azslot: slot <k> warm` or `azslot: slot <k> cold (<reason>: no kept, other lane, turbo, restart failed, closed)` and azt.ps1 copies it into build3ds/azt/logs/<scenario>.txt and the bench record (field warm 0/1). Gate: a list of 2 runs on lane A, then 1 on lane B, then 1 on lane A: lane A runs 2 and 4 are warm (slot affinity kept A's slot), shown by the lines.

## Rules
Files: 3ds/tools/azslot.ps1, 3ds/run_azahar.ps1, 3ds/tools/azt.ps1, 3ds/tools/bench.py (warm field only). Other agents run tests now: the harness must work after each commit. Pathspec commits, own hunks only. No git add -A, commit -a, reset, stash, checkout, restore. No push, no attribution. Hand back after the gate.
