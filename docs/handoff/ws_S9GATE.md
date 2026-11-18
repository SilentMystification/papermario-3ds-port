# ws_S9GATE (Haiku): the end proof of the display list removal on main. No code edit. Counters: every listcen total 0 (C1-C10) and `elfgate func 0 data 0 gfxnames 0` (C11, C12). Gate: the numbers below and 4 drec compares against the start of the DEL chain.
Designer: oracle DEL helper. WAIT: start from main after the merges of DEL1D2, DEL2, DEL2E, DEL3C1-3, DELGT, DEL5B3 and IMGFX and IMGFX2 (without IMGFX2 reader stays 11, nd_op.h:79-120). Worktree build3ds/wt/S9GATE, branch S9GATE. PM_ROW=S9GATE PM_LANE=S9GATE.
Edit only: no source file. Outputs in build3ds/out/ (commit them with `git add -f`).
## Facts
- FACT the end proof is listcen all 0 and elfgate 0 0 0 (plan_finish.md:2, :36). listcen core1 is outside it (UD1 option 1, oracle_handoff.md:5).
- FACT every DEL row gates byte-equal against its merge-base, so the drec of main equals the drec of f2ab50d38d (the main before the first DEL page of this oracle) unless a later row merged with an expected-diff gate.
- FACT listcen prints `listcen total <class> <n>` lines and `listcen core1 <n>` (listcen.py:342); elfgate prints the last line `elfgate func <n> data <m> gfxnames <k> gfxscan_skip <s>` (elfgate.py header).
## Steps (one command per step, output to build3ds/out/, read the last 20 lines; Bash `timeout: 600000`; 1 lane build; at most 8 Azahar starts)
1. `PM_LANE=S9GATE sh 3ds/build.sh > build3ds/out/s9gate_build.txt 2>&1`: rc 0.
2. `python 3ds/tools/listcen.py > build3ds/out/s9gate_listcen.txt 2>&1`. Then `grep "^listcen total" build3ds/out/s9gate_listcen.txt | grep -v " 0$" > build3ds/out/s9gate_nonzero.txt`.
3. `python 3ds/tools/elfgate.py build3ds/lane-S9GATE/pm_3ds.elf > build3ds/out/s9gate_elf.txt 2>&1`.
4. `sh 3ds/tools/nd_host_test.sh > build3ds/out/s9gate_host.txt 2>&1`: PASS.
5. The base commit B: `git log --oneline --grep "expected-diff" f2ab50d38d..main > build3ds/out/s9gate_xdiff.txt`. Empty: B = f2ab50d38d. Not empty: B = the newest merge commit in that file; write B into build3ds/out/s9gate_base.txt.
6. For each of spot_mac01_drec, entcape_kmr03_drec, fxparts_drec, wscmb_pause_drec, one bench per call: `python 3ds/tools/bench.py base <bench> --commit <B> > build3ds/out/s9gate_base_<bench>.txt 2>&1`, then `sh 3ds/tools/rowloop.sh --row S9GATE --bench <bench> --no-host --why "S9GATE end proof" > build3ds/out/s9gate_loop_<bench>.txt 2>&1`.
## Gate (exact)
Build rc 0. s9gate_nonzero.txt empty (every `listcen total` line ends in 0). The last line of s9gate_elf.txt starts with `elfgate func 0 data 0 gfxnames 0`. Host PASS. The 4 rowloops exit 0 (byte-equal).
## Stop
Any number above 0, a DIFF, a crash or a tool fault: stop. Commit the outputs (`git add -f`), send the paths and the non-zero lines to the oracle of the launch prompt with SendMessage, wait. Do not edit code to pass this gate.
## Hand-back
Last commit `S9GATE: end proof. listcen all 0, elfgate 0 0 0, 4 drec equal to <B>` deletes this page. One SubagentHandback: commit hash, the listcen total lines, the elfgate line, B. The user's end checks follow (plan_finish.md section 5).
