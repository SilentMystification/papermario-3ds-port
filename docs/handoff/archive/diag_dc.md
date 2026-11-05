# diag_dc: cost of a non-empty batch close (plan_draw_cost.md, rows DC1 and DC2)

Branch worktree-agent-ac8130878d743ca42 (merged with dlc-compiler-c1c2 at 6e9a17f82). Lane dc (build3ds/lane-dc). Azahar, direction only, not console.
Azahar starts: PM_ROW=dc1 5 of 8 (yard, pause, file select, intro fail at frame 210 (story text), intro preset run). PM_ROW=dc2 4 of 8 (yard, pause, file select, intro).

## DC1: sampled dcsplit counters (commit 5b01ec2a9)
One close in 8 with phase on is timed. Log line `dcsplit f<n> ...` under the phase switch. The us fields are sample sums x 8 per frame.
Mean over 103 frames, yard (preset=yard phase shots shotmode=4:80 battlef=80):

| field | per frame | per non-empty close (n 41.7) |
|---|---|---|
| n (non-empty closes) | 41.7 | - |
| raw | 25.5 | - |
| pal (uniforms, palette) us | 131.3 | 3.1 |
| bind (texture binds) us | 131.7 | 3.2 |
| tev us | 87.6 | 2.1 |
| st (fragment state) us | 83.6 | 2.0 |
| draw (draw call) us | 643.2 | 15.4 |
| kick (kick_check) us | 87.2 | 2.1 |
| palup | 6.9 | - |
| pmul | 3.7 | - |
| tevr (TEV rebuilds) | 12.9 | - |
| tb0 / tb1 / tb2 (real binds) | 42.0 / 0.0 / 0.0 | - |
| tbs (staged binds) | 1.0 | - |

Draws per frame (gpusplit): 81 at f102 (one frame). Mean over the 103 gpusplit lines: 53.5 (includes frames before the yard starts).
Reading: the draw call is the largest field in the sample, about 15 us per close of the 28 us sampled sum. This is a sampled, single-run number. The plan's 106 us per close is not reproduced in this sample: the sampled closes sum to about 28 us per close, so the rest of the 106 us is not in these fields (not checked further).

## DC2: redundant dirty marks (commit 91b225755)
Changes: TEV stage 0 fetch (C3D_GetTexEnv(0)) only in the TEV rebuild path. C3D_GetBufInfo only when the pool buffer info changes. Unit 1 bind shadow in pm_tex_bind_aux (cleared by pm_tex_bind_reset, by a dropped or loaded slot, by the snap bind, and by pm_tex_unbind_aux; staged slots never skip). Unit 2 bound once per sh_clear.

Yard, same switches, dcsplit mean over 107 frames: n 42.5, raw 26.0, pal 133.7, bind 130.2, tev 93.8, st 93.1, draw 598.0, kick 82.9, palup 7.0, pmul 3.7, tevr 13.1, tb0 42.8, tb1 0.0, tb2 0.0, tbs 0.9.
Draws per frame: 81 at f107 (same as DC1 at f102).
Reading: no change above run-to-run spread. bind 131.7 to 130.2 us, tb0 42.0 to 42.8 per frame. The draw field moved 643 to 598 us. Not proven as an effect. Unit 1 and unit 2 binds are 0 in the yard, so DC2 unit 1 and unit 2 gains are not visible in this scene.
Not recorded: fl tex n / empty / us for DC1 and DC2 (flsplit line), and dcsplit for the pause, file select and intro runs. The lane log was overwritten by the next run.

## Pictures (all four checks, DC1 and DC2)
1. Yard shot 40 (frame 89), preset=yard: Mario, ground, shadow and post office sign visible. Zoom 4x under Mario: dark shadow beside Mario on the grass. DC2 is pixel identical to DC1 (PIL diff, no difference).
2. Pause status (preset=toad padscript=wait80,START*3,wait100, shot 0005, frame 181): dialog "Check here to see what Mario's current status is." readable. Item list Hammer, Boots, Stats, Items readable. DC2 pixel identical to DC1.
3. File select (fast autoa, shot 0003, frame 153): panels, NEW, Delete File, Copy File, Cancel, Stereo, Mono, "Select file to start!" and file text readable. No garbled texture seen. DC2 pixel identical to DC1.
4. Intro Mario House sign (fast preset=intro shotwin=60:40, shot 0039, frame 99): sign reads MARIO (rotated crop). DC2 pixel identical to DC1. Note: shotwin=210:1 without preset gives the story text box, not the house.

Bottom-screen memory rows: not in the pictures. The shots hold the top screen only (240 x 400). The rows are generated: log line `hud mem rows ready: FCRAM free 11/96M commit used 84/96M | linear 7942/14208K heap 27792/51200K | VRAM used 2286K free 3858K tex 360K` (DC1 intro run). The on-screen rows were not seen.

## Not run
statehash, golden.py (not requested). DC3 to DC5 (plan says they follow DC1 evidence and console data; not started).
