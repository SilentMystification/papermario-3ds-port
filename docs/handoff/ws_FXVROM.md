# ws_FXVROM (Haiku): a vertex load past its array end splits in ROM order; 33 effect lists move to the twin
Objective (FACT, oracle dry run on main 1e45bdcad9, scratch worktree): converter unsup 19 -> 12, inherit 40 -> 21, tables 1745 -> 1771, gset ok
1660 -> 1693; gset moved 33 (lil_oink, snowman_doll); C5 data 5149 -> 4233, C6 data_arrays 461 -> 428, C7 gfxtype 1083 -> 1079.
Worktree build3ds/wt/FXVROM, branch FXVROM. PM_ROW=FXVROM PM_LANE=FXVROM on every bench call.
Files: 3ds/tools/nd_gfx_conv.c, 3ds/tools/nd_gconv.h, 3ds/tools/nd_gtab_test.c, 3ds/tests/host.d/vrom.sh (new), 3ds/tools/gset.d/FXVROM.txt (new),
and the files gset.py rewrites (src/effects/gfx/lil_oink.c, snowman_doll.c, their gfxin twins, src/effects/lil_oink.c, snowman_doll.c).
Do not edit gset.py, gset.sh or gen_gfx_syms.py (coder GSETI owns gset.py). Do not edit nd_gtab.h, model.c or any NDN reader.
## Rule (the device keeps ROM order: build3ds/pm_3ds.map D_09001000_3E3960 0x4e0a20, D_09001070_3E39D0 0x4e0a90 = +7 vertices)
A gsSPVertex that reads past its array end reads the next array in ROM order. The converter splits such a load into pieces. The next piece
reads the Vtx symbol whose name ROM hex equals hex + 16 * length. No such symbol: the load stays whole. The host compare (m_tri) maps an index
past the array end to the same next array. Only the host tables (NdNTab, ND_HOST_GFX) change. The device unit words (ndnu) do not change.
## Edit 1 (block A of ws_FXVROM_code.md: one script makes all 4 edits)
Example (nd_gfx_conv.c:539). Before: `o.kind = ND_VS_VTX; ... o.first = off / 16u;` then `v2_seg_add(&o);`.
After: the same line, then `while (ROM_SPLIT && o.first + o.n > gconv_vtx_len(vs)) { ... }` (adds one piece per array), then `v2_seg_add(&o);`.
Example (nd_gtab_test.c:380-381). After the two lines `e->base[j] = ...; e->i[j] = ...;` the loop moves (base, index past end) to the next array.
## Edit 2: block B of ws_FXVROM_code.md as 3ds/tests/host.d/vrom.sh.
## Edit 3: `ls src/effects/gfx/*.c > 3ds/tools/gset.d/FXVROM.txt` (118 lines, LF).
## Loop (1 build, at most 8 Azahar starts; base runs not charged)
1. Edit 1 and 2. `sh 3ds/tools/nd_host_test.sh > build3ds/out/fxvrom_host1.txt 2>&1`. Pass: `ND HOST TESTS PASS`, `vrom ok`,
   `gconv ntab tables 1771 inherit 21 unsup 12`, `gconv gset ok 1693 of 1763 lists`, conv2c ok run `ntab tables 1771 draws 35637 bad 0`, `ntab moved 1616`. Commit `FXVROM 1`.
2. `python 3ds/tools/gset.py 3ds/tools/gset.d/FXVROM.txt build3ds/out/gset_ok.txt > build3ds/out/fxvrom_gset.txt 2>&1`.
   Pass: `gset moved 33 files 2 keep 0` and `gset ok`. Another number: stop, send the output path to the oracle.
3. Host test again (> build3ds/out/fxvrom_host2.txt). Pass: PASS, `vrom ok`, `gconv lists 1763 parts 179 defs 1903 ops 19236 bad 0 moved 1611`, conv2c ok run `ntab tables 1771 draws 35637 bad 0`, `ntab dev entries 1730 bad 0`, `ntab moved 1649`.
4. One build. memcheck margin above 0 (it is 434 KB; GUESS -2 to -10 KB). `python 3ds/tools/listcen.py`: data 4233, data_arrays 428.
   Commit `FXVROM 2` with the listcen totals and the margin.
5. Gate. For each of spot_mac01_drec, sbk14_drec, fxr_drec, fxp_drec: `python 3ds/tools/bench.py base <b>`, then
   `python 3ds/tools/bench.py run <b> --row FXVROM --why "FXVROM gate"`, then `python 3ds/tools/bench.py cmp <b> --row FXVROM --commit $(git merge-base HEAD main)`.
   Pass: 1/1 byte-equal each (one differing bit fails). A cold first fxr_drec cmp: one warm rerun (bench.py rule).
## Fix scope
Only the block A edits, vrom.sh and the set file. A host compare failure: send build3ds/out/fxvrom_host*.txt to the oracle. Do not change the rule.
## Stop and hand-back
Stop: all 4 gates pass. Final commit `FXVROM 3: gate` with the values. That commit deletes this page and ws_FXVROM_code.md.
Hand-back: build3ds/out/handback_FXVROM.txt (line 1 commit hash, then gate values, listcen change C5/C6/C7, memcheck margin).
Any other failure after 2 tries: write the output to build3ds/out/ and send the path to the oracle (SendMessage). Then stop.
