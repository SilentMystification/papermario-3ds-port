# frame_budget: 16.7 ms per frame, split into stage targets (O3DS, 60 fps)

Goal (user, 2026-11-04): absolute target 60 fps on the Old 3DS, 30 fps floor. Frame budget 16.7 ms. The budget is the pass criterion for every
later slice. The console baseline (build 2020, commit 42f6fac7e) is the starting point of the measured numbers. Basis of the targets: inv_frame_cost.md
(build 1588, phase on) and plan_native_system.md section 7 (expected end state, phase off). Phase-on numbers are about 0.74 to 0.79 times the play
numbers (inv_frame_cost.md section 3). Targets are phase-off ms per frame unless marked.

Rule: a slice is judged by the stage it changes. The prof budget line (pm_phase.c, `prof f<n> w<n> budget`) prints each target next to the measured value.
A stage over its target blocks the slice that owns it. Slack is the frame budget minus the CPU targets.

## 1. Stage targets

| Stage | Target ms (phase off) | Prof field (measured) | Owner slice | Basis |
|---|---|---|---|---|
| Game logic (step, AI, physics) | 3.3 | not split (fsplit `logic`) | none (game code) | plan 7 end state: logic 3.3; inv_frame_cost S1 3.2 phase on |
| Game list build (producers) | 4.4 | not split (fsplit `dl`) | S3 to S9 (producers ported) | plan 7 end state: list build 4.4; inv_frame_cost S1 4.4 phase on |
| Native emit and GPU submit (nd_emit, citro3d calls) | 2.0 | nd (emit ms per sampled frame) | S1 to S3 | plan 7 end state: native emit 1 to 2 |
| Legacy walker (gbi minus close) | 0.0 | lg (ms per sampled frame) | S10 deletes it | inv_frame_cost S1 walker 39.8 phase on; target 0 at S10 |
| Frame end (flush, swap, rest) | 1.5 | not split (fsplit flush, swap, rest) | S10, S11 | plan 7 end state: swap and rest 1.5; inv_frame_cost 0.3 + 0.8 + 0.6 phase on |
| Audio | 0.5 | not split (fsplit audio) | none | inv_frame_cost audio 0.0 measured; 0.5 headroom |
| CPU total (sum of the five stages above) | 11.7 | ft (frame time, sampled) | | |
| Slack (16.7 minus CPU total) | 5.0 | | | |
| GPU busy (per frame, runs in parallel with the CPU) | 14.0 | gb (submit to sync, sampled) | S11 (pipeline depth) | must stay under 16.7; no measured value below 16.7 yet (inv_frame_cost section 2) |

Notes:
- The GPU busy target is not added to the CPU total. The GPU and the CPU run in parallel. gb measures submit to sync, so it includes CPU time before the sync. It is an upper bound of GPU busy until S11 separates them.
- Targets are not measured. The S0-6 prof line gives the measured nd, lg, gb values. The other stages need the fsplit lines of a console run (phase on, a slice end).
- A target that is wrong after measurement is changed here first, with the new basis, then in the prof budget line.

## 2. Frame time check

| Case | CPU ms | Slack ms | 60 fps |
|---|---|---|---|
| Now (build 1588, phase on, Toad Town S1) | 55.0 wall | negative | no, 24 to 28 fps with logging off (project_console-fps-logging-off) |
| Plan end state (plan 7, phase off) | 10 to 11 | 5 to 6 | yes, if the GPU busy stays under 16.7 |

The end state depends on S1 to S10 and on the GPU busy time. S0-6 measures the GPU busy time. S0-7 (console baseline) gives the starting values.

## 3. Console baseline (starting column, build 2020, commit 42f6fac7e)

Source: build3ds/hw_live_log_131.txt (O3DS XL, pushed by the orchestrator, `prof` on, loglevel 1). 11 prof windows of 300 frames. The scene is not known from this log (no warp line in the file), so the numbers are not tied to a scene. The ranges are the min and max over all 11 windows.

Note: the coordinator message quoted fps 24.3 to 26.2, ft 38 to 41 ms, gbi 27 to 31.5, dlc 5.8 to 6.8, gpu 3.4 to 4.0 and gb 8.9 to 12. The log gives a wider range (fps 24.3 to 30.1, ft 33.4 to 40.9). The 24.3 to 26.2 windows are inside it. The log values are used here. The quoted gbi, dlc and gpu values are matched by the log only in part (gbi 2.2 to 31.5, dlc 0.0 to 6.8, gpu 0.7 to 4.0).

| Measure (prof field) | Baseline range, 11 windows | Stage it matches | Target (section 1) |
|---|---|---|---|
| fps (window) | 24.3 to 30.1 | frame rate | 60 |
| ft (frame time, ms) | 33.4 to 40.9 | CPU total (phase off basis, prof is phase on) | 11.7 CPU, 16.7 frame |
| gbi (ms per sampled frame, walker plus close) | 2.2 to 31.5 | legacy walker (lg 2.2 to 31.5) | 0.0 at S10 |
| dlc (ms, learn and replay list) | 0.0 to 6.8 | none (old replay, deleted in S1-1) | not in the budget |
| gpu (ms, texture and GPU stage) | 0.7 to 4.0 | none direct | not in the budget |
| nd (native emit) | 0.00 | native emit | 2.0 |
| gb (GPU busy, submit to sync, ms per sampled frame) | 8.9 to 29.4 | GPU busy (upper bound) | 14.0 |

Reading: the legacy walker is the largest CPU stage in the windows with the walk running. The GPU busy upper bound is above 14.0 in several windows, so the GPU target is not met yet. The native emit is 0 because no native code exists yet. Logic and list build are not in the prof line; the fsplit lines of a console run give them.
