# Console numbers, build 1175, O3DS 192.168.1.170 (diagnosis only)
Source: build3ds/hw_pull/hw_o3ds_log.txt (108216 lines, FTP pull). Tools: `python 3ds/tools/console_numbers.py LOG [segments]`, `python 3ds/tools/attract_report.py LOG OUT ROW CR_OUT`.
Tag: [M] measured in this log, [I] inference. Wall is ms per drawn frame (phase W windows, 5947 drawn frames in 352 s of windows). gpusplit has 8055 lines (f1 to f8194).

## 1. Live log loss [M]
- build3ds/hw_live_log_170.txt: 1074 lines of the 108216 the game wrote (1.0 %). 115 gpusplit lines, 4 phase W windows, 2 logstat lines.
- Last live logstat (line #101099): dropped 64575 live, eagain 0, stall closes 0, bytes 147258 sent of 165294. Pull log logstat shows the same 64575.
- logstat2 (28 lines): writer max us file 360000 to 2020136 (SD write stalls up to 2.0 s), sock 327 to 593, console 1.58 to 1.81 s with "console skipped" up to 1402; queue 47 to 702 of 1024.
- Cause [I]: queue (1024) fills while the file and console writers stall for 0.4 to 2 s. The socket writer is fast (under 0.6 ms), so the live sink drops at enqueue, not on the wire.
- Pull log vs live: pull log lost only 139 gpusplit frames (8055 of 8194), so the file path is nearly complete. Live log is useless for numbers.
- logstat "frames 300 work 33 to 40 ms wall 34 ms" (71 lines, mean wall 34.5 ms) does NOT match phase W wall 35 to 86 ms per drawn frame [M]. logstat counts game ticks, not drawn frames [I]. Use phase W, not logstat wall.

## 2. Per scene table (ms per drawn frame, phase W, frame weighted) [M]
| scene | frames | wall | fps | logic | gbi(W) | gpu | tex | fbeg | fend | idle | audio | other | gdraw |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| LOGOS | 300 | 33.4 | 29.9 | 0.2 | 2.1 | 0.7 | 0.0 | 0.2 | 0.6 | 25.9 | 0.2 | 2.0 | 0.5 |
| INTRO | 1313 | 60.5 | 16.5 | 1.4 | 32.7 | 6.1 | 0.5 | 3.2 | 1.2 | 0.8 | 2.5 | 4.4 | 4.0 |
| TITLE | 623 | 35.6 | 28.1 | 0.3 | 16.6 | 3.5 | 0.2 | 2.5 | 0.6 | 3.7 | 0.6 | 2.5 | 2.9 |
| DEMO_ENTER | 248 | 50.3 | 19.9 | 4.6 | 19.5 | 4.5 | 0.9 | 1.4 | 1.2 | 5.9 | 1.4 | 6.3 | 2.1 |
| WORLD (yard) | 2179 | 69.3 | 14.4 | 6.5 | 34.3 | 5.9 | 1.9 | 0.9 | 1.1 | 1.3 | 3.2 | 7.4 | 3.1 |
| CHANGE_MAP | 155 | 53.0 | 18.9 | 4.4 | 26.4 | 3.8 | 1.1 | 1.8 | 1.1 | 0.9 | 1.2 | 6.6 | 2.5 |
| BATTLE | 947 | 51.6 | 19.4 | 2.5 | 22.7 | 5.2 | 0.9 | 1.6 | 1.1 | 1.7 | 1.5 | 7.8 | 2.4 |
| FILE_SEL | 84 | 48.4 | 20.7 | 0.3 | 22.0 | 5.1 | 0.8 | 2.1 | 1.0 | 6.1 | 1.0 | 4.7 | 3.5 |
| PAUSE | 97 | 84.3 | 11.9 | 1.5 | 40.4 | 11.5 | 2.2 | 2.1 | 0.7 | 0.0 | 7.3 | 9.3 | 5.2 |
(DEMO scenes: 22 frames, no phase window. Longest single WORLD segment: 570 frames, wall 86.5, gbi 45.4.)

## 3. Per scene W C X G ops closes tasks hops (us per frame) [M]
| group | frames | W | C (fl sum) | X (th+tm) | G | ops | closes | tasks | hops |
|---|---|---|---|---|---|---|---|---|---|
| LOGOS | 312 | 2120 | 807 | 166 | 5107 | 199 | 21 | 2.0 | 32 |
| INTRO | 2290 | 32687 | 9242 | 2767 | 8258 | 2659 | 278 | 2.0 | 245 |
| TITLE | 570 | 16575 | 8296 | 2860 | 6969 | 1106 | 196 | 2.0 | 41 |
| DEMO_ENTER | 146 | 19451 | 6878 | 2626 | 2180 | 585 | 43 | 1.2 | 73 |
| WORLD | 2718 | 34331 | 8847 | 3067 | 7608 | 4545 | 377 | 2.0 | 493 |
| CHANGE_MAP | 134 | 26446 | 6407 | 1831 | 6938 | 2505 | 194 | 1.8 | 290 |
| BATTLE | 1401 | 22702 | 7541 | 2282 | 7803 | 2113 | 184 | 2.0 | 183 |
| FILE_SEL | 80 | 22005 | 8469 | 2316 | 7314 | 1864 | 227 | 2.0 | 65 |
| PAUSE | 332 | 40381 | 12766 | 4084 | 8324 | 2366 | 330 | 2.0 | 36 |
Note: C is the flsplit sum. The attract_report PI2 check (fl vs exclusive phases) is 1.37 on this run, so C may double count; treat C as an upper bound [I].
Other fields (ms per drawn frame, console_numbers.py): WORLD task bg 6.6 main 40.5; fb 0.9 fe 1.1 idle 0.1; log+hud 1.4; tex hit 86 per frame = 1.4 ms (16 us per hit); tex miss 1.6 per frame = 1.6 ms (1.0 ms per miss); closes by class (n/empty): tex 281/207, cc 7/7, rect 46/44, tri 13/12, oth 28/28.
Texture: texstat f300 sample: hits 2624, miss 45, evict 0, uploaded 170 KB [M].
log+hud: 1.1 to 1.8 ms in every scene, above the 1 ms rule. PI5: log avg 926 us, hud avg 542 us [M].

## 4. Decision table, WORLD (yard) [M numbers, I verdict]
| test | number | result |
|---|---|---|
| tasks > 2 | 2.0 per frame (bg + main, key 0) | not met |
| hops/ops > 0.15 and W > 50 % wall | 493/4545 = 0.108; W 34.3 of 69.3 = 49.5 % | not met (close) |
| empty closes > 20 % | 298 of 375 closes empty = 79 % (tex class 207 of 281) | met, supports (a) in the empty-close form |
| C + X > W | 8.8 + 3.1 = 11.9 ms vs W 34.3 ms | not met, (b) not supported |
| G < 40 % cpu with fb near 0 | fb 0.9 ms, G 7.6 ms; GPU idle 0.1 ms | GPU does not wait; (b) not supported |
| fl TEX and CC dominate C, W/ops constant | fl tex 8.4 of 8.8 ms C; W/ops console 7.55 us (WORLD), 12.3 us (INTRO), 15.6 us (TITLE), 10.8 us (BATTLE) | W/ops is not constant; tex-class dominates C. (c) partly |
| fb > 30 % wall | 1.3 % | not met |
| logic > 40 % wall | 9.4 % | not met |
| log+hud > 1 ms | 1.4 ms | met: instrumentation cost |
Verdict [I]: (a) tasks/hops is not the cause (2 tasks, hops/ops 0.11). (b) is not the cause. The console frame is CPU-bound in the gbi walk. (c) display-list work supports best, with empty tex-class closes as the visible waste.

## 5. Azahar vs console [M for both columns, I for ratio]
| scene | Azahar W us/f | console W us/f | Azahar us/op | console us/op |
|---|---|---|---|---|
| WORLD | 9670 | 34331 | 2.8 | 7.6 |
| INTRO | 8948 | 32687 | 3.3 | 12.3 |
| TITLE | 4835 | 16575 | 4.4 | 15.0 |
| BATTLE | 6955 | 22702 | 3.2 | 10.7 |
Console is 3.5x slower per walk than Azahar for the same scene type [I: CPU speed, 268 MHz ARM11 and no branch prediction help]. User figure: yard walk 10 ms Azahar vs 27 ms console (build 1141, wall 53 ms). Build 1175 shows walk 34 ms, wall 69 ms: +7 ms walk and +16 ms wall over build 1141 [M]. Part of the rise is instrumentation (phase ticks 0.96 us each, 493 hops, log+hud 1.4 ms, "other" 7.4 ms) and part may be a different yard scene [I]. Do not compare 1175 wall with 1141 wall.

## 6. What dominates the console frame (WORLD, 69.3 ms) [M]
gbi walk 34.3 ms (49.5 %), of which tex-class closes (fl tex) 8.4 ms and texture hit/miss 3.1 ms. Then other 7.4, logic 6.5, gpu 5.9, audio 3.2, gdraw 3.1. fb/fe/idle 2.3 ms. The GPU is not the limit.
Walk residual after C and X: 34.3 - 8.8 - 3.1 = about 22 ms for 4545 ops = 4.8 us per op of pure interpretation [I].

## 7. Top 3 rows to attack
1. Display-list work cut: replay a cached draw list for static model/map lists (review_render_pipeline.md row 2). Ops 4545 per frame is the multiplier at 4.8 to 7.6 us. If 40 to 60 % of ops hit, W falls 14 to 20 ms (yard 69 to about 50 ms) [I].
2. Skip empty tex-class closes (207 of 281 tex closes empty, 8.4 ms in fl tex, 30 us per close). Close only when a draw follows. Expected 5 to 7 ms per yard frame [I].
3. Fix logging cost first: log+hud 1.4 ms, "other" 7.4 ms unaccounted, writer stalls up to 2 s drop 65 % of lines. Drop hud work (542 us avg) and raise queue or sample lines. Expected 1.4 to 2 ms, plus trustworthy live log [I]. Texture miss at 1.0 ms each is a fourth row (1.6 ms per frame, preload on map load).
