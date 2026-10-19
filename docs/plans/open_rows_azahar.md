# Open rows: can Azahar work them now? (2026-10-19)
Sources: 3ds/SLICES.md, docs/ORCH_STATE.md section 10.

- B7: O3DS measure run (phase times, VRAM, F03 query time). Azahar now: NO. Needs O3DS hardware. Rule: after S28.
- F09: language select, future feature. Azahar now: NO. Scope is decided after the hardware run. User decision 2026-10-18: not before optimization.
- F10: endless loop in tst_12, nice to have, last. Azahar now: YES for the cause. NO for the final fix: patch game data or add a port rule needs the user.
  First step: run `warp=tst_12:0 -Turbo -Smoke -FaultStop` once (PM_ROW=F10), read the loop address in the log, and trace the script that does not end.
- V07: item entities flicker when the camera moves. Azahar now: NO. SHELVED by the user 2026-10-18. The user reports it again if seen.
- V09: intro tints (hos_05 shots 70 to 77), nok_01 unmapped access. Azahar now: NO. SHELVED by the user 2026-10-18. Take it up only if the user notices it. V17 reactivates part of it and is a separate row.
- PO2: transform each vertex once at G_VTX. Azahar now: NO. Rule: optimization starts after S28.
- PO3: GPU vertex transform. Azahar now: NO. Rule: after S28. The plan may be written earlier (opus, plan only).
- AU1: audio on core 1 or ndsp. Azahar now: NO. Rule: after S28. The check is audio cost per tick on the O3DS.
- MA1: ARM11 math audit with the replay check. Azahar now: NO. Rule: after S28.
- F11-5: rebinding. Azahar now: NO. Rule: after S28. It also needs the user (D5).
- F11-6: Select menu at any time on the bottom screen. Azahar now: NO. Rule: after S28. It also needs the user (D2).

Result: only F10 can be started in Azahar now, and only to find the cause.
