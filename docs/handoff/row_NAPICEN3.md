# Row NAPICEN3: the napicen census lists every caller (the 60 % "unmapped" is the top-40 cut)

Model: Haiku (one debug dump loop and a rerun of 3 existing scenes; no design choice).

## Cause of the gap (oracle code read)
napi_dump (3ds/src/nd_rec.c:414-426) prints only the 40 largest callers (`for (unsigned r = 0; r < 40u; r++)`). The total in the
done line counts all callers. napi_note (:427-438) also drops a caller silently when all 512 slots are taken (the probe loop
ends with no count). So the NAPICEN2 sums (262 of 547 and so on) are the top 40, not an unmapped share.

## Start
`sh 3ds/tools/wt_new.sh NAPICEN3`; PM_ROW=NAPICEN3 PM_LANE=NAPICEN3.

## Touch only
3ds/src/nd_rec.c (napi_dump and napi_note only).

## Exact edits
1. napi_note: a static `unsigned napi_lost;`. When the probe loop ends without a slot, `napi_lost++`.
2. napi_dump: print every caller with n > 0 in descending order (the same selection loop with `r < 512u`). The done line becomes
   `napicen f%u done total %u lost %u` (lost / 300 per frame, as total); reset napi_lost after the line.
3. Pre-flight: 3ds/tools/napicen.py reads `napicen f... ra ... n ...` lines and the done line by regex; check that the extra
   ` lost %u` field does not break its done-line regex (read the regex; if it anchors at the end, extend it in the same commit).

## Commands and gate
- Build; `python 3ds/tools/bench.py run napicen_mac01,napicen_talk,napicen_btl --row NAPICEN3 --why "napicen all callers"` (3 starts).
- `python 3ds/tools/napicen.py <log> --map <worktree>/build3ds/pm_3ds.map --label <scene>` per log, to
  build3ds/out/napicen3_<scene>.txt.
- Pass: in each scene the per-caller sum of the last window equals its done-line total within 1 % (rounding per caller), and
  lost is 0. lost above 0: stop and hand back (the table needs more slots: a design choice for the oracle).

## Hand back
Per scene: every producer file with ops per frame (all of them), and the total. Commit the code; delete this row file in the same
commit.
