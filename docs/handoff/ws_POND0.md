# ws_POND0: pond gate benches and bisect (Haiku, oracle NATIVE21, plan_pond.md)
Objective: two det benches at the mac_00 pond. Name the row that broke the water by a byte compare. No game code.
Counter: OPEN (plan_pond.md section 4). Metric: pond_mac00_det cmp result per commit.
## Files (your own only)
1. 3ds/tools/azt.d/POND0.json: {"scenarios": [two entries]}. Copy the fields of spot_mac01_drec (azt.d/HOSTCORE2.json).
   - pond_mac00_det: debug `warp=mac_00:4 pos=-178:0:88 prof=4 shots shotwin=200:6 loglevel=debug det`,
     until `prof f\\d+ w\\d+ at `, cap 35, deadline 30, first_marker `prof=1 log=off`, expects none, needs_picture false,
     events: prof on (`prof=1 log=off`), scene (`prof f\\d+ w\\d+ at mac_00 `).
   - pond_mac00_drec: debug `warp=mac_00:4 pos=-178:0:88 prof=4 loglevel=debug det drec=200:2`,
     until `drec f201 draws [1-9]\\d* bytes \\d+`, cap 35, deadline 30, the same marker, events: prof on, drec (the until regex).
2. 3ds/tools/bench.d/POND0.json: {"benches": [two entries]}, kind azahar_pic, speed normal, default false, tags azahar det cold
   (plus drec on the second), gates [], scenario = name, cap and until as in step 1. Example: entcape_kmr03_drec in bench.d/ENTCAPE.json.
3. Commit both files in one commit.
## Runs (in this order; budget 8 Azahar starts)
4. `python 3ds/tools/bench.py base pond_mac00_det,pond_mac00_drec --commit bd0a65064e --row POND0` (DO1 parent; 1 build, 4 runs).
5. `python 3ds/tools/bench.py build-at 6bdd944bec pond_mac00_det --row POND0 --why "POND0 DO1 merge"`, then
   `python 3ds/tools/bench.py cmp pond_mac00_det --row POND0 --commit bd0a65064e > build3ds/out/pond0_do1.txt`.
6. `PM_LANE=POND0 sh 3ds/build.sh`. Then with PM_ROW=POND0 PM_LANE=POND0:
   `python 3ds/tools/bench.py run pond_mac00_det,pond_mac00_drec --row POND0 --why "POND0 tip"`.
7. `bench.py cmp pond_mac00_det --row POND0 --commit bd0a65064e > build3ds/out/pond0_tip.txt`. Same for pond_mac00_drec (pond0_tipd.txt).
8. If cmp prints `the row run was a cold start`, run that bench once more, then cmp again.
## Pass and fail
- Pass: step 4 stores both bases (two equal runs each). Steps 5 and 7 give a cmp result line each.
- Expected (GUESS): step 5 DIFF, step 7 det DIFF, step 7 drec byte-equal. Any result is data. Do not change a base to get it.
- You may edit only POND0.json files to fix a scenario fault (a missing event, a wrong regex). Note it in the commit.
- A base that fails or two base runs that differ: stop (CLAUDE.md rule 4). Send the output path to the oracle.
## Stop and hand-back
- Stop after step 7, or at the budget, or at a tool fault.
- Commit build3ds-free files only (runs.jsonl lines if bench.py writes them). Delete this page in the last commit.
- SubagentHandback: commit hash; the `N/M byte-equal` line and the first DIFF line of each cmp; the run count.
- Then SendMessage the oracle NATIVE21: the same lines (POND1 starts from your tip).
