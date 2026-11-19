# ws_NAT4G (Haiku): every other scalar stream unit (GEOM, FILL, PRIMZ, SCISSOR, MW, TIMG..LOADTLUT) also cuts only on a core 1 mark. Then core 1 is the only cut source of the stream state units. Counter: c0gate unchanged, expected `c0gate calls 122 interp 34 state 23`. Gate: expected-diff (the UD-N5 epcut rule).
Designer: oracle NATIVE. WAIT: `Merge NAT4F`. Branch from main after it. Worktree build3ds/wt/NAT4G, branch NAT4G. PM_ROW=NAT4G PM_LANE=NAT4G.
Edit only: 3ds/src/nd_rec.c (the mark line), 3ds/src/nd_walk.c (nr_set, the NAT4F block). No other row edits these files then.
## Facts (main after NAT4F)
- FACT the unit ids: OMH 0x20 .. COMB 0x2d, END 0x2e, DROP 0x2f, TIMG 0x30 .. LOADTLUT 0x35, VTX 0x36 (nd_fs.h:9-31). nr_set runs every scalar id (nd_walk.c nd_nr_exec: not geo, not ctl, not END).
- FACT core 0 never skips FILL, PRIMZ, SCISSOR, TIMG..LOADTLUT, and skips MW only for an equal LIGHTCOL (nd_unit_same nd_set.c:123-147, default 0). Core 1 marks them always (rule 6), MW always but LIGHTCOL (rule 7), LIGHTCOL on a change (rule 1) (nd_ds_ep_rule nd_ds.c:402-407). So these agree by rule.
- FACT GEOM: core 0 skips an equal value only when pm_gpu_mode_is agrees (nd_set.c:128-131). Core 1 marks a change (rule 1). A GPU mode that a rectangle or a direct draw changed gives a core 0 cut with no mark: an epcut frame (UD-N5).
- FACT these handlers still run on core 0 when they change a field (the vertex load, the binds and the draws read them).
## Edits (changed lines end with `/* NAT4G */`)
1. nd_rec.c, the mark line: the class test becomes `((id >= ND_FS_OMH && id <= ND_FS_COMB) || (id >= ND_FS_TIMG && id <= ND_FS_LOADTLUT))`.
2. nd_walk.c nr_set, the NAT4F block: `cb_ = i_ == ND_FS_OMH || i_ == ND_FS_TEXTURE` becomes `cb_ = !ca_ && ((i_ >= ND_FS_OMH && i_ <= ND_FS_COMB) || (i_ >= ND_FS_TIMG && i_ <= ND_FS_LOADTLUT))` (the comment: NAT4G all other scalar units). The rest stays.
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, 1 build, 8 Azahar starts of 8)
1. Edits 1-2. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat4g_host.txt 2>&1`: PASS. A macro-against-native host test that fails at a shape its seed did not ask for: add the NAT4A nd_ds restore line to its run function. Commit `NAT4G 1: all stream scalar units cut on the core 1 mark`.
2. For B in spot_mac01_drec, entcape_kmr03_drec, fxparts_drec, wscmb_pause_drec: `python 3ds/tools/bench.py base <B> > build3ds/out/nat4g_base_<B>.txt 2>&1`.
3. `sh 3ds/tools/rowloop.sh --row NAT4G --bench spot_mac01_drec,entcape_kmr03_drec,fxparts_drec,wscmb_pause_drec --no-host --why "NAT4G: core 1 cut for all scalar units" > build3ds/out/nat4g_loop.txt 2>&1`.
4. `grep -H "epchk\|epcut\|epbin\|c0src f\|drec frame\|drec draw .* DIFF" build3ds/azt/logs/*_drec.txt build3ds/out/loop_NAT4G_cmp_*.txt > build3ds/out/nat4g_epchk.txt`. 5. `python 3ds/tools/c0gate.py build3ds/lane-NAT4G/pm_3ds.elf > build3ds/out/nat4g_c0gate.txt 2>&1`.
## Gate (exact, expected-diff rule)
Host PASS. For each bench: equal, or the first DIFF draw lies in a frame F with `epcut fF`. c0gate `calls 122 interp 34 state 23`.
Fix rights: compile errors, the host test restore line. A DIFF in a frame with no epcut line, a crash or a tool fault: stop, send nat4g_epchk.txt to oracle NATIVE with SendMessage, wait.
## Hand-back
Last commit `NAT4G 2: gate. expected-diff: <bench> ...` (the word `expected-diff` in it) with `git add -f build3ds/out/nat4g_epchk.txt build3ds/out/nat4g_loop.txt`. It deletes this page. One SubagentHandback: hash, the gate line per bench, the epbin lines.
