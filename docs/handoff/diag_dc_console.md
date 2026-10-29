# diag_dc console: draw cost, O3DS build 1227-dc_dc2, yard

Source: build3ds/hw_pull/hw_o3ds_log.txt. Steady frames f1172-1183 (n 12, 81 draws). Medians [min-max]. f1184+ CPU rises to 90-360 ms after the pause menu closes, so excluded.

## Per-frame counts (n 12)
- closes all 334 [330-337]; empty 267 [263-270]; non-empty 67 [66-67] (dcsplit n)
- raw 34; palup 15 [14-16]; pmul 8; tevr 17; tb0 68; tb1 0; tb2 0; tbs 0
- flsplit tex us 7229 [6421-9400] (fl TEX is in flsplit, not gbisplit)
- sampled us: pal 792 [736-968]; bind 592 [488-824]; tev 456 [344-1080]; st 672 [632-808]; draw 1928 [1832-2704]; kick 96 [88-31224]
- gpusplit us: cpu 57983 [56094-602016]; fb 1432 [11-6056]; fe 1797 [84-1822]; idle 8.5 [4-43]; gdraw 8202 [6138-8280]; kick us 10 [8-14]
- gbisplit us: sync 1324; gv 1511; tex 14892 [14025-16831]
- frame wall: phase W f1157-1183 avg 78.3 ms, p95 60.2 ms, max 602.0 ms (window only)

## Per non-empty close (us per frame / 67)
| stage | us/frame | us/close | share |
|---|---|---|---|
| draw | 1928 | 28.8 | 43% |
| pal | 792 | 11.8 | 18% |
| st | 672 | 10.0 | 15% |
| bind | 592 | 8.8 | 13% |
| tev | 456 | 6.8 | 10% |

- Largest stage: draw, 28.8 us per close.
- Total per close: (7229 - 267 x 2.6 - 10) / 67 = 97.4 us. The 2.6 us empty cost is from diag_ec2.md, not measured here.
- Closes take 67 x 97.4 us = 6.5 ms per frame (9.4% of 69.3 ms). All closes: 7.2 ms (10.4%).
- Five stages sum to 66.3 us per close. 31 us per close is not in these fields.

## Azahar (diag_dc.md, DC1) vs console, us per close
- pal 3.1 vs 11.8; bind 3.2 vs 8.8; tev 2.1 vs 6.8; st 2.0 vs 10.0; draw 15.4 vs 28.8. Sum 25.8 vs 66.3 (2.6 x).
- Non-empty closes 41.7 vs 67. palup 6.9 vs 15. tb0 42.0 vs 68.
- Draw share: Azahar 60%, console 43%.

## Verdict (plan_draw_cost.md)
- DC3 (pal clears): not supported. Gate "pal largest" fails. Ceiling 0.79 ms per frame.
- DC4 (tick reads): not supported. Gate ">10 us per close" fails. 4 ticks x 1.024 us = 4.1 us per close, 0.27 ms per frame.
- DC5 (merge): not proven. tb0 68 per frame vs 67 closes. Merge count not measured. Range 0 to 0.88 ms per frame (plan ceiling: 9 x 97.4 us).
- No plan row targets draw (largest stage, includes UpdateContext).
- Caveats: one close in 8 timed (values x 8); 12 frames; no per-frame wall in log.
