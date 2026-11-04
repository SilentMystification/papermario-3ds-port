# agent_vramcache: rows BUZ1, SLP2, ENC1, VRAM2, CACHE1 (branch native, worktree papermario-3ds-port-main)

## Commits
- 50626922f BUZ1: Buzzar pick-up calls break_free init (buzzar.c:944). Scan of src/ for init calls: no other mismatch.
- 5d48a2ef8 SLP2: long sleep. Wake and sleep state (pm_wd_asleep, pm_wd_slept, pm_wd_wake_frames), watchdog does not exit while asleep, crash line written before the fsyncs, audio and render joins bounded after a sleep (2 s), frame begin without SYNCDRAW for 120 frames after a wake (INFERRED effect).
- d9c7076d8 VRAM2: copy block and 4 full images reserved at boot (full_reserve). Cache = bank free space at boot. Boot: A 160 KB, B 1696 KB, cap 1856 KB. Removed shrink, regrow, bdrain, full_touch, IDLE_FREE frees.
- 2fd4b59bc CACHE1: eviction by cause (slot, cap, extent) in texstat; previous frame protected.
- ENC1: NOT committed. encwarp now calls set_battle_formation, set_battle_stage, load_battle. Still crashes (find_script_labels). Change reverted.

## Test state
- ENC1 (battle_start_fb): 2 of 8 starts. Run 1 (cap 20 s) stopped at frame 182, the 20 s cap was too short. Run 2 (cap 30 s): assert i < MAX_NPCS at log line 549, unmapped reads at find_script_labels (PC 0x00173CC0), game log silent after frame 182 (stall). Shots 0005 to 0007: overworld, no transition. FAILED gate. Needs a diagnosis agent (fresh) on the caller of the bad source pointer (diag_battle_start_crash.md section 6, fix 2).
- SLP2 (yard): 3 of 8 starts. Run 1 and 2 with a 500 ms and 2 s join bound on every step: 85 to 86 timeouts in one yard run. Bound moved to post-sleep only (pm_wd_slept). Run 3: 0 timeouts, boot and frames advance. Console sleep test (30 s and 120 s lid close) is for the user.
- VRAM2: 3 of 8 starts. yard (boot, memory line): PASS. pause_spin_grab (pause shot): picture shows pause status screen; no grab line in the pause. yard_vramfill: texstat f300 and f600 cap 1856 KB, vramfill block 1 failed, no shrink line: PASS.
  Not run: battle_start_fb star transition for VRAM2. Blocked by the ENC1 crash.
- CACHE1: 2 of 8 starts. mac_01: 85 slots (f300), 104 slots (f600), 510 KB, 0 evictions. kzn_19: 323 (f300), 459 (f600), 1099 KB, 0 evictions. 512 slot limit (pm_tex.c SLOTS) is 90 percent used on kzn_19 and rising. Not changed: a larger limit needs RAM for the Slot array (user decision).

## Open items
1. ENC1 crash: diagnosis agent. Gate: battle_start_fb star transition shot, no find_script_labels faults.
2. VRAM2 gate: battle_start_fb star transition shot, after ENC1 is fixed.
3. CACHE1: user decision on SLOTS (512 vs more). kzn_19 at 459 of 512.
4. SLP2: console test by the user. The C3D frame begin without SYNCDRAW after a wake may need a check for overlap (INFERRED).
5. pm_wd_slept: join bound stays off until the first sleep.
6. Scenarios added: pause_spin_grab, yard_vramfill, mac01_texstat, kzn19_texstat (test_catalog.md rows added).

## Commands
- Build: `python 3ds/tools/gen_map_restore.py`, then `PM_LANE=native PM_ROW=<row> sh 3ds/build.sh`, then restore 3ds/src/map_restore_table.c from a copy.
- Test: `PM_ROW=<row> powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 <scenario> -Row <row> -Build native -Try`
- Not committed and not touched: 3ds/src/map_restore_table.c, 3ds/logs/perf/pushed_builds.txt.
