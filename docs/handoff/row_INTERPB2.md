# Row INTERPB2: design B step 2: the K command lists as a patched copy of L (built and checked, not shown yet)

(Model: Sonnet (user approved). After INTERPB1 is merged; same coder. Design: row_ORACLE_NEXT.md, option B.)

## Design
During the L walk every float uniform write of a matrix-derived value writes the L words into the command buffer, as today, and adds
a patch (word offset in the frame's command lists, the K words from the INTERPB1 twins). At the L frame end the frame's command lists
(the GX queue entries of type 0x01 read in sr_take order, the INTERP2M icmp_take reader) are copied into a K buffer and the patches
applied. This row only builds the copy and compares it with the real key walk (still running): every word must match.

## Sites (oracle read; complete the list from the census)
- nd_emit_draw palette rows (3ds/src/nd_emit.c:91-94, `uni(o, d->loc_pal + 4u * k, d->pal[k], 4)`): add the patch with the K row.
  The shadow skip (:91) compares the row with the last written row: compare the pair (L and K) so a row skipped in L is not missing in
  K (store the K twin in the shadow NdShadow, nd.h:266-275).
- blk_put (pm_gpu.c:1103, HC6 cached block): its pal0 row patch gets a K patch.
- pm5 uniform block (pm5_upload_draw, pm_gpu.c:1067-1069), the eye uniform (eye_put :2022, same in both: no patch), the rect palette
  RECT_GM (ortho: same), any other site INTERPB1 listed.
- Completeness check: extend the INTERP2M census (switch icmp) to compare the patched copy with K: diff by class and, for U, the
  uniform location (the 0x2C0 location word before the data) of each differing word: `icmpp f<n> pairs .. diff U .. A .. T .. O ..
  locs <loc:n ...>`. A location left over names a missing site: add it.

## From INTERPB1
One upload_mvp per step differs between the twin and the key walk (no draw uses it: pal0 and nrm match). In the first run add
to twinchk the index of the first differing upload per step and its caller (__builtin_return_address(0) in upload_mvp) for 3 steps;
hand the lines back with the gate. The icmpp compare (all command words 0 diff) is the proof that no draw sees it.

## Try 1 to 2 (done)
Try 1: diff 0 but 39 % of patches unread (the open tail list). Try 2: the tail is read (icmp_take keeps the last list end, icmp_frame
reads up to the GPUCMD offset); the restore upload_mvp runs with pm_icmp_kind 0. No other site is expected by reading.

## Decision (after try 2: tail fix works; failing windows are mac01 f40, f51, btl f56, f59, all with lenmis > 0)
(a) cmul and cadd (locations 87, 88 in btl f59) need no twin: they are draw colour parameters (nd_gv_params), not matrix values. In
    f59 the lists already differ in length (lenmis 2) and twinchk shows pal0 54 nrm 82: while the battle camera moves, the L walk's CPU
    culls use L matrices, so the two walks draw different site sequences and the compare meets other draws. This is the known design
    limit (row_ORACLE_NEXT.md); the K copy shows L's draw set with K matrices.
(b) mac01 f51 is the scene start (warp at f49-f50): wl 14139 against wk 10545: the two frames differ in structure across the cut. Not a
    twin fault, but INTERPB3 must not show a copy across a scene change: it falls back to the key walk there (rule in row_INTERPB3.md).
(c) Unused patches in a window with lenmis > 0 are the boundary effect (the compare of a list stops at its length mismatch).
Gate rule: every window with lenmis 0 must have U, A, T, O 0 and unused 0 (try 2 meets it: mac01 all but f40 and f51, btl f51 and
f62-f81); windows with lenmis > 0 are reported. Use the 1 spare start for spot_mac01_fps60_det (12/12 byte-equal); then commit this
decision, delete the row file and hand back for the merge.

## Start
`sh 3ds/tools/wt_new.sh INTERPB2` (from main with INTERPB1). PM_ROW=INTERPB2 PM_LANE=INTERPB2.

## Touch only
3ds/src/nd_emit.c (nd_emit_draw palette, shadow pair), 3ds/include/nd.h (NdShadow twin rows, patch list API), 3ds/src/pm_gpu.c
(blk_put, pm5 block, the copy at the L frame end, the icmpp compare). No present change.

## Gate (CLAUDE.md 2-3)
- Build, memcheck OK (the K buffer and patch list bytes reported); nd_host PASS (nd_emit_test builds nd_emit.c).
- `bench.py run icmp_mac01,icmp_btl,spot_mac01_fps60_det --row INTERPB2 --why "patched K copy against the key walk"` (base of
  spot_mac01_fps60_det stored). Pass: icmpp diff U 0, A 0, T 0, O 0 in every window after frame 300 (boundary lenmis and nol as in
  INTERP2M allowed only in the first window); spot_mac01_fps60_det 12/12 byte-equal. Budget: 4 Azahar starts.

## Stop / At the end
A failed gate: commit, hand back the icmpp lines. Delete this row file in the completing commit; INTERPB3 follows.
