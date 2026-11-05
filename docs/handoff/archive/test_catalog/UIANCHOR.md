## UIANCHOR (2026-11-03): wide UI anchors, scenarios

| Scenario | Status | Command | First marker | Runtime | What it proves | Known limits |
|---|---|---|---|---|---|---|
| battle_target_wide | unverified (gate pending, UA1) | `battle_target_wide -Row UIANCHOR -Build UIANCHOR -Try` | `warp: kmr_03 entry 0` | cap 45 s | target pointer log and shot burst at target select (wide) | Wheel input does not reach target select (starts 2 to 4); see docs/handoff/agent_uianchor.md |
| talk_wide | unverified (discovery) | `talk_wide -Row UIANCHOR -Build UIANCHOR -Try` | `warp: mac_00 entry 4` | cap 30 s | NPC x y z (simstep lines, wide) | Log only. Mario does not walk by padscript (start 6). |
| battle_wide | UA4 gate: pass by eye (wheel beside Mario, wide), log `debug wheel open: base 33 183` | `battle_wide -Row UA4 -Build UIANCHOR -Try` | `warp: kmr_03 entry 0` | cap 45 s | battle wheel base follows the zoomed view (pm_wide_vx / vy) | Wide-off check: battle_start (verified) shows the wheel at the old place by eye; byte-equal to HEAD not run (wide-off code path returns the input). |
