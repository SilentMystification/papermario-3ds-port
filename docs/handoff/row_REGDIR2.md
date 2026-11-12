# Row REGDIR2: scenarios and benches as one file per row (3ds/tools/azt.d/, 3ds/tools/bench.d/), merged at load

(Model: Haiku. Three readers and one writer, each named with its line; the existing entries do not change.)

## Code read (ORACLE7, main 5be532ef9): every reader and writer
- Readers of azt_scenarios.json: 3ds/tools/azt.ps1:44 (`$scs = ... .scenarios`), :124 (self-test base copy of toad_pond),
  3ds/tools/bench.py:89 SCEN and :219-223 scenario(). Writer: azt.ps1:451-470 Set-ScenarioStatus (text edit of one entry's status).
- Readers of bench_defs.json: bench.py:88 DEFS, :208-209 load_defs (benches, profiles, consoles, push_base: callers :186, :394,
  :826, :851-863), :212-217 bench_def; 3ds/push_3ds.sh:80 (profiles only). go.ps1:10 and test_catalog.md name the file in text.
- Today every new scenario and bench is inserted into the two shared files (the S1TSTEN and S7EFX5/8 conflicts).

## Start
`sh 3ds/tools/wt_new.sh REGDIR2`. PM_ROW=REGDIR2.

## Touch only
3ds/tools/azt.ps1 (:44, :124, :451-470), 3ds/tools/bench.py (:88-89, :208-223 only), new 3ds/tools/azt.d/ and 3ds/tools/bench.d/ (one
README.txt each), docs/handoff/test_catalog.md (one paragraph). The two json files keep their content (the main part).

## Exact edits
1. Format: `3ds/tools/azt.d/<row>.json` = `{"scenarios": [ ... ]}`; `3ds/tools/bench.d/<row>.json` = `{"benches": [ ... ]}`. Load
   order: the main file first, then the directory files sorted by name. A name defined twice (in any two files) is an error that
   names both files; nothing runs.
2. bench.py: `load_defs()` returns the main dict with `benches` extended by every bench.d file; `scenario()` reads the merged list
   (one helper `all_scenarios()`). push_3ds.sh stays (profiles live only in the main file).
3. azt.ps1 :44 and :124: the merged list (main plus azt.d/*.json sorted). Set-ScenarioStatus: find the file that holds the name
   (main or one azt.d file) and edit that file in place as today.
4. README.txt in each directory: "a row adds its own <row>.json here; never edit another row's file or the main file".
5. test_catalog.md: one paragraph with the rule above.

## Gate (expected values)
- `python 3ds/tools/bench.py list`: the same bench names as before (count equal; hand back the count).
- `powershell -File 3ds/tools/azt.ps1 -SelfTest`: PASS (as before).
- A test pair in a temp copy: one azt.d and one bench.d file with a new scenario `regdir_probe` (a copy of toad_pond) is listed by
  `bench.py show regdir_probe`; a second file with the same name gives the duplicate error. Delete the pair after the test.
- No Azahar start. Budget 0.

## Stop
A changed bench list or a self-test failure: stop, commit, hand back. Delete this row file in the completing commit.
