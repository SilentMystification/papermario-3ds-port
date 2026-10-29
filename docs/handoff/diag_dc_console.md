# diag_dc console: draw cost, O3DS build 1227-dc_dc2, yard

Source: build3ds/hw_pull/hw_o3ds_log.txt (FTP pull). Steady frames f1172-1183 (n 12): 81 draws per frame, before the slowdown after the pause menu closes (f1184+ CPU 90 to 360 ms). Medians, [min-max].

## Per-frame counts and totals (n 12)
- closes, all (flsplit tex n): 334 [330-337]
- empty closes (flsplit tex e): 267 [263-270]
- non-empty closes (dcsplit n): 67 [66-67] (334 - 267 = 67)
- raw 34 [34-34]; palup 15 [14-16]; pmul 8 [8-8]; tevr 17 [17-17]
- tb0 68 [68-68]; tb1 0; tb2 0; tbs 0
- flsplit tex us: 7229 [6421-9400] (fl TEX fields are in flsplit, not gbisplit)
- sampled us: pal 792 [736-968]; bind 592 [488-824]; tev 456 [344-1080]; st 672 [632-808]; draw 1928 [1832-2704]; kick 96 [88-31224]
- gpusplit us: cpu 57983 [56094-602016]; fb 1432 [11-6056]; fe 1797 [84-1822]; idle 8.5 [4-43]; gdraw 8202 [6138-8280]
- gpusplit kick n 1, kick us 10 [8-14] (pm_ph_kick_us, subtracted below)
- gbisplit us: sync 1324 [1287-1528]; gv 1511 [1474-1611]; tex 14892 [14025-16831]
- frame wall: phase W f1157-1183 avg 78.3 ms, p95 60.2 ms, max 602.0 ms (window, no per-frame wall median in log)

## Per non-empty close (sampled us per frame / 67)
| stage | us per frame | us per close | share of 5 stages |
|---|---|---|---|
| draw | 1928 | 28.8 | 43.4% |
| pal | 792 | 11.8 | 17.8% |
| st | 672 | 10.0 | 15.1% |
| bind | 592 | 8.8 | 13.3% |
| tev | 456 | 6.8 | 10.3% |
| sum of 5 | 4440 | 66.3 | 100% |
- kick: 96 us per frame (once per frame, 1.4 us per close equivalent). Not in the sum.
- Total per non-empty close: (7229 - 267 x 2.6 - 10) / 67 = 97.4 us. The 2.6 us empty-close cost is from diag_ec2.md, not measured here.
- Closes per frame, non-empty: 67 x 97.4 us = 6.5 ms = 9.4% of 69.3 ms.
- Closes per frame, all: 7.2 ms = 10.4% of 69.3 ms.
- Not in the 5 stages: 2085 us per frame (about 31 us per close). Source not measured.

## Azahar (diag_dc.md, DC1 mean, 103 frames) vs console
| field | Azahar us/frame | console us/frame | Azahar us/close | console us/close |
|---|---|---|---|---|
| pal | 131.3 | 792 | 3.1 | 11.8 |
| bind | 131.7 | 592 | 3.2 | 8.8 |
| tev | 87.6 | 456 | 2.1 | 6.8 |
| st | 83.6 | 672 | 2.0 | 10.0 |
| draw | 643.2 | 1928 | 15.4 | 28.8 |
| sum of 5 | 1077 | 4440 | 25.8 | 66.3 |
- Non-empty closes: Azahar 41.7, console 67. palup: 6.9 vs 15. tb0: 42.0 vs 68. tevr: 12.9 vs 17.
- Draw share: Azahar 60% of the 5 stages, console 43%. Console pal and st are larger.
- Console total per close is 2.6 x Azahar (66.3 vs 25.8 us).

## Verdict per row (plan_draw_cost.md)
- DC3 (pal shadow clears): not supported. Gate "pal largest" fails: draw is largest. Ceiling 0.79 ms per frame (all pal time).
- DC4 (tick reads): not supported. Gate "above 10 us per close" fails. Ticks: 4 per close, 1.024 us each (log line 4, phase on, fine timing off): 4.1 us per close, 0.27 ms per frame at 67 closes.
- DC5 (merge on resolved state): not proven. tb0 68 per frame against 67 non-empty closes means about one unit-0 bind per close. Merge count not measured. Expected 0 to 0.88 ms per frame (plan ceiling: 9 closes x 97.4 us).
- No row in plan_draw_cost.md targets draw (28.8 us per close, largest). The draw field includes UpdateContext (plan item 14).

## Caveats
- Sampled fields: one close in 8 timed, values are sample sum x 8 per frame. 12 frames only.
- Frame wall per frame is not in the log. The 78.3 ms avg includes outliers. Use p95 60.2 ms.
