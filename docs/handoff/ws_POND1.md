# ws_POND1: no model memo hit inside an open unit log (Haiku, oracle NATIVE21, plan_pond.md)
Objective: the mac_00 water copy record (no custom gfx) takes no DO1 memo hit while build_custom_gfx logs units.
Cause (FACT code read): a hit (nd_rec.c:359-361, mm_fast :338-343) writes nd_ds without lg_put (nd_ds.c:351). The water record
replays the log (nd_ds.c:486) without the copy units. Its snap then has no tile 1, so the surface is refused (why 6).
Counter: OPEN (plan_pond.md section 4). Metric: pond_mac00_det cmp against bd0a65064e, DIFF to all byte-equal.
Start: the POND0 branch tip (it has the benches and the base at bd0a65064e).
## Edits (Edit tool only: CRLF sources)
1. 3ds/src/nd_ds.c:331. Before: `void nd_ds_lg_close(void) { if (lg_dep) lg_dep--; }`
   After: that line, then a new line
   `int nd_ds_lg_on(void) { return lg_dep != 0u; } /* POND1: a range of the unit log is open (nd_rec_model takes no memo hit) */`
2. 3ds/include/nd_ds.h:56. Before: `void nd_ds_lg_close(void);` After: that line, then `int nd_ds_lg_on(void); /* POND1 */`
3. 3ds/src/nd_rec.c:356. Before:
   `    const int mmc = !r[22] && !r[23]; MmEnt* me = NULL; unsigned mh = 0; /* DO1: a record with a custom gfx range has no memo */`
   After:
   `    const int mmc = !r[22] && !r[23] && !nd_ds_lg_on(); MmEnt* me = NULL; unsigned mh = 0; /* DO1: a record with a custom gfx range has no memo; POND1: nor a record inside an open unit log (a hit writes nd_ds with no lg_put, so nd_ds_range misses its units) */`
4. 3ds/tools/nd_ds_test.c, NST2D block (:221-226). After `nd_ds_lg_open();` add `C5("lg_on open", nd_ds_lg_on(), 1);`.
   After `nd_ds_lg_close();` add `C5("lg_on closed", nd_ds_lg_on(), 0);`.
## Tests (every commit: build, host tests, c0gate)
5. `sh 3ds/tools/nd_host_test.sh > build3ds/out/pond1_host.txt` (timeout 600000). Pass: the last line says PASS.
6. `PM_LANE=POND1 sh 3ds/build.sh`. `python 3ds/tools/c0gate.py <elf>`. Pass: calls 0 interp 0 state 0.
7. Gate (2 Azahar runs), with PM_ROW=POND1 PM_LANE=POND1:
   `python 3ds/tools/bench.py run pond_mac00_det,spot_mac01_fps60_det --row POND1 --why "POND1 gate"`.
   `bench.py cmp pond_mac00_det --row POND1 --commit bd0a65064e > build3ds/out/pond1_cmp.txt`.
   `bench.py base spot_mac01_fps60_det --commit $(git merge-base HEAD main)`, then `bench.py cmp spot_mac01_fps60_det --row POND1 --commit $(git merge-base HEAD main)`.
   Pass: both cmp print all shots byte-equal. Seeded fault: the POND0 tip cmp was DIFF (its hand-back).
8. If the pond base at bd0a65064e is not stored (the store keeps 5 commits), run `bench.py base pond_mac00_det --commit bd0a65064e --row POND1` first.
## On a fail
- Edits 1 to 4 are all you may change. A pond DIFF after the fix: send build3ds/out/pond1_cmp.txt to the oracle NATIVE21.
- A mac_01 DIFF: send its cmp output to the oracle. Do not change a base. Budget: 8 Azahar starts.
## Stop and hand-back
- Commit the 4 files and runs.jsonl lines. Delete this page in the last commit.
- SubagentHandback: commit hash, host result, c0gate values, both cmp lines (`N/M byte-equal`). Last line: `POND1 done`.
