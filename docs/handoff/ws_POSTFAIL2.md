# ws_POSTFAIL2 (Haiku): bad viewport log (debug tool) and up to 3 diagnosis starts of spot_mac01_drec (no counter)
Designer: oracle DESIGN2 (SendMessage). Worktree build3ds/wt/POSTFAIL2, branch POSTFAIL2 from main, PM_ROW=POSTFAIL2 PM_LANE=POSTFAIL2.
Main at release: 5764ac7bbb. Start: now (no shared file with FLTGT or OVLIMG). Model: Haiku.
The user allows 3 Azahar starts for this fault (row POSTFAIL2, 2026-11-17). Base runs are not charged.
Edit only the 2 files of docs/handoff/ws_POSTFAIL2.patch: 3ds/src/nd_walk.c, 3ds/src/nd_rec.c.
## Objective
- FACT (build3ds/out/postfail1_trace.txt): one run read a viewport at vp 0x00000001. The unit was w0 0x3B080008, w1 1
  (op_geo default arm, nd_walk.c:297, then gbi_viewport, nd_mtx_set.c:167). No producer passes 1 (30 sites, nd_tx.h:34).
- FACT (code): core 1 records into chain[rec_ctx] (nd_rec.c:164-172). Core 0 runs a chain by stream_run (nd_rec.c:579).
  OPEN: the caller and the buffer of the bad unit. The patch logs them.
- The patch: op_geo calls gbi_cold_bad_vp(u) when a G_MV_VIEWPORT unit has a non-host w1. It writes one line (4 at most):
  `POSTFAIL2 bad vp f<frame> rm <n>: unit <p> <w0> <w1> prev .. next ..; unit ctx <k> chunk <i>; cmd <p> ctx <k> chunk <i> h <hdr> w1 <runner>; rec ctx <k> dctx <k>`.
  stream_run stores the running command in nd_stream_cur (one store per command). nd_rec_where names the chain and chunk of a pointer.
- FACT (oracle host run, 5764ac7bbb + patch, PM_HOSTSEL=all): ND HOST TESTS PASS, geounit ok 14/14.
- FACT (oracle host run without PM_HOSTSEL=all): geounit.sh fails (`/tmp/ndcombs.c: No such file`): it links nd_rec.c and reads files of
  the inline unit, which hostsel does not rerun. A needs line cannot name inline (nd_host_test.sh:146 sort rule). Use PM_HOSTSEL=all.
## Edit shape (nd_walk.c:297). Before: `    default: gbi_movemem(a, b); break;`
After: `    default:` / `        if ((a & 0xffu) == G_MV_VIEWPORT && !is_host(b)) gbi_cold_bad_vp(u); /* POSTFAIL2 (debug tool) */` /
`        gbi_movemem(a, b);` / `        break;`
## Loop (write each output to build3ds/out/, read the last 20 lines)
1. `tr -d '\r' < docs/handoff/ws_POSTFAIL2.patch | git apply -`. A reject: stop. Send the output to the oracle.
2. Commit the 2 files: `POSTFAIL2 1: bad viewport log gbi_cold_bad_vp (debug tool): unit, neighbours, stream chain and chunk, command`.
3. `python 3ds/tools/bench.py base spot_mac01_drec --row POSTFAIL2 > build3ds/out/postfail2_base.txt 2>&1` (base runs, not charged).
4. Start 1: `PM_HOSTSEL=all sh 3ds/tools/rowloop.sh --row POSTFAIL2 --bench spot_mac01_drec --why "POSTFAIL2 diag 1" > build3ds/out/postfail2_loop.txt 2>&1`.
5. After each start: `grep -h "POSTFAIL2 bad vp" azt/logs/spot_mac01_drec.txt build3ds/azslots/slot*/user/log/*.txt >> build3ds/out/postfail2_hits.txt`.
   GUESS (path of postfail1_trace.txt FACT 1 and 2): the game log is azt/logs/spot_mac01_drec.txt of the main folder. No file: use the log
   path that the run prints, and note it in the hand-back.
6. No hit and starts below 3: `python 3ds/tools/bench.py run spot_mac01_drec --row POSTFAIL2 --lane POSTFAIL2 --why "POSTFAIL2 diag <n>" > build3ds/out/postfail2_run<n>.txt 2>&1`.
   Then step 5 again. Stop at the first hit or after 3 starts (a cold-start rerun of rowloop counts as a start).
## Tests that decide
- Step 4: `LOOP PASS` (host PASS, build rc 0, cmp byte-equal: the log changes no draw).
- The result is the hit lines, or the fact "no hit in <n> starts". Both are a done task.
## What you may edit to fix a fail
Only the 2 files. A build error in them: fix the line (mechanical), note it in the commit. A DIFF or a tool fault: stop.
Send the output path in build3ds/out/ to the oracle. Never run a 4th start.
## Stop and hand-back
Last commit `POSTFAIL2 2: <n> starts, <k> hits (build3ds/out/postfail2_hits.txt)` deletes ws_POSTFAIL2.md and ws_POSTFAIL2.patch. The log stays (rule 7).
One SubagentHandback: commit hashes, LOOP lines, starts used, each hit line in full (or "no hit").
