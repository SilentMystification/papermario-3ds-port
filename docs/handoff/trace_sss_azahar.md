# trace_sss_azahar: Shooting Star Summit in Azahar (SSSA)

Date: 2026-11-04. Worktree: papermario-3ds-port-main, branch native, HEAD 4ff107938 at start.
Build: lane native, build3ds/lane-native/pm_3ds.3dsx (00:41). This build predates commit 6aeb0c117 (TEXF1, 00:47).
Status file: 3ds/logs/perf/status_SSSA.txt (3 starts, budget 3 of 8 used). No source edited.

## Starts

| Start | Map and entry | Switches | Shots | Until hit |
|---|---|---|---|---|
| 1 | hos_00 entry 0 | shots shotwin=60:250 loglevel=debug | 63 shots, frames 60 to 308, every 4 frames | frame 300 shot logged, run ended at the cap |
| 2 | hos_01 entry 0 | same | 63 shots, frames 60 to 308, every 4 frames | no (cap) |
| 3 | hos_01 entry 0 | shots shotwin=0:60 loglevel=debug | 43 shots, frames 0 to 57 (frames 0 to 37 every frame, then every 4) | no: until `shot \d+ frame 56` never matched, cap ended the run |

Shot files: build3ds/lane-native/shots (runs 1 and 2 share the folder, the names differ by map: 0000_5_0 = hos_00, 0000_5_1 = hos_01). Run 3 files: build3ds/lane-native/shots_run3 (0000_0_0 to 0014_0_0 = boot map 0:0, 0015_5_1 onward = hos_01).
Shot n in runs 1 and 2 is frame 60 + 4n. Frame 100 = shot 0010, frame 200 = shot 0035, frame 300 = shot 0060.

## What the pictures show

- Frames 0 to 37 (run 3): a flat blue screen with one RGB triangle (boot test picture). The map is not on screen. The map appears at frame 41 to 42.
- hos_01 frame 42 (0042_5_1): the Shooting Star Summit night sky, blue wall on the left, Mario and Goombario, stars. No garbled surface seen.
- hos_01 frames 60, 200, 300 (0000, 0035, 0060 of run 2, shown at 2x): blue wall with a faint diamond pattern, orange-brown edge. No stretched, striped or wrong-colour surface seen on the wall.
- hos_01 frame 300: white diagonal streaks at the top right and right edge (approx. x 150 to 240, y 40 to 110 in the 240x400 shot). The shape is straight parallel lines with a dark outline. They look like star-trail effect lines, not texture data. Not confirmed.
- hos_01 frame 200: white horizontal streaks at the bottom edge (approx. x 60 to 240, y 330 to 350). Same kind of line. Not confirmed.
- hos_00 frame 100 (0010): white and green curved streaks over the purple wall, approx. x 77 to 107, y 40 to 100. Frame 300 (0060): short white stripes at approx. x 76 to 86, y 52 to 65. Both appear on the top of the purple wall. Frames 60, 152 and 200 on hos_00 show no streak at that spot.

## Result

No garbled, stretched or wrong-colour surface was seen in any of the 106 shots at the frames read.
The only candidate is the white streak set on hos_00 (frames 100 and 300) and the streaks on hos_01 (frames 200 and 300). They are not confirmed as a glitch.
The glitch the user reported did not reproduce in these three starts. The load frames 0 to 37 show only the boot picture.

## Not done

- Step 3 (draw chunk and texture for the region): not run. No glitch region was confirmed, so no chunk was chosen.
- Step 4 (commit 124e1d468 comparison): not run. No worktree or build was made.

## Next step

1. Find the frame and camera where the user sees the glitch in Azahar. Ask the user for the frame count or the camera view, or the timing of the load.
2. If the streak set on hos_00 frame 100 is the glitch, run one drawlog=100:1 start on hos_00 and read the chunk whose box covers x 77 to 107, y 40 to 100.
3. Run the 124e1d468 comparison only after step 2 names a chunk.
