# Row BASEKEEP: a base pair that differs keeps both shot sets and names every differing shot (evidence for the oracle)

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names.)
## Tool fault (CLAUDE.md 4) and code read (ORACLE8, main 4404177f0)
- `bench.py base spot_mac01_det` at 86f530af6 (NPSO2) and `base spot_mac01_fps60_det` at 3c7d977ab (FPS60FL) ended
  "two runs differ, not stored (0000_1_2.ppm differs)". spot_mac01_det stored equal pairs at 5 older commits.
- The oracle found no producer by code read: the shot frames are walker frames (nd_wq.c:64, :124, :134, deterministic in the
  serial det mode), dump_shot reads the target after C3D_FrameBegin waited (pm_gpu.c:1955-2017), the game writes no SD file
  that a later boot reads except ndref.bin (grep of fopen "w" in 3ds/src), FRAMEHASH4 changed only which files are compared
  (shot_names), not their bytes.
- The evidence is lost: shots_diff (bench.py:1526-1537) returns the FIRST differing name only, and base_run deletes both
  snapshot folders (bench.py:1608 `shutil.rmtree(snap ...)`). So nobody can say which shots differ, by how many pixels, where.
Start: `sh 3ds/tools/wt_new.sh BASEKEEP`. PM_ROW=BASEKEEP PM_LANE=BASEKEEP on every call.
## Touch only (places)
3ds/tools/bench.py: shots_diff (:1526-1537), the `diff = shots_diff(...)` block of base_run (:1599-1603) and its final rmtree line
(:1608). New 3ds/tests/basediff/run.sh and test_basediff.py.
## Exact edits
1. New function after shots_diff: `def shots_diff_all(a, b)` -> list of (name, text) for every name in shot_names(a) or
   shot_names(b): missing in one, or md5 differs (for a .ppm: text from compare_ppm(a/n, b/n, None) as "px <n> bbox x0..x1
   y0..y1", for fhash.txt or drec.txt: "text differs"). Empty list when equal.
2. base_run, where `diff` is not empty: before the print, `fails = shots_diff_all(first shot_path, last shot_path)`; keep both
   folders: `keep = REPO / "build3ds" / "basesnap_fail" / (lane or "main") / time.strftime("%Y%m%d-%H%M%S")`, copytree of each
   into keep/"a" and keep/"b"; print `base <bench>: two runs differ, not stored, <k> shots: <name text; name text ...>, kept at
   <keep>`. The rmtree at :1608 stays (it removes the snap folder, not the kept copy).
3. test_basediff.py (as 3ds/tests/basekey/test_basekey.py imports bench): two temp folders with 3 equal 2 x 2 P6 ppm files, then
   change one pixel of 0001 and one of 0002 in folder b: shots_diff_all gives exactly 2 entries (0001, 0002) with px 1 each.
   Equal folders: empty list. Print PASS/FAIL per case, exit 1 on a FAIL. run.sh runs it from the repo root.
4. Commit: `BASEKEEP: base_run keeps both shot sets of a differing pair and names every differing shot`.
## Gate (expected values)
1. Seeded fault (a value change): in shots_diff_all compare only the first name of the list. `sh 3ds/tests/basediff/run.sh`:
   FAIL (1 entry, not 2). Restore: PASS, exit 0.
2. `sh 3ds/tests/basekey/run.sh` and `sh 3ds/tests/fhashcmp/run.sh`: pass as before. No Azahar start. Counter: none (tool row).
## After the merge (orchestrator)
Rerun `bench.py base spot_mac01_det` once at the NPSO2 merge-base (new commit, budget reset is a tool fault: no reset). If it
differs again, the kept folders go to the oracle: the differing shot list names the producer frame and area.
## Stop and end
A failed gate, a line that does not match the text above, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5).
Delete this row file in the completing commit.
