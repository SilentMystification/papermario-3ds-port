# Performance run on the O3DS (checklist for the user)

1. Plug in the O3DS (192.168.1.168), same Wi-Fi as the PC. Put `dspfirm.cdc` in `sdmc:/3ds/` and `papermario.z64` in `sdmc:/3ds/PaperMario/`. Delete old `debug3ds.txt` files.
2. Start Homebrew Launcher (netloader) on the console. Azahar must be closed on the PC.
3. On the PC run this one command from the repo folder: `powershell -NoProfile -File 3ds/tools/perf_capture.ps1`
4. The script runs 8 runs: A boot, B title, C toad, D yard, F battle, E1 phase toad, E2 phase yard, G phase toad battle (grabs, splits, VRAM, swirl, F03 query).
5. Before each run the script asks you to press Enter. First open the Homebrew Launcher on the console and press Y. Do not press other keys during a run.
6. After each run press HOME on the console and close the game. After the runs E1, E2 and G the script asks you to start the FTP app on the console, then press Enter.
7. Time: about 15 minutes plus the prompts. Each run is 60 to 90 seconds.
8. The script writes `3ds/logs/perf/baseline_<build>.txt` and the full logs next to it (`<build>_A.txt` and so on).
9. Send back `3ds/logs/perf/baseline_<build>.txt` and tell the orchestrator "baseline done". Keep the folder 3ds/logs/perf, an agent reads the full logs.
10. If one run fails: run only it with `-Only C` (example). Use `-NoPrompt` to skip the Enter prompts.
