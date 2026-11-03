# agent_btlstar: battle start star transition (BTLSTAR)

## State
- Branch btlstar, worktree C:/Users/Andrew/Documents/GitHub/papermario-3ds-port-btlstar, base main 7d19c5808.
- Cause NOT named. Fix NOT made. Garble NOT confirmed by picture. Result: INCOMPLETE.
- Azahar starts used: 7 of 8 (runs 1 to 7 on the BTLSTAR row; runs 3 and 5 failed at the marker, runs 6 failed at the marker too, see Notes).
- Debug scenario added: battle_start_enc (3ds/tools/azt_scenarios.json, catalog row in docs/handoff/test_catalog.md, status unverified).

## Debug switch encwarp=<scene frame> (3ds/src/pm_warp.c, encwarp_tick)
- Enters ENCOUNTER_STATE_PRE_BATTLE for the first world enemy of the map with the fields START_BATTLE sets (firstStrikeType, scriptedBattle, fadeOutAmount, substateDelay, state, EncounterStateChanged, gEncounterSubState). The real update_encounters_pre_battle and OVERLAY_START_BATTLE then run.
- Logs: `encwarp: pre-battle entered by encwarp`, `encwarp: pre-battle start at frame N`, `encwarp: window end at frame N`. Diag lines `encwarp diag at N` every 5 frames for 40 frames after the switch (npc pos and flags, enemy flags, suspend time, cooldown, hitType, encounter state, action state).
- The touch test (update_encounters_neutral) did not start the battle in run 4: the enemy npc stayed at y -1000 (sentinel; encounter.c treats y < -990 as not in the world), encounter state 2 (NEUTRAL). Cause of the sentinel not found.
- Shot window: pm_gpu_shot_window(from, n) in 3ds/src/pm_gpu.c (debug shot code only, no draw code changed). Shots at 4-frame steps in the window (10 files).

## Measured (run 7, lane BTLSTAR, build from this commit)
- Log checks 4 of 4 PASS. Pre-battle entered at scene frame 120, pre-battle start at frame 174, window end at frame 219.
- CPU readback: one line `D03: last frame copied to a frame buffer in 8731 us` (cfb_capture_prev CPU branch, pm_main.c) at frame about 170. Only one CPU readback line in the run.
- Frame phases in the transition (Azahar, not console): gbi 140 to 177 ms per frame in frames 161 to 169 (avg 223 ms wall); fbread avg 0.2 ms, max 8.7 ms; audio 55 to 272 ms per frame (Azahar audio, not the transition); texcall 65, texmiss 0 to 1 per frame in frames 161 to 169.
- Frames 116 to 160 (before the window): wall avg 48.3 ms, gbi avg 19.8 ms.
- Picture (4 shots, frames 174, 186, 198, 210): field with Mario. A black wedge grows at the lower right. No star hole read. No garble read at this scale.
- The pm_b7 cap_n and cap_ticks counters were not read from the log (not printed in the phsum lines of this run).

## Open items
1. Star not confirmed on the picture. Check the front overlay path: the forced entry sets the pre-battle state, but the overlay may need demoState DEMO_STATE_CHANGE_MAP (back layer) or the front layer params (src/encounter.c:1538). Read the shot at full size before any fix.
2. Garble not confirmed. Compare the copy frame in the shot with the field frame at the same frame index.
3. cap_n and cap_ticks: read them from the phase lines (pc_end, pm_phase.c:600-612 prints them in pcopen lines only when the pause capture runs). Add a phase print for the transition or read the pm_b7 counters from the debug build.
4. Pause spin (user report): not added. A pause scenario exists (pause_menu, phase on, shotwin=130:1). A spin scenario needs the START press frame and a shot window of 40 frames. Not run (budget).
5. Merlon grab (coordinator request): the per-frame grab log (pm_gpu.c:1707, grab id, source, frame, stale reuse yes or no) is not added. Merlon scenario not added.
6. Fix point (not made): the strip copy in src/screen_render_util.c:30-77 and the grab in pm_gpu.c:1702-1724. The measured cost in the transition is gbi and audio, not the readback (fbread avg 0.2 ms), so the readback is not the main cost in this run.

## Next steps
1. Open shot 0005 (frame 174) and shot 0014 (frame 210) at full size in build3ds/lane-BTLSTAR/shots or build3ds/azt/shots/battle_start_enc. Name the star shape or its absence.
2. Set the overlay layer to match the real encounter (back layer when demoState is DEMO_STATE_CHANGE_MAP, otherwise front) and rerun with one start.
3. Read pm_b7 cap_n and cap_ticks per frame (add one phase line in the transition window; log-only).

## Commands
- Build: `PM_LANE=BTLSTAR sh 3ds/build.sh` (needs assets, ver/us/build and ver/current as real copies in the worktree; junctions fail in the docker copy).
- Run: `PM_ROW=BTLSTAR powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 battle_start_enc -Row BTLSTAR -Build BTLSTAR -Try`
- Log: build3ds/lane-BTLSTAR/last_log.txt. Shots: build3ds/azt/shots/battle_start_enc/.

## Notes
- Azahar runs share one sdmc folder (C:/Users/Andrew/AppData/Roaming/Azahar/sdmc/3ds/PaperMario). Another agent's start replaced the 3dsx and debug3ds.txt during run 5, and log.txt was not written. Runs 3, 5 and 6 failed at the marker for this reason or for a closed Azahar. Use one start at a time across agents.
- Shot copy: the shared folder rule in the orchestrator note applies. Stay under 45 s per start.
