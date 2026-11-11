# Row DETBASE2: find the first commit where two runs of one build of spot_mac01_det differ (bisect, no code edit)

(Model: Haiku. The agent reads only this file, CLAUDE.md and what this file names.)
## Facts (2026-11-11)
- b8bc06613: spot_mac01_det and mdl5_iwa10_det stored (two equal runs). HCCUT1 at f978553f0 and HC8 at 7992565a8: one lane run each,
  cmp 6/6 equal to b8bc06613.
- `bench.py base --fill` at main 4bbd73043 (after the merges of LOADMTX c3ab9a9d2, SAVEFIX f978553f0, HC8 7992565a8): spot_mac01_det
  "two runs differ (0000_1_2.ppm)", mdl5_iwa10_det "two runs differ (0000_8_5.ppm)"; battle_start_f182_cold equal. All runs rc 0.
## Start
The main folder (no worktree, no build by hand: `bench.py base --commit` builds each commit in its lane). PM_ROW=DETBASE2.
Before each step: `powershell -NoProfile -Command "Get-CimInstance Win32_Process -Filter \"Name='azahar.exe'\" | Select ProcessId,ExecutablePath"`:
any path outside build3ds\azslots\slot<k>: stop and hand back (the user closes it). No other row runs Azahar during this row.
## Steps (each 2 starts; stop at the first "two runs differ"; at most 8 starts)
1. `python 3ds/tools/bench.py base spot_mac01_det --row DETBASE2 --commit f978553f0 > build3ds/out/detbase2_1.txt 2>&1`
   (before HC8). Equal: go to 2. Differ: stop (the cause is before HC8).
2. The same with `--commit 7992565a8` (HC8 alone, before its merge with LOADMTX and SAVEFIX), output detbase2_2.txt. Equal: go to 3.
   Differ: stop (HC8 itself).
3. The same with `--commit 4bbd73043` (the merge), output detbase2_3.txt. Differ: the merge. Equal: hand back (the fill result
   was not repeated: a timing fault that shows only some of the time).
After a "two runs differ": before anything else, copy both snapshot folders named in the runs.jsonl records of the two runs
(shot_path, build3ds/basesnap/...) to build3ds/out/detbase2_runA/ and runB/ (bench.py deletes them at the end of the base call: if
they are gone, say so), and the two game logs (log_path of each record) to build3ds/out/.
## Gate
Each step prints "two runs equal, stored" or "two runs differ, not stored (<shot>)", and both records name the same slot in log_path.
Any other result (a build failure, "not stored (run results ...)", two slots, a stray Azahar) is a tool fault: stop.
## Stop and end
Commit the runs.jsonl lines only ("DETBASE2: <commit> <result> per step"). Hand back per step: commit, result line, the two run ids,
warm values, and the shot names that differ. Delete this row file in that commit.
