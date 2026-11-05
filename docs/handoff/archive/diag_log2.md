# log2 (PI6 check) numbers

Branch: worktree-agent-ab8fe316a098f83b3. Commit 0a42da50b (PI6) merged. No code change in this row.

## Builds
- Base: build3ds/lane-log2/base_pm_3ds.3dsx (13360412 bytes, HEAD before PI6, no "sink thread" string).
- PI6: build3ds/lane-log2/pi6_pm_3ds.3dsx (13362228 bytes, built PM_LANE=log2, "fbuf waits" string present).

## Yard run (preset=yard, loglevel=debug logstat phase, 90 s, 300 frames)

| | base | PI6 |
|---|---|---|
| logstat dropped | 0 | 0 |
| logstat stall closes | 0 | 0 |
| queue (of 1024) at last logstat | 15 | 6 |
| gpusplit log, average (us, 9478 lines base, 10597 lines PI6) | 653 | 3.2 |
| gpusplit hud, average (us) | 160 | 2.1 |
| log + hud per frame (us) | 813 | 5.3 |
| writer max us, file / sock | 461 / 474 | 494 / 548 |
| console step max us | 122359 (writer) | 327741 (sink) |
| console lines skipped | 1 | 358 |
| fbuf waits | n/a | 0 |
| wall per frame (ms) | 33.42 | 33.42 |
| work per frame (ms) | 14.53 | 13.88 |

Numbers are Azahar only. The console judgement is for the user.

Reading: the game thread cost of log plus HUD falls from 0.81 ms to 0.005 ms per frame in Azahar. The sink thread console step reaches 328 ms max. It blocks the sink only. 358 console lines were skipped, but log.txt and the socket keep every line.

## Picture
- Run: PM_LANE=log2, -Until "shot 40 frame", -Capture, preset=yard battlef=80, shotmode=4:80. Matched at 16 s.
- last_shot.png: Mario on the green pipe, Luigi beside him, the house, the fence and ground are shown. Shadow is visible under Mario. The bottom screen shows the memory rows (FCRAM, VRAM, heap) and the phase lines, and they change.

## Statehash (fixedseed, statehash=100, PI6)
- toad: f100 94083528, f200 83b272fe, f300 aed188c8. Equal to the recorded base (diag_dl).
- battle: f100 94083528, f200 dda688fa, f300 6b6fe648. Equal to the recorded base.

## Thread check (code read)
- fb_head and fb_tail are counters under fb_lock. The sink copies bytes out under fb_lock, writes outside it, then advances fb_tail under fb_lock. The writer checks room under fb_lock. Lock order is fsink_lock, then fb_lock. No inverse order was found.
- Phase ring phr_head is written by the game thread with release stores. phr_tail is written by the writer with release stores. Each side reads the other with acquire loads. The ring has 64 records. Overflow adds to phr_dropped and is reported.
- hud_rows are written by the game thread and read by the sink without a lock. A row can tear for one draw. The flag hud_new is volatile. This is accepted and is stated in the code comment.
- sink_file_max, sink_con_max and con_skipped are read by logstat without an atomic. Only the value can be off by one step.

## Open items
- O3DS has one app core. Sink and writer run at priority 0x3F on core 0, the same level as the writer in the base. The game thread runs at a higher priority. Frame time on O3DS is not measured here. Check pacestat and the console on O3DS.
- The 2 s summarize and ps_step bucket lines still run on the game thread (unchanged from PI6).
- writer hb differs (16067 base, 4104 PI6). The heartbeat counts writer wakes. The PI6 writer wakes less often because the sink takes the file work. Not checked further.
- Azahar budget row log2: 5 of 8 starts used (base run, PI6 yard, PI6 picture, PI6 toad, PI6 battle). One cached run (the first PI6 yard attempt, same file as base) was not counted.
