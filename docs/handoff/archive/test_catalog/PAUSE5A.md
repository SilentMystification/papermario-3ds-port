## PAUSE5A (2026-10-29): menu windows inside the open menu, gates at +15 %

- Build: default lane (build3ds\pm_3ds.3dsx, sha256 559728b3, git 9461b95a8 code). One run on lane pbase (HEAD 1514) was made by mistake and is not a gate value.
- Timeline (pause_menu, run 2 log): pause open at frame 96. Open spike in windows f61 to f175 (wall max 182.6 ms in f120 to f175). Steady pause from f176. Close START at frame 400 (scan 764), so the close is after the window.
- The padscript START presses: scan 146 (frame 91) and scan 764 (frame 400). Frame = 51 + (scan - 66) / 2.
- Until changed from `phase W f1[2-9]\d-` (windows that start in the spike) to `phase W f2[3-9]\d-`. The run stops at the first window that starts at frame 230 to 299. Cap 40 s (was 90).
- Window values (phase W, 60 frames, gates = larger of two runs +15 %, rounded up):

| Scenario | Run | Window | Walk cpu ms | Closes | Ops | Gate (walk, closes, ops) |
|---|---|---|---|---|---|---|
| pause_menu | 2 | f296-355 (until f296) | 9.3 | 96 | 1003 | 10.7, 111, 1154 |
| pause_menu | 3 | f236-295 (until f236) | 9.2 | 96 | 1003 | (same) |
| partner_menu | 1 | f295-354 (until f235-294) | 7.6 | 68 | 789 | 8.8, 79, 908 |
| partner_menu | 2 | f296-355 (until f236-295) | 7.6 | 68 | 789 | (same) |
| file_select | 1 | f119-178 (until) | 8.1 | 88 | 1346 | 9.5, 102, 1548 |
| file_select | 2 | f118-177 (until) | 8.2 | 88 | 1346 | (same) |

- Walk gate uses the last window in the log. For partner_menu the last window is one after the until window (both inside the menu). The scenario stops within 5 s of its until-line.
- Picture: pause_menu top shot at frame 130 (inside the open menu, the open spike period). A person must open 0000_1_1.ppm.
- Starts used on row PAUSE5A: 8 of 8 (1 pbase, 3 pause, 2 partner, 2 file). Budget reset is needed before more PAUSE5A starts.

