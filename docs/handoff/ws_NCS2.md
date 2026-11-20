# ws_NCS2 (Haiku: 5 files, core 0 only, expected-diff UD-N5): the core 0 box fold goes; core 1 folds in the snap
READY. Start commit: main. WAIT: none (no data dependency; NCS1 edits other lines of nd_comb.c). Worktree build3ds/wt/NCS2, branch NCS2. PM_ROW=NCS2 PM_LANE=NCS2. Designer: oracle NATIVE14 (id in the launch prompt).
Objective: box_fold_sync (nd_comb.c:312) and tile0_white (:294) go. Core 1 already folds tile 0 into the snap (nd_snap_in.c:23-26), and sc_adopt copies the fold (in.tst, in.use_t1, in.box_folded) into the walker at each armed draw. At the 4 old call sites, a cold drec marker logs `bfold f<n>` once per drec frame where the old rule would have changed the fold. Counter C13: calls -5 (FACT c0gate: nd_direct.c:173, nd_geom.c:347, :399, nd_rect.c:243, and nd_comb_sync at nd_comb.c:327). interp -1 (FUNC box_fold_sync). state 0.
Regions (main bc9985bc5e, find by text): nd_comb.c :293-328; nd_geom.c :347, :399; nd_rect.c :243; nd_direct.c :173; nd_int.h :29; 3ds/tools/nd_direct_test.c :106. NCS1 edits nd_comb.c :166-182 only.
## Facts
- FACT box_fold_sync returns at once when `box_prod && nd_ws.td[1].valid && tile0_white()` equals box_folded (nd_comb.c:313-314). Only then does it write tst1, use_t1, box_save, comb_gen and call nd_comb_sync. So the marker test below is the exact condition of a change.
- FACT nd_td_white (nd_snap_in.c:30, declared nd_snap.h:50) is tile0_white on an NdTd. nd_comb.c includes nd_snap.h (:16).
- FACT pm_drec_now (pm_gpu.c:596) is 1 only in a recorded drec frame. pm_log_evt and pm_frame_count are used in nd_comb.c already (:196, :500).
## Edits (Edit tool only: CRLF files. Each new or changed line ends with `/* NCS2 */`)
1. nd_comb.c: delete tile0_white (the comment line `/* All texels of tile 0 are 1.0 ...` to its closing brace) and box_fold_sync (the 2 comment lines `/* Called before each textured draw. ...` to its closing brace). Put in their place:
```c
int pm_drec_now(void); /* NCS2: pm_gpu.c */
static __attribute__((cold, noinline)) void bfold_log(void) { static unsigned fr = ~0u; if (pm_frame_count() != fr) { fr = pm_frame_count(); pm_log_evt("bfold f%u", fr); } } /* NCS2 */
void nd_bfold_note(void) { if (__builtin_expect(pm_drec_now(), 0) && (box_prod && nd_ws.td[1].valid && nd_td_white(&nd_ws.td[0])) != box_folded) bfold_log(); } /* NCS2: the drec marker of the deleted core 0 box fold (UD-N5); NWS1 deletes it */
```
2. The 4 call sites: replace `box_fold_sync();` with `nd_bfold_note();` and add ` /* NCS2 */` at the line end. Example nd_geom.c:399. Before: `            if (ep_osc != gv_epoch) { nd_img_check(); box_fold_sync(); ep_osc = gv_epoch; }`. After: `            if (ep_osc != gv_epoch) { nd_img_check(); nd_bfold_note(); ep_osc = gv_epoch; } /* NCS2 */`. The same for nd_geom.c:347 (the PG4 line), nd_rect.c:243 and nd_direct.c:173 (glyph).
3. nd_int.h:29. Before: `void box_fold_sync(void);`. After: `void nd_bfold_note(void); /* NCS2, nd_comb.c: the drec marker of the deleted core 0 box fold */`.
4. 3ds/tools/nd_direct_test.c:106. Before: `void box_fold_sync(void) {}`. After: `void nd_bfold_note(void) {} /* NCS2 */`.
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, foreground only, 1 build, 8 Azahar starts)
1. Edits 1-4. `sh 3ds/tools/nd_host_test.sh > build3ds/out/ncs2_host.txt 2>&1`: PASS. Commit `NCS2 1: the core 0 box fold goes; bfold drec marker`.
2. `python 3ds/tools/bench.py base wscmb_pause_drec > build3ds/out/ncs2_base1.txt 2>&1`, the same for fxparts_drec (ncs2_base2.txt). wscmb_pause: pause windows (box product), glyphs. fxparts: HUD rectangles, particle triangles (nd_geom.c:399).
3. `sh 3ds/tools/rowloop.sh --row NCS2 --bench wscmb_pause_drec,fxparts_drec --no-host --why "NCS2: core 0 box fold goes" > build3ds/out/ncs2_loop.txt 2>&1`.
4. For each bench B: `grep -c "bfold f" build3ds/azt/logs/B.txt` and, with F the drec frame of the first DIFF in build3ds/out/loop_NCS2_cmp_B.txt, `grep -n "bfold fF$" build3ds/azt/logs/B.txt`. Seeded check: `grep -n "bfold f999999$" build3ds/azt/logs/B.txt` prints nothing. All lines into build3ds/out/ncs2_bf.txt.
5. `mkdir -p build3ds/out/c0base && git archive $(git merge-base HEAD main) | tar -x -C build3ds/out/c0base`, then `python build3ds/out/c0base/3ds/tools/c0gate.py <basebin ELF named in ncs2_base1.txt>` and `python 3ds/tools/c0gate.py build3ds/lane-NCS2/pm_3ds.elf`, both into build3ds/out/ncs2_c0.txt.
## Gate (exact, UD-N5): host PASS. Each bench is byte-equal, or its first DIFF frame F has a `bfold fF` line. The seeded check prints nothing. c0gate head minus base: calls -5, interp -1, state 0; head interp_syms has no box_fold_sync. Report the bfold counts (not judged; GUESS 0).
Fix rights: compile errors, prototypes, unused warnings. A DIFF with no bfold line at F, a crash, other c0gate values or a tool fault: stop. Send the cmp path and ncs2_bf.txt to oracle NATIVE14 with SendMessage. Wait.
## Hand-back: last commit `NCS2 2: gate. <lines>` with `git add -f build3ds/out/ncs2_loop.txt build3ds/out/ncs2_bf.txt build3ds/out/ncs2_c0.txt`. It deletes this page. One SubagentHandback: hashes, cmp line and bfold lines per bench, c0gate lines.
