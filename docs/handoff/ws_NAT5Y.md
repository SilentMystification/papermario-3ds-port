# ws_NAT5Y (Haiku): the glyph fallback units go into a NAT5R alt range. The fallback binds the tiles it just loaded, not the tiles of the previous glyph (user UD-N6 yes). Counter: c0gate calls -1 (msg_draw.c:1749). Gate: drec byte-equal, with the glyphalt exception.
Designer: oracle NATIVE3 (agent a86ed4bedc3a8575e). WAIT: `Merge NAT5F`. Start commit main after it. Worktree build3ds/wt/NAT5Y, branch NAT5Y. PM_ROW=NAT5Y PM_LANE=NAT5Y. Edit only: src/msg_draw.c.
## Facts (main cbacc2f7c8 plus NAT5F)
- FACT core 1 takes the glyph snap before the glyph units (msg_draw.c:1863 nd_snap_mesh, :1864 record, :1865 msg_glyph_feed). msg_glyph_run (:1731-1750) does not drop the snap, so the fallback texture rectangle binds nd_snap_cur_b (nd_rect.c:250): the tiles before the loads.
- FACT nd_rec_alt (nd_rec.c, NAT5R) records a texture rectangle with a new snap (nd_rec_texrect) after the loads. NAT5R round 1 with this edit: wscmb_pause_drec DIFF at draw 13 (f140). Without it: byte-equal.
- FACT UD-N6 (user, yes): a DIFF bench passes when its log has a `glyphalt` line at or before the DIFF frame.
## Edits (new lines end with `/* NAT5Y */`)
1. Record site, before (:1864-1865) and after:
```c
        nd_rec_napi_s(msg_glyph_run, &rec, sizeof rec / 8u);
        msg_glyph_feed((const uint32_t*)&rec);
        { uint32_t au[48]; uint32_t* q = au; msg_glyph_emit(&rec, &q); nd_rec_alt(msg_glyph_run, &rec, sizeof rec / 8u, 1, au, (unsigned)(q - au) / 2u, 0); } /* NAT5Y: the units as the alt range (no sc_lost: the old nd_ds_feed) */
```
2. Fallback, before (:1748-1749) and after:
```c
    msg_glyph_emit(r, &nrp);
    nd_nr_exec(buf, (unsigned)(nrp - buf) / 2u);
    { static unsigned gfr = ~0u; if (pm_drec_now() && pm_frame_count() != gfr) { gfr = pm_frame_count(); pm_log_evt("glyphalt f%u", gfr); } } /* NAT5Y: the UD-N6 marker */
    nd_alt_run(u + 2u * (unsigned)(sizeof(NdGlyphRec) / 8u)); /* NAT5Y */
```
3. Delete msg_glyph_feed (:1752-1758) and the `buf`, `nrp` lines of msg_glyph_run. Add `int pm_drec_now(void); /* NAT5Y: pm_gpu.c */` next to the pm_log_evt prototype (:1710).
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, 1 build, 8 Azahar starts of 8)
1. Edits 1-3. `grep -n "msg_glyph_feed\|nd_nr_exec" src/msg_draw.c` prints nothing. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat5y_host.txt 2>&1`: PASS. Commit `NAT5Y 1: glyph fallback units as an alt range (UD-N6)`.
2. For B in spot_mac01_drec, entcape_kmr03_drec, fxparts_drec, wscmb_pause_drec: `python 3ds/tools/bench.py base <B> > build3ds/out/nat5y_base_<B>.txt 2>&1`. 3. `sh 3ds/tools/rowloop.sh --row NAT5Y --bench spot_mac01_drec,entcape_kmr03_drec,fxparts_drec,wscmb_pause_drec --no-host --why "NAT5Y: glyph alt (UD-N6)" > build3ds/out/nat5y_loop.txt 2>&1`.
4. `grep -H "glyphalt\|c0fb\|drec frame\|drec draw .* DIFF" build3ds/azt/logs/*_drec.txt build3ds/out/loop_NAT5Y_cmp_*.txt > build3ds/out/nat5y_c0src.txt`. 5. `python 3ds/tools/c0gate.py build3ds/lane-NAT5Y/pm_3ds.elf > build3ds/out/nat5y_c0gate.txt 2>&1`. The base is the start-commit ELF of the bench base lane.
## Gate (exact): host PASS. Each of the 4 benches is byte-equal, or is DIFF with a `glyphalt f<n>` line at or before the frame of its first DIFF draw (UD-N6). The DIFF frame is the last `drec frame` line before the DIFF draw in the row log. c0gate calls = base - 1, state 23, interp as base. Fix rights: compile errors, prototypes. A DIFF with no such glyphalt line, a crash or a tool fault: stop, send nat5y_c0src.txt and the first DIFF line to oracle NATIVE3 (a86ed4bedc3a8575e), wait.
## Hand-back: last commit `NAT5Y 2: gate. <gate lines>; UD-N6` with `git add -f build3ds/out/nat5y_c0src.txt build3ds/out/nat5y_loop.txt`. It deletes this page. One SubagentHandback: hash, gate line per bench (equal, or DIFF frame and glyphalt line), c0gate line.
