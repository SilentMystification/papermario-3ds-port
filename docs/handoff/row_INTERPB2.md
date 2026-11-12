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
