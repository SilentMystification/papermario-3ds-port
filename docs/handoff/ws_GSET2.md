# ws_GSET2: the rest of the effect lists leave the device (gset.py of ws_GSET1 on every converted effect source; moves C5, C6, C12)
Model: Haiku. PM_ROW, PM_LANE = GSET2. Budget: 4 Azahar runs. Start after `Merge GSET1` shows in `git log --oneline main` (same tool and twin folder). Branch GSET2.
Files: new 3ds/tools/gset.d/GSET2a.txt and GSET2b.txt, 3ds/tools/gfxin/ (twins the tool writes), the src lines gset.py rewrites. No tool edit: a tool fault is an oracle question.
FACT (main 1149b5513): the converted effect sources are the names of 3ds/tools/gconv_src.d/000_base.txt and S7EFX1.txt .. S7EFX8.txt (bare names, gen_gfx_tab.sh:15 maps
`<g>` to src/effects/gfx/<g>.c). gset.py moves only lists named in build3ds/out/gset_ok.txt (ws_GSET1 step 6), so a list that is not ok stays.
## Groups (per group: set file, tool run, nd_host, 1 build, 2 runs, commit `GSET2 <a|b>: ...`)
1. Set file (a: 000_base S7EFX1 S7EFX2 S7EFX3 S7EFX4; b: S7EFX5 S7EFX6 S7EFX7 S7EFX8):
   `cat $(for r in <rows>; do echo 3ds/tools/gconv_src.d/$r.txt; done) | sed 's/#.*//' | tr -s ' \t\r\n' '\n' | grep -v '^$' | sed 's|.*|src/effects/gfx/&.c|' | grep -v '/fright_jar\.c$' > 3ds/tools/gset.d/GSET2<a|b>.txt`
2. `sh 3ds/tools/nd_host_test.sh > build3ds/out/host_gset2.txt` (writes gset_ok.txt), then `python 3ds/tools/listcen.py > build3ds/out/lc_gset2_before.txt`.
3. `python 3ds/tools/gset.py 3ds/tools/gset.d/GSET2<a|b>.txt > build3ds/out/gset2<a|b>.txt`. Last line `gset moved <k> files <f> keep <c>`.
   keep above 0: edit each kept line as ws_GSET1 step 8 (nd_draw_list(NDN(X), <its desc>)) or remove X from its twin by hand, then go on.
   Example (blast.c, ws_GSET1 facts) Before `extern Gfx D_09001300_37ECD0[];` After `NDN_DECL(D_09001300_37ECD0);`; Before `nd_draw_list(D_09001300_37ECD0, ND_SELF);`
   After `nd_draw_list(NDN(D_09001300_37ECD0), ND_SELF);`.
4. `sh 3ds/tools/nd_host_test.sh`: PASS, the `ntab moved <k>` line above the value before the group, every `bad 0` line. A `gconv: moved <X>: <reason>` exit: move X's include
   line back from the twin to its device source, rerun step 4.
5. Build rc 0 (a link error names a caller left: move that list back as in step 4), memcheck margin 310 KB or more.
6. Gate: `bench.py run gold_12_drec --row GSET2 --why "GSET2 <g>"`, `bench.py cmp gold_12_drec --row GSET2 --commit 930de29e1`; `bench.py run fxpic_91_drec`, `bench.py cmp
   fxpic_91_drec --row GSET2 --commit d29deacc3`: both byte-equal. listcen after: data and data_arrays fall (values in the commit), build and gfxpos unchanged.
The group b commit deletes this page. Fix rights: the set files, the twins, the rewritten lines. A cmp difference or a host FAIL: 2 tries per named cause, then no commit,
build3ds/out/gset2.txt, send the path to the oracle. Hand back 8 lines: hashes, gset moved and keep per group, ntab moved, listcen data and data_arrays before and after, cmp, margin.
