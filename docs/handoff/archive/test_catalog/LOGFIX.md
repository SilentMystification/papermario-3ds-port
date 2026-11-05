## LOGFIX (2026-10-29): cost scenarios for the phase and dcsum sets

| Scenario | Flags (after `loglevel=debug`) | Command | Result (build 1467, git 06e62c0aa) | Status |
|---|---|---|---|---|
| yard_pace | `pacestat logstat` | `yard_pace -Try` | logstat work 11.66 ms per frame (300 frames). Picture equal to yard baseline. | unverified (one run, frame-time base) |
| yard_dcsum | `pacestat logstat dcsum` | `yard_dcsum -Try` | work 11.79 ms (+0.13 ms, +1.1 %). dcsum line printed at f299. Picture equal. | unverified (one run) |
| yard_phase | `phase phsum audiostat texstat pacestat logstat` | `yard_phase -Try` | work 12.74 ms (+1.08 ms, +9.3 %). Picture equal. | unverified (one run) |

- Picture rule: top shots equal to the FINAL4 baseline (build 6e965c84b; the only later src change is LIVELOG log text). yard, yard_pace, yard_dcsum, yard_phase, mac01 (2 of 2): byte-equal. pause_menu: run 1 differed, run 2 byte-equal to baseline. Run 1 and run 2 of one binary differ, so the difference is run noise.
- The yard run without flags prints no frame-time line. Its frame time is taken from yard_pace.
- pause_menu run 1: walk gate failed (12.9 against 12.8). Run 2 passed (11.5). Gate value varies between runs.

