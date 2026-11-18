# ws_NCONV2B (Haiku): the walker takes the combine and render fields of each consumed snap (adopt). Prep for NCONV2D (lists skip combine units). Counter: none of C1-C12. Gate: 2 drec byte-equal, snapchk lines equal base.
Designer: oracle NCONV2RA. Start from main (NCONV2A merged, 73359b439a). No WAIT: NREC4 edits nd_snap.c lines 29-71, this row edits only the nd_snapchk_tri block (nd_snap.c:228-232). Worktree build3ds/wt/NCONV2B, branch NCONV2B. PM_ROW=NCONV2B PM_LANE=NCONV2B.
Edit only (Edit tool, keep CRLF): 3ds/src/nd_snap.c.
## Facts (code read, main 73359b439a; find the quoted text, line numbers move)
- FACT nd_snapchk_tri (nd_snap.c:228-232) consumes each armed snap of a draw: draw_tri (nd_geom.c:354), dlc_seg (nd_shape.c:437), rectangles (nd_rect.c:244), glyphs (nd_direct.c:170).
- FACT sc_compare (nd_snap.c:151-196) runs first under PMDL("snapchk"). It names each walker field that differs from the snap. The walker sources are comb_in_walker (nd_comb.c:200-206) and nd_ws.omode_l, fogc, blendc.
- FACT the fields of this row are compare names w0 w1 omh prim env keyc keys cvk4 tst tex_on use_t1 dual_on box_prod box_folded comb_set comb_alpha_shade oml fogc blendc. A write of an equal value changes nothing.
- FACT base fxops_snapchk f500: bad 469, the logged names are "sl" only. sl_const is not a field of this row (nd_comb_apply nd_comb.c:312 writes it).
- FACT geom and tss stay core 0 state: the vertex load reads them (nd_geom.c:463, :491). This row does not write them.
## Edits
1. nd_snap.c. Above `void nd_snapchk_tri(void) {` add:
```c
/* NCONV2B: the walker takes the combine and render inputs of the consumed snap (the 1:1 fields of comb_in_walker, nd_comb.c:200).
 * sc_compare names each of these fields when it differs, so with no bad name the writes change nothing. geom and tss stay core 0 state. */
static void sc_adopt(void) {
    extern unsigned* const tst_f[16]; /* nd_comb.c:46 */
    const NdCombIn* i = &sc_cur.in;
    nd_ws.comb_w0 = i->w0; nd_ws.comb_w1 = i->w1; nd_ws.omode_h = i->omode_h; nd_ws.prim = i->prim; nd_ws.envc = i->env;
    keyc = i->key_c; keys = i->key_s; conv_k4 = i->conv_k4;
    for (int k = 0; k < 16; k++) *tst_f[k] = i->tst[k];
    nd_ws.tex_on = i->tex_on; nd_ws.use_t1 = i->use_t1; dual_on = i->dual_on; box_prod = i->box_prod; box_folded = i->box_folded;
    comb_set = i->comb_set; comb_alpha_shade = i->comb_alpha_shade;
    nd_ws.omode_l = sc_cur.oml; fogc = sc_cur.fogc; blendc = sc_cur.blendc;
}
```
2. nd_snapchk_tri. Before: `    nd_snap_comb();` After: the same line, then a new line `    sc_adopt(); /* NCONV2B */`.
## Steps (one command per step, output to build3ds/out/, read the last 20 lines; Bash `timeout: 600000`; 1 lane build; at most 4 Azahar starts of 8)
1. Edits 1-2. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nconv2b_host.txt 2>&1`: PASS. Commit `NCONV2B 1: the walker takes the fields of the consumed snap`.
2. `python 3ds/tools/bench.py base fxops_drec --commit $(git merge-base HEAD main) > build3ds/out/nconv2b_base1.txt 2>&1`. Then `sh 3ds/tools/rowloop.sh --row NCONV2B --bench fxops_drec --no-host --why "NCONV2B adopt" > build3ds/out/nconv2b_loop1.txt 2>&1` (it builds).
3. The same 2 commands for entcape_kmr03_drec (outputs nconv2b_base2.txt, nconv2b_loop2.txt).
4. `python 3ds/tools/bench.py base fxops_snapchk --commit $(git merge-base HEAD main) > build3ds/out/nconv2b_base3.txt 2>&1`. Then `PM_ROW=NCONV2B PM_LANE=NCONV2B python 3ds/tools/bench.py run fxops_snapchk --row NCONV2B --why "NCONV2B snapchk" > build3ds/out/nconv2b_run3.txt 2>&1`.
5. Step 4 for entcape_kmr03_snapchk (nconv2b_base4.txt, nconv2b_run4.txt). Then `grep -a "snapchk f\|lsnap f" build3ds/azt/logs/fxops_snapchk.txt build3ds/azt/logs/entcape_kmr03_snapchk.txt build3ds/basecache/$(git merge-base HEAD main)/*_snapchk/run.log > build3ds/out/nconv2b_lines.txt`.
Gate (exact): host PASS. Both rowloops exit 0 (byte-equal). For each snapchk scene the last `snapchk f` and `lsnap f` lines of the row run equal the base lines.
Stop: a DIFF, a snapchk or lsnap line that differs from base, a crash, a fatal line, a tool fault. Commit the outputs (`git add -f`), send the paths to the oracle of the launch prompt with SendMessage, wait. Fix rights: nd_snap.c, compile errors only. Pitfalls: one host test at a time. One bench per rowloop call. PM_CACHE=off only as a prefix of one command.
Hand-back: last commit `NCONV2B 2: gate. fxops and entcape drec equal; snapchk lines equal base` deletes this page. One SubagentHandback: commit hash, the 4 last snapchk and lsnap lines.
