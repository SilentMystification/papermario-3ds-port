# ws_FSKIP (Haiku): 60 FPS option with 3 modes: Off, On, Dynamic
Designer to ask: the oracle (SendMessage to oracle). Branch from main. Worktree build3ds/wt/FSKIP, branch FSKIP.
PM_ROW=FSKIP PM_LANE=FSKIP on every bench call. No counter (C1..C12) moves.
Cause, FACT (code read and console log build3ds/hw_tidal1_live.txt, build 5852, O3DS XL):
- With 60 FPS on, pm_main.c:721 sets LGF_ISKIP (no interpolated frame) when interp_heavy() is 1.
- interp_heavy (pm_main.c:514-516) goes heavy at walk > 26000 us or GPU > 13000 us. It leaves heavy only after 30 steps with
  walk < 20000 us and GPU < 10000 us. While heavy, the walk is the plain walk.
- The log lines #176758, #179122, #180735 give `steps 300 heavy 300`, plain_us 37290, 22440, 22501, gpu_us 8982, 7364, 7182.
  22440 us is over 20000 us, so heavy never ends: every step skips the interpolated frame: 30 FPS with no interpolation.
- The Frame skip row (fskip, pm_main.c:569 and 690) does not change this. Off: no draw is dropped, ISKIP stays.
Design (block A, pm_fpsmode.h): options.ini key fps60 = 0 Off, 1 On, 2 Dynamic. Old files with fps60=1 get On.
- Off: no change (interpolation off). On: no ISKIP from load or lateness (the game slows when a step is slow).
- Dynamic: heavy at walk > 30000 us or GPU > 15000 us (at once). Calm after 30 steps in a row with walk < 27000 us and GPU < 12000 us.
  ISKIP when heavy or when the step is late (iv > 2, as now). The values 30000, 27000, 15000, 12000 are GUESS (the user may change them).
- The fps60 launch switch with fps60=0 acts as Dynamic. Det runs never take this path (pm_det_on first, no change).
- Menu row text stays "60 FPS". Values: "Off", "On", "Dynamic". A and left or right cycle Off, On, Dynamic.
Files: 3ds/include/pm_fpsmode.h (new), 3ds/src/pm_main.c, 3ds/src/pm_options.c, 3ds/tools/pm_fpsmode_test.c (new),
3ds/tests/host.d/fskip.sh (new). Do not edit any other file.
## Edit shape (example, pm_options.c:730)
Before: `        case RW_FPS: return pm_options_get(0) ? "On" : "Off";`
After:  `        case RW_FPS: return pm_fps_name(pm_opt.fps60);`
## Edits: blocks A to E of docs/handoff/ws_FSKIP_code.md, exact text.
## Loop (1 build, at most 2 Azahar starts)
1. Do blocks A to E. Run `sh 3ds/tools/nd_host_test.sh > build3ds/out/fskip_host.txt 2>&1`.
   Pass: `ND HOST TESTS PASS` and `fskip ok 12/12`. Commit `FSKIP 1: 60 FPS modes Off, On, Dynamic (pm_fpsmode.h)`.
2. Run `sh 3ds/build.sh > build3ds/out/fskip_build.txt 2>&1`. Pass: rc 0 and memcheck margin above 0.
3. Gate (Off, the default fps60=0): `python 3ds/tools/bench.py base spot_mac01_drec --commit $(git merge-base HEAD main)`,
   then `python 3ds/tools/bench.py run spot_mac01_drec --row FSKIP --why "FSKIP gate, Off byte-equal"`,
   then `python 3ds/tools/bench.py cmp spot_mac01_drec --row FSKIP --commit $(git merge-base HEAD main)`. Pass: 1/1 byte-equal.
## Fix scope
A failing host test or seed: compare your files with the blocks byte for byte (LF line ends). Do not change an expected text or a seed.
A compile error: check the two include lines (B1, C1). Do not edit src/.
## Stop and hand-back
Stop: host PASS, build rc 0, gate 1/1 byte-equal. Final commit `FSKIP 2: gate` with the values. It deletes ws_FSKIP.md and ws_FSKIP_code.md.
Hand-back: build3ds/out/handback_FSKIP.txt (line 1 commit hash, then host line, build number, memcheck margin, gate value).
Any other failure after 2 tries: write the output to build3ds/out/ and send the path to the oracle (SendMessage). Then stop.
User check on the O3DS: 60 FPS On: the HUD shows more than 30 FPS in the world. Dynamic: 60 in light scenes, 30 under load.
