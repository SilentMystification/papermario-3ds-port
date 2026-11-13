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
1. Before `def shots_diff` add:
```
FHASH_RX = re.compile(r"fhash f(\d+) h ([0-9a-f]{16}) draws (\d+) words (\d+)")
def shot_names(d):
    """FRAMEHASH4: the compared files of a shot folder: the *.ppm shots and fhash.txt (frame hashes), sorted."""
    d = Path(d)
    return sorted(p.name for p in d.glob("*.ppm")) + (["fhash.txt"] if (d / "fhash.txt").is_file() else [])
def fhash_extract(log, folder):
    """FRAMEHASH4: the fhash entries of a log as folder/fhash.txt, one line `f<n> h <h> draws <d> words <w>` each. Returns the count."""
    rows = ["f%s h %s draws %s words %s" % m.groups() for m in FHASH_RX.finditer(Path(log).read_text(encoding="utf-8", errors="replace"))]
    if rows:
        Path(folder).mkdir(parents=True, exist_ok=True)
        (Path(folder) / "fhash.txt").write_text("\n".join(rows) + "\n", encoding="utf-8")
    return len(rows)
def fhash_cmp(a, b):
    """FRAMEHASH4: (bad, lines) for two fhash.txt files: per frame `f<n> equal` or `f<n> DIFF`, then `<k>/<n> frames equal`."""
    la, lb = Path(a).read_text(encoding="utf-8").split("\n")[:-1], Path(b).read_text(encoding="utf-8").split("\n")[:-1]
    out, eq = [], 0
    for i, x in enumerate(la):
        f = x.split(" ")[0]
        if i < len(lb) and lb[i] == x: eq += 1; out.append("  %s equal" % f)
        else: out.append("  %s DIFF (rerun with fhashdraw=%s to name the draw)" % (f, f[1:]))
    out.append("%d/%d frames equal" % (eq, len(la)))
    return (0 if eq == len(la) == len(lb) and la else 1), out
```
2. Replace each glob of the Facts: `x.glob("*.ppm")` used for names -> `shot_names(x)`, `any(x.glob("*.ppm"))` -> `shot_names(x)`.
   Keep `compare_ppm` for *.ppm names.
3. cmd_cmp loop: for the name `fhash.txt` call fhash_cmp(base_dir / n, row_dir / n), print its lines, add its bad to `bad`, and
   count it as equal when bad is 0. Other names: as today.
4. cmd_ingest_last: after the `shot = a.shots or ...` line add `if bd and bd["kind"] == "azahar_hash": fhash_extract(p, shot)`.
5. test_fhashcmp.py (import bench, as 3ds/tests/basekey/test_basekey.py does) with tmp folders: a log with 3 fhash entries and
   other lines -> fhash_extract gives 3 and the exact 3 lines; shot_names of that folder == ["fhash.txt"]; fhash_cmp of two equal
   files -> (0, [..., "3/3 frames equal"]); one hash changed in frame 2 of 3 -> bad 1 and "  f2 DIFF (rerun with fhashdraw=2 to
   name the draw)" and "2/3 frames equal"; an empty file -> bad 1. Print `PASS: <case>` / `FAIL: <case>`, exit 1 on a FAIL.
   run.sh: `cd "$(dirname "$0")/../../.." && python 3ds/tests/fhashcmp/test_fhashcmp.py`.
6. Commit: `FRAMEHASH4: bench.py fhash.txt in the shot folder, base and cmp compare frame hashes`.
## Gate (expected values)
1. Seeded fault: edits 1 and 5 only, with fhash_cmp returning 0 always: `sh 3ds/tests/fhashcmp/run.sh > build3ds/out/fh_before.txt`
   -> a FAIL line for the changed frame. Fix fhash_cmp. All edits: run it again -> only PASS lines, exit 0.
2. `sh 3ds/tests/basekey/run.sh` and `sh 3ds/tests/benchpath/run.sh`: pass as before (the *.ppm path is unchanged).
3. No Azahar start. Counter: none (harness row).
## Stop and end
A failed gate, a line that does not match the text above, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5).
Delete this row file in the completing commit.
