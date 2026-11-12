# Row TOOLB4: base runs keep one slot after a retry; merge_row.sh stops copying note lines again (runs.jsonl growth)

(Model: Haiku. Tool row, three exact edits. No blocker. The one-time cleanup of the file is row_TOOLDEDUP.md: it waits for the user.)

## Cause (ORACLE6 code read, main 1fb48358e)
- Slot fault: base_run (3ds/tools/bench.py:1517-1567) starts one azt list of two runs (one kept slot: azt.ps1 TOOLB3 :291-308).
  When a run of that list is not warm (a cold start, AZT2), it adds single starts (:1529-1537). Those calls set no slot, so
  azt.ps1:524 asks for any slot and the next start can land on another one. The compare (:1553-1558) then refuses the pair
  (TOOL FAULT). The same-slot rule stays: each slot has its own sdmc (options.ini, saves, cheats file: test_catalog.md, Slots),
  so per-slot state can change bytes. The retry must ask for the list's slot. azslot.ps1:404-411 grants a want > 0 entry only that
  slot, so the run waits for it.
- runs.lock 120 s: append_runs (bench.py:302-324) holds the lock while it parses the main runs.jsonl and every worktree's copy
  (all_tree_runs_files :1029), then rebuild_db parses the main file again (:384-420). Main's runs.jsonl is 1,773,559,010 bytes
  with 5,230,161 lines, of which 4,750 are distinct. Four correction-note lines (FADV run 497 and POND run 480, two spellings
  each) appear 1,017,063 to 1,595,619 times each. The growth comes from merge_row.sh:79-83: a line without run_id
  (`r == ""`) is always appended. A worktree's copy holds all of main's notes, so every merge appends all of them again. The
  count doubles per merge: 583,318 lines at 111132143, 1,161,881 at d52a46720, 2,038,910 at 0a9839d1a.

## Start
`sh 3ds/tools/wt_new.sh TOOLB4`. PM_ROW=TOOLB4 on every call.

## Touch only
3ds/tools/merge_row.sh (the awk of step 5), 3ds/tools/bench.py (base_run only), 3ds/tools/azt.ps1 (line 524 only).

## Exact edits
1. merge_row.sh awk: `FNR == NR { r = rid($0); if (r != "") known[r] = 1; next }` ->
   `FNR == NR { r = rid($0); if (r != "") known[r] = 1; else seen[$0] = 1; next }`, and
   `if (r == "" || !(r in known)) { print; if (r != "") known[r] = 1 }` ->
   `if (r == "") { if (!($0 in seen)) { print; seen[$0] = 1 } else skip++ } else if (!(r in known)) { print; known[r] = 1 }`
   (the `else skip++` of the run branch stays). Comment: "TOOLB4: a note line (no run_id) is appended only when main has no
   equal line".
2. azt.ps1:524: insert `elseif ($env:PM_AZT_BASE_SLOT) { [int]$env:PM_AZT_BASE_SLOT }` after the PM_AZT_LIST_SLOT arm (comment
   "TOOLB4: a base retry asks for the slot of its list").
3. bench.py base_run, right after the first cmd_run (the list) and before `n = BASE_TWICE`: take the slot of the list from the
   newest record of the first bench, `recs = base_warm(bds[0], row, label, commit, since)[0]`, `m = re.search(r"slot(\d+)",
   (recs[-1].get("log_path") or "") if recs else "")`, `if m: os.environ["PM_AZT_BASE_SLOT"] = m.group(1)`. In the `finally:`
   add `os.environ.pop("PM_AZT_BASE_SLOT", None)`.

## Gate (expected values)
- `sh 3ds/tests/merge/run.sh` if it exists (merge_row host test), else a dry check: copy main's runs.jsonl head (first 20000
  lines) and a worktree-like copy (the same lines plus 2 new run lines and the 4 notes again) into build3ds/out/, run the awk of
  step 1 on them: output is exactly the 2 new run lines. Write the commands and the output line count in the commit.
- `python -c "import ast,sys; ast.parse(open('3ds/tools/bench.py').read())"` OK; `powershell -NoProfile -Command "[void]
  [ScriptBlock]::Create((Get-Content 3ds/tools/azt.ps1 -Raw))"` OK.
- One base on a bench that is not stored: `PM_ROW=TOOLB4 python 3ds/tools/bench.py base battle_wheel_nowide_det` (2 or 3
  starts): "two runs equal, stored". If the list gave two warm runs, no retry happened: hand back that fact (the retry arm is
  then shown by the code read only). Budget 4.

## Stop
A test-tool fault or a failed check: stop, commit, hand back the output. Delete this row file in the completing commit.
