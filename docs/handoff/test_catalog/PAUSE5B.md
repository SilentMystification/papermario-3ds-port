## PAUSE5B (2026-10-29): cold timing of the pause open (fsplit, phase)

- Code: `pm_pc_begin` (state_pause.c, BEGIN_PAUSED), timers in state_pause.c (rom, pause_init, setup block) and background_gfx.c (filter), window and lines in 3ds/src/pm_phase.c. All under TARGET_3DS and the fsplit switch. No change to the picture: pause_pcopen top shot byte-equal to pause_menu top shot (PAUSE5A run 3), shot_cmp 1 of 1.
- Lines: `pcopen` (one per open: 60 drawn frames from BEGIN_PAUSED), `pcphase` (worst frame and window sums per phase), `pcgbi` (worst frame GBI sub-phases, phasegbi only). Log lines are kept under 256 bytes, so the summary is three lines.
- Build 1519 (git a02d5ac09 plus PAUSE5B), default lane.

| Scenario | Run | pcopen avg / max ms per frame | Window ms | Tex loads (KB) | Evict | Map chg | Readback ms (calls) | Filter ms | Setup ms (rom / pause_init / rest) |
|---|---|---|---|---|---|---|---|---|---|
| pause_pcopen | 1 (before the phase split) | 36.04 / 173.42 (f129) | 2162 | 96 (1385) | 0 | 0 | 8.77 (1) | 6.01 | 9.25 (0.00 / 7.30 / 1.94) |
| pause_pcopen | 2 | 35.96 / 168.61 (f128) | 2157 | 95 (1385) | 0 | 0 | 8.77 (1) | 6.01 | 9.25 (0.00 / 7.30 / 1.94) |
| pause_pcopen_gbi | 1 (phasegbi) | 36.10 / 175.31 (f128) | 2166 | 95 (1385) | 0 | 0 | 8.77 (1) | 6.00 | 9.25 (0.00 / 7.35 / 1.90) |

- Worst frame (f128, run 2 and 3): gbi 141 to 148 ms, idle 23 to 24 ms, tex 0 loads, fbread 0, gpu 0.9 ms. The GBI sub-phases of that frame (gvtx, gtri, gmtx, gop) are 0.0 ms; gtev, gbind, gst total 0.8 ms. So the GBI spike is not in any timed sub-phase.
- Window sums over 60 frames (run 2): tex 81.6 ms (95 loads, about 0.86 ms per load), gbi 538.5 ms, gpu 51.1 ms, dl 44.7 ms, other 22.1 ms, fbread 8.8 ms.
- Derived: pause steady gbi about 6.6 ms per frame ((538.5 - 147.7) / 59).
- Field, same map (`field_mac00`, Toad Town, window f241-300, no pause): closes 16 per frame (draw 16, flush 16), gbiops 465, gbi 3.2 ms avg, logic 2.3 ms, cpu 13.2 ms. Pause steady (PAUSE5A, f176-295): closes 96, gbiops 1003, cpu 9.3 ms.
- Not measured: top draw groups in the pause (frozen background, labels, icons, windows). The build has no per-group census. A census needs a new counter (diagnosis agent).
- Azahar numbers are direction only. Console: not measured.
- Starts used on row PAUSE5B: 4 of 8 (pause_pcopen 2, pause_pcopen_gbi 1, field_mac00 1).

| Scenario | What it proves | Command | First marker | Status |
|---|---|---|---|---|
| pause_pcopen | pcopen line per pause open (fsplit phase) | `pause_pcopen -Try -Row PAUSE5B` | `warp: mac_00 entry 4` | unverified (2 runs) |
| pause_pcopen_gbi | pcgbi line (phasegbi) for the worst frame | `pause_pcopen_gbi -Try -Row PAUSE5B` | `warp: mac_00 entry 4` | unverified (1 run) |
| field_mac00 | field window on mac_00 (no pause), gameplay comparison | `field_mac00 -Try -Row PAUSE5B` | `warp: mac_00 entry 4` | unverified (1 run) |
