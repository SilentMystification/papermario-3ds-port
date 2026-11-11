# Row DETBASE: find the first merge where two spot_mac01_det runs of one build differ

(Model: Haiku. The agent reads only this file, CLAUDE.md and what this file names. No code edit in this row.)
## Facts
- Stored bases of spot_mac01_det (build3ds/basecache) at ec572bd96, 6d6cfb9ea and 86004672a have the same 6 shot md5s: two runs
  of each were byte-equal (bench.py base_run :1498-1503).
- The two runs of one build now differ on 0000_1_2.ppm: HC4f3 base at 5d61fda6a (two slots) and TOOLB3 base at e18160cad (one slot,
  both cold starts, both log_pass). So one of the code merges after 86004672a made this det scenario differ from run to run.
- The code merges on main after 86004672a (first parent; the other commits are docs, rows, runs.jsonl or tools):
  63b9174b7 = HC4e3 group 3 (its first parent 6dc89f149 has the same 3ds/src, 3ds/include, src, include as 86004672a),
  493f2f359 = FFOG1, e18160cad = ESCAPE2 (known bad).
- The oracle code read found no time or thread input in the shot path: dump_shot (pm_gpu.c:2449) runs after C3D_FrameBegin; det turns
  off the frame skip (pm_main.c:646), the step skip (:676) and the seed salt (pm_os.c:55), and runs the logic thread serially (:491).
  So the merge is found by runs, then the oracle reads that merge.
## Start
The main folder, after TOOLB3 is merged (its slot pin is needed). No worktree, no build by hand: `bench.py base` builds each commit in its lane.
Run alone: no other Azahar run on the machine during this row.
## Steps (stop at the first "two runs differ"; each step is 2 Azahar starts)
1. `python 3ds/tools/bench.py base spot_mac01_det --row DETBASE --commit 6dc89f149 > build3ds/out/detbase_1.txt 2>&1`
   "two runs equal, stored": the tools and the host are not the cause, go to 2. "two runs differ": stop (tools or host).
2. The same with `--commit 63b9174b7` (output detbase_2.txt). Equal: go to 3. Differ: stop (HC4e3 group 3).
3. The same with `--commit 493f2f359` (output detbase_3.txt). Equal: the cause is ESCAPE2 (e18160cad). Differ: FFOG1.
## Gate
Each base run prints "two runs equal, stored" or "two runs differ, not stored (<shot>)", and the two records have the same slot in
log_path. Any other output (a build failure, "not stored (run results ...)", a run on two slots) is a tool fault: stop.
## Stop and end
Commit the runs.jsonl lines only ("DETBASE: bases <commits>: <result each>"). Hand back: per step the commit, the result line, the two
run ids and their warm values. Delete this row file in that commit. The oracle then reads the named merge for the cause.
