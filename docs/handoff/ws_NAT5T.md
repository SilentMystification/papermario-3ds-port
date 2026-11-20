# ws_NAT5T round 3 (Sonnet: last designer round): the debug walk of a static list call runs the static table. Delete that branch.
Designer: oracle NATIVE8. Worktree build3ds/wt/NAT5T, branch NAT5T, code tip ee51f4d66a. PM_ROW=NAT5T PM_LANE=NAT5T. Edits 1-7 of round 1 and rounds 1-2 are done. Counters: c0gate interp base - 2 (nd_tx_ldblk, nd_tx_ldtile), state 23, calls base - n (report n). Gate: 4 drec byte-equal, txraw 0.
## Facts (code tip ee51f4d66a)
- FACT the 53 txraw lines of spot_mac01_drec are on frames 35 + 4k (f35..f243, 53 frames). The bench switches hold prof=4 (log line #10). `prof cal every 4` (#17).
- FACT pf_arm (pm_phase.c:697, :710-712) sets pm_ph_on = 1 and calls pm_walk_dbg_set every 4th frame (pf_cd reset :738, arm :743). pm_walk_dbg_set (nd_walk.c:92) then sets WD_PH (8), so pm_walk_dbg != 0 on that frame.
- FACT gt_run0 (nd_walk.c:512-516): with pm_walk_dbg != 0 it ignores ab and runs a CALL unit through nd_nr_exec. nr_ctl (:321) calls nr_call with ab NULL. nr_call (:231-232) runs the static table (nd_nr_list nt->u or nd_ntab_run). nr_set (:267) calls nd_unit_apply. nd_set.c:179 calls nd_tx_tsz with a raw TILESIZE of tile 0. nd_tex_set.c:189 logs txraw and drops the window.
- FACT on frames with pm_walk_dbg 0 the same records run gt_body with ab (nd_walk.c:495, alt range with ND_FS_TXR). These frames have no txraw line.
- FACT walk_dbg_op for a CALL returns 0 (nd_walk.c:125: fp only for VTX, TRI1, TRI2, MTX). The branch feeds census counters only. The unit runs of the alt range still call walk_dbg_op (nd_nr_exec :361-372 reads pm_walk_dbg).
- FACT nd_nr_exec is an interp name (build3ds/out/nat5r_c0gate_base.txt line 16). The deletion removes 1 call site (calls -1 from this edit).
## Edit (one commit). File 3ds/src/nd_walk.c only.
1. gt_run0, lines 512-516. Delete the debug branch. The debug frame then takes the arms of a clean frame.
before:
```c
    if (__builtin_expect(pm_walk_dbg, 0)) {
        const uint32_t u[2] = { ND_OP_CALL_W0, (uint32_t)(uintptr_t)key };
        nd_nr_exec(u, 1u);
        return;
    }
    const NdW2* next = (const NdW2*)key;
```
after:
```c
    const NdW2* next = (const NdW2*)key; /* NAT5T: a debug walk (pm_walk_dbg) takes these arms too: the static table has no tile window */
```
2. The comment of gt_run0, lines 489-490. In line 489 replace `A debug walk (pm_walk_dbg) runs the G_DL unit on` with `NAT5T: a debug walk takes the same arms (its`. In line 490 replace ` * the walker so its census hooks see the list. Vertex` with ` * unit runs keep the census hooks). Vertex`. Keep the rest of both lines.
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`)
1. Apply the edits with the Edit tool. Commit `NAT5T 3a: the debug walk of a static list call takes the alt range arms (oracle round 3)`.
2. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat5t_host.txt 2>&1`: PASS.
3. `sh 3ds/tools/rowloop.sh --row NAT5T --bench spot_mac01_drec,entcape_kmr03_drec,fxparts_drec,wscmb_pause_drec --no-host --why "NAT5T round 3: debug walk" > build3ds/out/nat5t_loop.txt 2>&1`. Do not run `bench.py base` again.
4. `grep -H "txraw\|drec frame\|drec draw .* DIFF" build3ds/azt/logs/*_drec.txt > build3ds/out/nat5t_c0src.txt`.
5. `python 3ds/tools/c0gate.py build3ds/lane-NAT5T/pm_3ds.elf > build3ds/out/nat5t_c0gate.txt`. Base: `python 3ds/tools/c0gate.py c:/Users/Andrew/Documents/GitHub/papermario-3ds-port/build3ds/basebin/ae5aded95a75afbd/pm_3ds.elf > build3ds/out/nat5t_c0gate_base.txt` (FACT commit.txt 9576c03383 = merge-base).
Cost: 1 build, 4 Azahar starts (one rowloop).
## Gate (exact)
- host PASS. 4 lines `cmp <bench>: 1/1 byte-equal`. `grep -c txraw` is 0 in each of the 4 logs.
- c0gate interp = base interp - 2. state 23. Report calls (base and tip).
Fix rights: compile errors only. A DIFF, a txraw line, a crash or a tool fault: stop. Send nat5t_c0src.txt and the first DIFF line to the orchestrator. Do not change other code.
## Hand-back
Last commit `NAT5T 3: gate. <one line per bench>` with `git add -f build3ds/out/nat5t_c0src.txt build3ds/out/nat5t_loop.txt build3ds/out/nat5t_c0gate.txt build3ds/out/nat5t_c0gate_base.txt`. It deletes this page. One SubagentHandback: hashes, gate line per bench, c0gate line of base and tip.
