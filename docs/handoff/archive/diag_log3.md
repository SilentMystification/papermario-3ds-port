# diag_log3: does logging cost frame time (O3DS, build pm_3ds_1227-dc_dc2)

Source: build3ds/hw_pull/hw_o3ds_log.txt (6701 lines). Read-only. Steady frames = gpusplit cpu (frame wall) < 200000 us, n = 483 of 549.

## Numbers
- gpusplit log field (record copy at frame end, us): median 12, range 9 to 16 in steady frames. Mean 11.9 over all 549 lines.
- gpusplit hud field (us, only with dbg_hud): median 27 in the slow phase, 6 to 8 in the first frames.
- Frame wall (gpusplit cpu, us): steady median 33946, p10 30318, p90 38335.
- Log high (log >= 13, n=109) vs log low (log <= 11, n=254): wall 36071 us vs 37654 us. Idle 17971 us vs 17562 us. No cost signal.
- Slow phase f361 to f418 (wall 200 to 400 ms, f418 at 3117 ms; gproc 140 to 1000 ms): log stays 10 to 15 us.
- logstat window 1 (frames 1 to 300): 10.1 lines/frame, 862 bytes/frame, writer on core 1, dropped 0, eagain 0, stall closes 0, fbuf waits 0, queue 469 of 1024.
- logstat window 2 (last 300 frames before f419, slow phase): work 325 ms, wall 327 ms, 7.1 lines/frame, 660 bytes/frame, dropped 0, fbuf waits 0, queue 2 of 1024. Writer max: sock 1422103 us, console 1670638 us.
- taskpl per steady frame: bg 2551 us, main 8805 us, ops 772.
- gbisplit per steady frame: sync 473 us, gv 427 us, tex 4635 us.
- rsplit per steady frame: tile 34, comb 12, xdup 33, clip 1.5, pcomp 1.0 (counts).
- dcsplit per steady frame: raw 4.2 closes, pal 327, bind 433, tev 196, st 308, draw 954, kick 1512 us (sampled x8).
- Not in log: pacestat (no lines), phsum (header only), phase per-frame avg (259 lines, not scanned).

## Verdict
Logging does not cost more than 1 ms per frame on the game thread, as far as the data shows. The measured record copy costs about 12 us per frame. Frame wall does not rise with log cost.
Limit: the log field times only the phase record copy. Enqueue cost of pm_log_d calls is not timed. Window 2 writer stalls (1.4 s sock, 1.7 s console) did not block the game thread: no fbuf waits, no drops.
Caveat: logstat wall (41 ms, window 1) differs from gpusplit median (34 ms).

## Next step
Add a tick sum around the pm_log_d enqueue path, printed in the logstat2 line, then run one Azahar window (PM_ROW set) and compare the sum with frame wall.
