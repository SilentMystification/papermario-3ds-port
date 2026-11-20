# ws_NC0T2 (Haiku): the last nd_c0 fields (timg, tlut, tlut_n, tst1, td) become the texture register nd_txw; nd_c0 goes (c0gate state 0)
Designer: oracle NATIVE17 (plan_perf.md section 7, Q3/Q5 default: the core 0 texture model is the static register, class C). WAIT NC0F: branch from
main after the NC0F merge (shared line pm_gpu.c:1236). Parallel with ALT2C (other lines). Worktree build3ds/wt/NC0T2, branch NC0T2. PM_ROW=NC0T2 PM_LANE=NC0T2.
Lines: main 0c20b5ebff; grep the quoted text. Why Haiku: step 1 is 3 closed edits under the existing bindsrc census; step 2 is one sed (byte-equal by construction).
Objective: two non-static readers take the recorded source (step 1); the rest moves to `NdTxw nd_txw` and NdC0 is deleted (step 2). Counter: c0gate 0 / 0 / 1 to 0 / 0 / 0.
## Facts (FACT code read, main 0c20b5ebff)
- NdC0 (nd.h:165-172) holds only timg, tlut, tlut_n, segbase (NC0F deletes it), tst1, td[2]. The texture setters (nd_tex_set.c, 85 lines) build them from
  the recorded texture units. Readers: the bind fallback ND_BTD/ND_BWR/ND_BSZ/ND_BSH when nd_bsrc is NULL (nd_snap.h:31-34: static calls after a copy
  texture unit, the glyph), rect_to_image (nd_rect.c:72, :81, :100, TMEM words), the depth query (nd_rect.c:167 timg), the model full key (nd_comb.c:197),
  debug (nd_ent.c:45-52, nd_rect.c:174, pm_gpu.c:1236-1237, :4239), the census walker blocks (nd_snap.c:137).
- Two readers decide on a non-static draw from the walker model: const_rgb_tex_alpha (nd_comb.c:67 `nd_c0.td[1].valid`, caller nd_direct.c:75, nd_ap is the
  direct snap) and the box pattern rectangle (nd_rect.c:247 and box_pattern_rect :115-143, tile_texel nd_tex_set.c:255-257). The rect block is set only at :249, after :247.
## Step 1: the two readers (commit `NC0T2 1`)
1. nd_comb.c:67 `c.t1_valid = nd_c0.td[1].valid;` to `c.t1_valid = nd_ap->in.t1_valid; if (__builtin_expect(c.t1_valid != nd_c0.td[1].valid, 0)) nd_txsrc_note();`
   nd_txsrc_note: cold noinline in nd_comb.c, in drec frames `pm_log_evt("txsrc f%u", frame)` once per frame.
2. nd_rect.c: move the 2 lines `nd_bsrc = rc_sn ? nd_snap_cur_b() : nd_ap->b; ...` and `nd_bsrc_chk(nd_bsrc, 3u);` (:249-250) to just before line 247.
   Line 247 before: `if (box_pat && nd_c0.td[0].valid && nd_c0.td[1].valid && box_pattern_rect(xl, yl, xh, yh, hs, hd)) { return; }`
   after: `if (box_pat && ND_BTD(0)->valid && ND_BTD(1)->valid && box_pattern_rect(xl, yl, xh, yh, hs, hd)) { nd_bsrc = NULL; return; } /* NC0T2 */`
3. box_pattern_rect (nd_rect.c:115, :131-134, :142-143): `nd_c0.td[n]` to `ND_BTD(n)->` (n = 0, 1). tile_texel (nd_tex_set.c:256-257): first line
   `const NdTd* const d = ND_BTD(n); /* NC0T2: the block of the rectangle */`, then `nd_c0.td[n].` to `d->` in that function.
## Step 2: the move (commit `NC0T2 2: gate`)
4. nd.h:165-173: replace the NdC0 typedef and `extern NdC0 nd_c0;` with
   `typedef struct NdTxw { const void* timg; const void* tlut; int tlut_n; NdTst tst1; NdTd td[2]; } NdTxw; /* NC0T2: the texture register: the TMEM model the recorded texture units build on core 0 (bind fallback of static calls and the glyph, TMEM rects); class C */`
   `extern NdTxw nd_txw;`
5. nd_state.c: delete `ND_HOTD NdC0 nd_c0;`. nd_tex_set.c after `struct TszLast tsz_last;` (:43): `ND_HOTD NdTxw nd_txw; /* NC0T2 */`.
6. `grep -rl "nd_c0\." 3ds/src 3ds/tools 3ds/include | xargs sed -b -i 's/nd_c0\./nd_txw./g'`. Example nd_snap.h:31 before `&nd_c0.td[n]`, after `&nd_txw.td[n]`.
7. Delete `NdC0 nd_c0;` in 3ds/tools/nd_direct_test.c:42 and nd_tgt_test.c:8. Check: `grep -rnw "nd_c0\|NdC0" 3ds/src 3ds/tools 3ds/include` prints comments only.
## Tests (build3ds/out/, last 20 lines). Each gate: 1 build, 2 Azahar runs. Base: `bench.py base <b> --commit $(git merge-base HEAD main)`.
1. Host `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS. Build. c0gate: step 1 0 / 0 / 1, step 2 0 / 0 / 0.
2. Step 1: hudfb_drec, wscmb_pause_drec byte-equal; `grep -c "bindsrc f"` and `grep -c "txsrc f"` 0. Step 2: spot_mac01_drec, fxparts_drec byte-equal.
Fix rights: compile errors; a host link error (add `NdTxw nd_txw;` to that test). A DIFF, a census line, a crash: stop, send the path to oracle NATIVE17.
Stop: `NC0T2 2: gate` deletes this page. Hand-back: one SubagentHandback: commit hashes, host, c0gate per step, the 4 cmp lines, census counts.
