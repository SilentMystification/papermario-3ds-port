## SPL1 (2026-11-03): splash grab, one per frame (lane native)

Result: no code change. HANG1a (commits e895b8b2f, 20fb1e88b) already makes one full-height grab per splash frame: pm_gbi.c grab_cur, splash branch, with splash_grab_f per frame. The row's proposed change in water_splash.c is the same path.

| Scenario | What it proves | Command | First marker | Runtime | Shot paths | Known limits |
|---|---|---|---|---|---|---|
| nok11_splash (SPL1 run 1, unverified) | Grab log: one `grab 0 ... rows 0-240` line per splash frame (frames 403, 407, 409, 410 ...). Shot a (file 0013, frame 412, Mario at -758 0 -49, spawn -758 25 -49) shows cyan splash drops by eye. Frame time from the log timestamps: frame 403 at 13896 ms, frame 412 at 14228 ms, frame 432 at 14959 ms, about 27 frames per second (Azahar, direction only). | `powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 nok11_splash -Row SPL1 -Build native -Try` | `warp: nok_11 entry 0` | about 16 s | build3ds\azt\shots\nok11_splash | No before-image: the build without the splash marks stops with svcBreak at the first splash frame, so no byte compare was made. |

