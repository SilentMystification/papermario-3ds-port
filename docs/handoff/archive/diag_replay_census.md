# Diagnosis: extra census batch with recplay on (kmr_10 entry 0, frame 200)

Branch nds1, HEAD 6c999c484. Scenario census_toad_rp, one Azahar start (justification line in 3ds/logs/perf/status_NDS1.txt).
Debug prints were added to pm_gbi.c and pm_gpu.c for frame 200 only and reverted. Build lane nds1.

## Fact (census of the run)

- cen f200 summary: batches 27, tris 346, distinct 24 (recplay on). census_toad control: 26, 338, 23.
- The one extra entry: key c5c1cb6c, tex 7b53f323 00000000, tris 8, n 1.

## Trace

| Event (frame 200) | Value |
|---|---|
| G_PM_MODEL group 0138f800 | slot 48, n 8 words, rc 0 (no capture), play 1 |
| rec_play | slot 48, G_DL list 012f3020, len 272 words, nd 1 |
| replay tuples (cen_put) | ncen 1: key c5c1cb6c, tex 7b53f323 00000000, tris 8 |
| draws recorded (nd) | 1 |
| draws replayed | 1 |
| live batches after the group | none with key c5c1cb6c |

- The same model does not draw twice. Group 0138f800 ran once. Its live words emit no tri op. Its G_DL replays the record once.
- No tuple mismatch. nd equals ncen at play (1 and 1).
- The extra batch is the one replayed draw of slot 48.

## Cause

Code path: pm_gbi.c rec_play (HEAD line 3752) runs before dlc_try at the G_DL (pm_gbi.c:3817). The record holds the whole list
as it was captured. A capture sets rec_on, and dlc_try (pm_dlc.inc.c:534) then draws every group without the group box test
(`!rec_on && dlc_box_out(g, m)`). When the box test is true, dlc_try skips the group. rec_play has no box test. It replays
every group of the record. So a group that the replay-off path skips is drawn when the replay is on.

- Status: INFERRED. The trace proves the replayed draw (slot 48, key c5c1cb6c, 8 tris). It does not prove that the replay-off run
  skips this group at frame 200, because no replay-off trace of group 0138f800 exists. The census absence of key c5c1cb6c
  in census_toad supports the skip. No other path in the replay-off run lets a G_DL list produce no draw without a census line.

## Fix (proposed, not applied)

pm_gbi.c rec_play (HEAD 3752, check before the pm_gpu_rec_play call at 3760): look up the dlc entry of l (dlc_find), and refuse
the replay (return 0) when any group g of the entry has dlc_box_out(g, combined). The dlc_try path then runs the skip as before.
The check reuses the game's own box test (allowed by the no-culling rule). Cost: one loop over ngrp groups per G_DL that replays.

## Real draw or census artifact

The draw is real. The replay sends 8 triangles to the GPU that the replay-off run does not send (INFERRED from the skip).
The picture effect on kmr_10 is not measured. The box test says the group lies outside a view plane, so no visible pixel
change is expected. The mac01t region diff (0 px) agrees, but it is another map.

## Next check (one start, only if the user asks)

census_toad with the same prints and a print of dlc_box_out for group 0138f800 (or for the G_DL 012f3020) at frame 200. A true
result proves the cause.
