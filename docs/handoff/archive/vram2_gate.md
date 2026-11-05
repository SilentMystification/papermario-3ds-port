# VRAM2 gate (row VRAMG, branch native, commit d9c7076d8 layout)

Build: existing native lane binary (build3ds/lane-native/pm_3ds.3dsx, 22:41). No source edits. No rebuild.

## Result

| Gate | Result | Basis |
|---|---|---|
| 1. battle_start_fb star transition complete, no garbled strips | FAIL | Shots 0001 to 0014 show an overworld with Mario. A dark wedge grows at the lower right from shot 0005 (frame 174) to shot 0014 (frame 210). No star shape is visible. No garbled strips are visible. The transition is not complete at the window end. |
| 2. battle_start_fbvram: same transition, cap constant, no alloc fail for off-screen or copy block | PASS (with a missing control) | Shots equal to gate 1 (gate 3). Cap stays at 1856 KB (bank B). No `vram alloc fail` line and no `V05` line. The forced failure did not occur, so the expected event `vram fail` did not match. |
| 3. Shot sets compared pixel by pixel | PASS | 15 of 15 shot pairs (0000 to 0014) byte equal in pixels: battle_start_fb run 2 against battle_start_fbvram run 3. |

## Starts (row VRAMG, 3 of 8 used)

1. battle_start_fb, run 1: log checks 4 of 4 PASS (pre-battle start at frame 174, shot 5 frame 174, window end at frame 219). Shots lost. The SLOT1 row started a run while run 1 moved its shots. `run_azahar.ps1` releases the Azahar lock before `Move-Item "$sd\shots"` (line 324), so a second run can remove the shots folder first.
2. battle_start_fb, run 2: shots 0000 to 0014 saved. Run stopped at `encwarp: window end` (log line 2353, frame 219). Run ended with rc 2 (crash symbols) after the until-line. The fault after the window is not part of this gate.
3. battle_start_fbvram, run 3: stopped at `encwarp: window end` after 17 s. Shots 0000 to 0014 saved. `vramfill: block 1 of 2 failed` (log line 39).

## Cap line evidence (run 3, log build3ds/lane-native/last_log.txt)

- Line 37: `texcache bank A: 160KB at 0x1f2d3800, cap 160KB`
- Line 38: `texcache bank B: 1696KB at 0x1f453800, cap 1856KB`
- Line 127: `hud mem rows ready: ... VRAM 6108/6144K tex 0/1856K`
- Lines 21 to 31: boot VRAM reserves (copy buffer in bank A, full images 0 to 2 in bank A, full image 3 in bank B). No shrink line.
- No `vram alloc fail` line. No `V05` line.

## Cap change (azt_scenarios.json)

- battle_start_fb and battle_start_fbvram: cap 45 s to 24 s.
- Basis: run_azahar stopped the run at the until-line after 17 s (integer, truncated) in runs 1 and 3. Frame 225 is 6 frames after frame 219. The exact time to frame 225 was not logged. Estimate: under 18.5 s. Plus 5 s, rounded up: 24 s.
- The change is committed through the index only. The working copy keeps other uncommitted work (SLOT1 scenarios) that is not committed here.

## Open items

- Gate 1 needs a decision: the transition is not complete by frame 210 in the 15-shot window. A longer window (frames 210 to 225 at least) is needed for a complete picture. This is a user decision (scenario change).
- run_azahar.ps1: move `$sd\shots` before `Release-Lock`, or hold the lock until the move ends. Otherwise two rows lose shots.
- The forced VRAM failure (vramfill=2) does not fail the off-screen alloc in the VRAM2 layout. A control with a larger fill is needed to prove the failure path.
