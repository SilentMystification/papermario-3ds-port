# Row ROWLINT3: rowlint.py check i (marker): a copied scenario keeps a first_marker its debug string can print

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names. ORACLE8 2026-11-13.)
## Facts (ORACLE8, main ff792275a)
- DRECSTEP stopped with "SCENARIO FAILED: marker missing: prof=1 log=off": its scenario copied spot_mac01_drec
  (azt.d/HOSTCORE2.json, first_marker `prof=1 log=off`) and its debug string had no `prof=`. The game prints that marker only in
  prof mode (pm_log.c:655-658). fxpic_89 (azt_scenarios.json) has first_marker `warp: mac_01 entry 0` and expects `warp`.
- 3ds/tools/rowlint.py: check_symbols (:184) and check_needs (:235) are the pattern; main (:247) calls them; the kinds line is :7;
  registries(rid) reads the four name registries.
Start: `sh 3ds/tools/wt_new.sh ROWLINT3`. PM_ROW=ROWLINT3 PM_LANE=ROWLINT3 on every call.
## Touch only (places)
3ds/tools/rowlint.py: a new function check_marker after check_needs, its call in main, the kinds line (:7). And the
new fixture 3ds/tools/rowlint_fixtures/marker.md. New names: none. Host link: none.
## Exact edits
1. check_marker(lines, faults): in "## Exact edits", for each line (or the 3 lines after it) that says `copy of <X>` or `copies of
   <X>` with X a scenario name in azt_scenarios.json or azt.d/*.json (HEAD): take X's first_marker. When it starts with `prof=`
   and the backtick `debug` string of that edit has no `prof=`, and the same edit names no other first_marker, append
   `('marker', '<X> first_marker <m> needs prof= in the debug string')`.
2. The kinds line: add marker (i).
3. The new fixture file 3ds/tools/rowlint_fixtures/marker.md: a fixture row (as host.md) whose edit 1 reads "a copy of
   spot_mac01_drec with `debug` `warp=mac_01:0 pos=0:0:200 loglevel=debug det drec=412:2`".
4. Commit: `ROWLINT3: rowlint check i (marker) for copied scenarios`.
## Gate (expected values)
1. On the new fixture: `python 3ds/tools/rowlint.py 3ds/tools/rowlint_fixtures/marker.md`: exactly one `rowlint FAULT marker`,
   exit 1 (the seeded fault).
2. The old fixtures give their old results (good.md: faults 0). docs/handoff/row_DRECSTEP.md, row_BATCHTEMPLATE.md and
   row_SEEDPROBE1.md give no marker fault (they copy fxpic_89 now).
3. Budget 0 Azahar starts. Counter: none moves (tool row).
## Stop and end
A fixture result other than the gate, or a tool fault: stop, commit, hand back the exact output. Delete this row file at the end.
