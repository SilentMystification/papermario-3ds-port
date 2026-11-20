# ws_NCS1 (Haiku: 3 files, core 0 debug only, byte-equal): the walker description reads the applied snap; comb_in_walker goes
READY. Start commit: main. WAIT: none. Worktree build3ds/wt/NCS1, branch NCS1. PM_ROW=NCS1 PM_LANE=NCS1. Designer: oracle NATIVE14 (id in the launch prompt).
Objective: nd_walker_desc (psochk and entcap debug switches) takes its fields from the last applied snap (sc_cur), not from nd_ws and the F3 globals. comb_in_walker has no caller left and goes. Counter C13: calls -1, interp -1 (FACT: the only call site is nd_comb.c:176; FUNC comb_in_walker is in the main ELF). state stays.
Regions (main dd95e967ca, find by text): nd_comb.c :166-182 (comb_in_walker, nd_walker_desc); nd_snap.c after :198 (`nd_snap_cur_b`); nd_snap.h after :73. No other row edits these lines.
## Facts
- FACT nd_walker_desc has 3 callers, all debug: nd_psochk_draw (nd_comb.c:192, switch psochk), entcap_rec (nd_walk.c:489, switch entcap). Neither writes a draw record, so drec stays byte-equal.
- FACT sc_cur (nd_snap.c:113) is the snap core 0 applied last. sc_cur.in is an NdCombIn (nd.h:476); sc_cur.oml, .geom, .plod hold the other mode L, geometry and prim LOD words (nd_snap.h:18-20).
- FACT 3ds/tools/nd_direct_test.c:44 defines its own comb_in_walker for the host test. Keep it (c0gate counts 3ds/src and src only).
## Edits (Edit tool only: CRLF files. Each new or changed line ends with `/* NCS1 */`)
1. nd_snap.c after the line `const NdSnapBind* nd_snap_cur_b(void) { return sc_cur.b; } /* NDR1C */` add:
   `const NdSnapChk* nd_snap_cur(void) { return &sc_cur; } /* NCS1: the snap core 0 applied last (debug readers) */`
2. nd_snap.h after `const NdSnapBind* nd_snap_cur_b(void);` add: `const NdSnapChk* nd_snap_cur(void); /* NCS1, nd_snap.c, core 0: the last applied snap */`
3. nd_comb.c: delete the function `void comb_in_walker(NdCombIn* in) {` ... `}` (4 lines). Replace nd_walker_desc. Before:
```c
void nd_walker_desc(NdPsoDesc* d) {
    NdCombIn in; comb_in_walker(&in); memset(d, 0, sizeof *d);
    d->cw0 = in.w0; d->cw1 = in.w1; d->omh = in.omode_h; d->oml = nd_ws.omode_l; d->geom = nd_ws.geom;
```
   After:
```c
void nd_walker_desc(NdPsoDesc* d) { /* NCS1: the fields of the last applied snap */
    const NdSnapChk* s = nd_snap_cur(); const NdCombIn in = s->in; memset(d, 0, sizeof *d); /* NCS1 */
    d->cw0 = in.w0; d->cw1 = in.w1; d->omh = in.omode_h; d->oml = s->oml; d->geom = s->geom; /* NCS1 */
```
   In the last line of nd_walker_desc replace `d->plod = (uint8_t)nd_ws.prim_lod;` with `d->plod = (uint8_t)s->plod; /* NCS1 */`. Keep the other lines (they read in.*, pm_comb_vprim, nd_img_role).
4. The comment of nd_comb.c:174 says `the NdPsoDesc fields that comb_in_walker and nd_ws give`: change it to `the NdPsoDesc fields of the last applied snap (NCS1)`.
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, foreground only, 1 build, 8 Azahar starts)
1. Edits 1-4. `sh 3ds/tools/nd_host_test.sh > build3ds/out/ncs1_host.txt 2>&1`: PASS. Commit `NCS1 1: the walker description reads the applied snap; comb_in_walker goes`.
2. `python 3ds/tools/bench.py base spot_mac01_drec > build3ds/out/ncs1_base.txt 2>&1` (it names the basebin ELF of the base). 3. `sh 3ds/tools/rowloop.sh --row NCS1 --bench spot_mac01_drec --no-host --why "NCS1: walker desc from the snap" > build3ds/out/ncs1_loop.txt 2>&1`.
4. `mkdir -p build3ds/out/c0base && git archive $(git merge-base HEAD main) | tar -x -C build3ds/out/c0base`, then `python build3ds/out/c0base/3ds/tools/c0gate.py <basebin ELF named in ncs1_base.txt>` and `python 3ds/tools/c0gate.py build3ds/lane-NCS1/pm_3ds.elf`, both into build3ds/out/ncs1_c0.txt.
## Gate (exact): host PASS; LOOP PASS (spot_mac01_drec byte-equal); c0gate head minus base: calls -1, interp -1, state 0; the head interp_syms line has no comb_in_walker.
Fix rights: compile errors (a missing include of nd_snap.h is not possible: nd_comb.c:16 has it), unused warnings. A DIFF, a crash, other c0gate values or a tool fault: stop. Send the cmp path and its first DIFF line to oracle NATIVE14 with SendMessage. Wait.
## Hand-back: last commit `NCS1 2: gate. <lines>` with `git add -f build3ds/out/ncs1_loop.txt build3ds/out/ncs1_c0.txt`. It deletes this page. One SubagentHandback: hashes, cmp line, c0gate lines.
