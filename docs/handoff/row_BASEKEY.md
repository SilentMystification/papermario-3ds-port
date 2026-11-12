# Row BASEKEY: the base store keys a base by its scenario definition; a base stores from two warm runs, not three fixed runs

(Model: Haiku. Tool row, 3ds/tools/bench.py only plus one new test folder. The exact code is below; no design choice.)

## Facts (S4WBLK, 2026-11-12; build3ds/wt/S4WBLK/build3ds/out/S4WBLK_base5_fxpic84.txt, build3ds/azslots/pool.log)
1. `bench.py base fxpic_84` printed `stored, 6 shots ... No build, no run` after the scenario window of fxpic_84 changed (404:4) under
   the same name and commit: the store is keyed by commit and bench only (bench.py:1412-1415 stored, :1418-1439 store_base).
2. base_run (bench.py:1481-1523) runs 3 starts (BASE_RUNS) and compares the last two; it stores nothing when one of them is cold
   (:1504-1507). A warm restart that fails (pool.log: `warm fail emu 0 B after the key`, about 4 % of warm tries, 81 of 1978) turns
   that run cold, and S4WBLK lost 4 base attempts in a row this way.

## Start
`sh 3ds/tools/wt_new.sh BASEKEY`. PM_ROW=BASEKEY PM_LANE=BASEKEY on every call. No running row edits bench.py.

## Touch only
3ds/tools/bench.py: the constants above base_run (:1477-1478), stored (:1412-1415), store_base callers' info dict in base_run,
base_run (:1481-1523). New: 3ds/tests/basekey/run.sh and 3ds/tests/basekey/test_basekey.py.

## Exact edits (bench.py; `import hashlib` added to the import line :80)
1. Above `def stored`: `SCEN_KEYS = ("debug", "extra", "until", "first_marker", "cap", "deadline")` and
   `def scen_hash(s):` docstring "BASEKEY: 12 hex digits of the scenario fields that change a run." body:
   `d = {k: (s or {}).get(k) for k in SCEN_KEYS}` / `return hashlib.sha1(json.dumps(d, sort_keys=True).encode("utf-8")).hexdigest()[:12]`.
2. `stored(commit, bench)`: keep the meta.json and *.ppm test (return False when it fails). Then
   `bd = bench_def(bench)`; `want = scen_hash(scenario(bd["scenario"])) if bd and bd.get("scenario") else None`; read
   `have = json.loads((d / "meta.json").read_text(encoding="utf-8")).get("scen_hash")` (OSError or ValueError: have = None);
   `if want and have != want:` print `base %s at %s: stored for another scenario definition (hash %s, now %s): stale` (bench,
   commit[:9], have or "none", want) and return False. Else return True. (A store without the field is stale once: it is rebuilt.)
3. Constants: delete `BASE_RUNS = 3`; keep `BASE_TWICE = 2`; add `BASE_MAX_RUNS = 4  # BASEKEY: a list of 2, then single runs until two warm runs pass`.
4. New helper above base_run: `def base_warm(bd, row, label, commit, since):` returns `(recs, warm)` with
   `recs = base_records(bd["name"], row, label, commit, since)` and `warm = [r for r in recs if r.get("result") == "log_pass" and r.get("warm") == 1]`.
5. base_run: the first cmd_run uses `",".join([name] * BASE_TWICE)` (2 starts). Then, still inside the try (PM_BASE_SNAP set):
   `n = BASE_TWICE` / `while n < BASE_MAX_RUNS:` / `need = [bd for bd in bds if len(base_warm(bd, row, label, commit, since)[1]) < BASE_TWICE and all(r.get("result") == "log_pass" for r in base_warm(bd, row, label, commit, since)[0])]`
   / `if not need: break` / `rc = cmd_run(argparse.Namespace(bench=",".join(b["name"] for b in need), row=row, why=why, label=label, build=build_no, commit=commit, speed=None, lane=lane))`
   / `n += 1`.
6. base_run per bench: `recs, warm = base_warm(...)`. A record with a result other than log_pass: keep the old "not stored (run
   results ...)" print. `len(warm) < BASE_TWICE`: print `base %s: fewer than two warm runs in %d starts (warm %s), not stored.`
   (name, len(recs), ", ".join(str(r.get("warm")) for r in recs)). Else `first, last = warm[-2], warm[-1]`; the slot check, the
   shots_diff and store_base stay; the info dict gets `"scen_hash": scen_hash(s)`. Delete the old "not both warm" block (:1503-1507).
7. Update the base_run docstring: "two starts in one list, then single starts (at most BASE_MAX_RUNS) until two warm log_pass runs
   exist; the last two warm runs are compared".

## Test (new)
3ds/tests/basekey/test_basekey.py: `sys.path.insert(0, "3ds/tools")` (bench.py imports prof_by_scene from there), then
`import bench` (bench.py:2470 guards main, so the import runs nothing). Point `store_root` at a
tempfile.TemporaryDirectory (`bench.store_root = lambda: Path(tmp)`). Cases: (a) scen_hash is equal for two dicts that differ only
in "status" and "reason"; (b) differs when "debug" differs; (c) a store folder with one .ppm and meta.json without scen_hash ->
stored() False; (d) meta with the current hash of bench spot_mac01_det -> True; (e) meta with "000000000000" -> False. Print
`basekey: 5/5 PASS`, exit 1 on a failure. run.sh: `cd "$(dirname "$0")/../../.." && python 3ds/tests/basekey/test_basekey.py`.

## Gate
- `sh 3ds/tests/basekey/run.sh`: `basekey: 5/5 PASS`. `python 3ds/tools/bench.py run nd_host --row BASEKEY --why "BASEKEY tool row"`: PASS.
- One real base: `PM_ROW=BASEKEY python 3ds/tools/bench.py base fxpic_84 --commit d048e654d --row BASEKEY > build3ds/out/basekey_base.txt 2>&1`.
  Expected: the stale line for the old store (no scen_hash), one build of d048e654d, 2 starts (3 or 4 only after a failed warm
  restart), `two runs equal, stored`, exit 0. Base starts are not counted in the budget (run_azahar.ps1:79). A failed base: tool
  fault, stop and hand back the file.

## Stop / end
Delete this row file in the completing commit. Hand back the hash and both gate outputs. After the merge the orchestrator runs
`bench.py base --fill` (every old store is stale once) and tells S4WBLK to run `bench.py base fxpic_84` again.
