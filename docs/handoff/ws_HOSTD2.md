# ws_HOSTD2 (Haiku): 7 host.d tests stop sharing 3 read-only /tmp names, so the batch rule packs them (counter T1)
Designer: the oracle (SendMessage). Worktree build3ds/wt/HOSTD2, PM_ROW=HOSTD2 PM_LANE=HOSTD2. Start now, parallel with
VOLHOST, GCONVSPLIT and POSTASYNC (no shared file).
## Objective
- FACT (nd_host_test.sh:77-83): two host.d scripts share a batch only when they name no common /tmp or build3ds/out path.
- FACT: /tmp/ndcomb.o, /tmp/ndcombs.c, /tmp/ndlogs.c are made by nd_host_test.sh (:108, :152, :160) before the host.d loop
  (:187). geounit, hunit, nmdl0, ns3unit, stunit, texunit and nxf only read them.
- FACT (oracle run of the batch rule on main b43d82ef57, 61 scripts): 8 batches now (5 x 12, 2, 1, 1). After the patch 6
  (5 x 12, 1). Time saved: GUESS 10-20 s of STAGE container-end (2 batches fewer that ran 1 script each).
## Files (all in docs/handoff/ws_HOSTD2.patch). Edit only these 7: 3ds/tests/host.d/geounit.sh, hunit.sh, nmdl0.sh,
ns3unit.sh, stunit.sh, texunit.sh, nxf.sh.
## Edit shape. Before (hunit.sh:6):
`LIBS="3ds/src/nd_xf.c ... 3ds/src/nd_tmpl.c /tmp/ndcomb.o /tmp/ndcombs.c /tmp/ndlogs.c -lm"`
After: one line `RO=/tmp # HOSTD2: read-only inputs ...`, then `LIBS="3ds/src/nd_xf.c ... $RO/ndcomb.o $RO/ndcombs.c $RO/ndlogs.c -lm"`.
## Loop (write each output to build3ds/out/, read the last 20 lines)
1. `tr -d '\r' < docs/handoff/ws_HOSTD2.patch | git apply -`. A reject: stop. Send the output to the oracle.
2. `sh 3ds/tools/nd_host_test.sh > build3ds/out/hd2_host.txt 2>&1`: `ND HOST TESTS PASS`. The 7 scripts print their ok lines.
3. `grep -c . build3ds/out/hostd_batches.txt` prints 6.
4. Commit the 7 files: `HOSTD2: the 3 read-only host inputs no longer split host.d batches (8 to 6). Host PASS. Claude`. In the
   same commit delete docs/handoff/ws_HOSTD2.md and ws_HOSTD2.patch.
Budget: 1 host run, no build, no Azahar start (no game code change).
## Fail
- A host.d FAIL in one of the 7 scripts: compare its line with the patch. 2 tries per cause, then send the output path to the oracle.
- Step 3 prints another number: send hostd_batches.txt to the oracle. Do not change nd_host_test.sh (VOLHOST owns it).
## Stop and hand-back. One SubagentHandback: the commit, the PASS line, the batch count, STAGE container-end.
