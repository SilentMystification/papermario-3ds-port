# Row FRAMEHASH4: bench.py stores and compares frame hashes (fhash.txt) as it does shots, no image

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names.)
## Facts (ORACLE8, main 5a53dd0a2, 3ds/tools/bench.py)
- A run's shot folder is SHOTS/<scenario>/<lane> (cmd_ingest_last :2030). snap_shots (:1511) copies it for a base list,
  shots_diff (:1526) compares two runs, store_base (:1481) copies every file of it, stored (:1463), row_shots_run (:1908),
  cmd_cmp (:1918) and stored_matches (:1968) read it. They find shots by the glob "*.ppm" at :1467, :1517, :1528, :1529, :1815,
  :1913, :1935, :1936, :1971, :1977.
- FRAMEHASH3 (later) prints one game entry per frame: `fhash f<frame> h <16 hex> draws <n> words <w>`. This row turns those
  entries of a run's log into the file fhash.txt in the shot folder, so the base store and cmp handle it like a shot.
Start: `sh 3ds/tools/wt_new.sh FRAMEHASH4`. PM_ROW=FRAMEHASH4 PM_LANE=FRAMEHASH4 on every call.
## Touch only (places)
3ds/tools/bench.py: a new block of 3 functions right before `def shots_diff` (:1526), the 10 glob lines of the Facts, the
compare loop of cmd_cmp (:1941-1959), cmd_ingest_last (:2030-2031). New 3ds/tests/fhashcmp/run.sh and test_fhashcmp.py.
## Exact edits
1.-3. Done in 91666e3ee (FHASH_RX, shot_names, fhash_extract, fhash_cmp before shots_diff, the 10 globs, the cmd_cmp loop).
4. Correction 1 (ORACLE8): a bench kind is a free string of the bench entry ("kind" in bench_defs.json or bench.d/<row>.json).
   bench.py has no kind list: every path tests `kind.startswith("azahar")` (:872, :901, :988, :1200, :1371, :1807, :1920, :2023).
   So "azahar_hash" needs no declaration in bench.py. FRAMEHASH5 declares it in its bench entries ("kind": "azahar_hash" in
   3ds/tools/bench.d/FRAMEHASH5.json). Until then no bench has the kind and the call below writes nothing (by design).
   a. After fhash_cmp add:
```
def ingest_fhash(bd, log, shot):
    """FRAMEHASH4: a run of a bench of kind azahar_hash (kind of its bench entry, FRAMEHASH5) gets fhash.txt. Returns the count, 0 for other kinds."""
    return fhash_extract(log, shot) if bd and bd.get("kind") == "azahar_hash" else 0
```
   b. cmd_ingest_last: right after the line `shot = a.shots or str(SHOTS / s / ...)` add `ingest_fhash(bd, p, shot)` (before
      snap_shots, so the base copy holds fhash.txt).
5. Done in 91666e3ee (9 cases). Add 2 cases to test_fhashcmp.py: ingest_fhash({"kind": "azahar_hash"}, log, tmp1) == 3 with
   tmp1/fhash.txt present, and ingest_fhash({"kind": "azahar_pic"}, log, tmp2) == 0 with no tmp2/fhash.txt.
6. Commit: `FRAMEHASH4: bench.py fhash.txt in the shot folder, base and cmp compare frame hashes`.
## Gate (expected values)
1. Seeded fault (done for edits 1-3, 5). For edit 4: add the 2 cases first, run `sh 3ds/tests/fhashcmp/run.sh` -> FAIL (no
   ingest_fhash). Then edit 4, run again -> 11 PASS lines, exit 0.
2. `sh 3ds/tests/basekey/run.sh` and `sh 3ds/tests/benchpath/run.sh`: pass as before (the *.ppm path is unchanged).
3. No Azahar start. Counter: none (harness row).
## Stop and end
A failed gate, a line that does not match the text above, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5).
Delete this row file in the completing commit.
