# plan_60fps.md: the way to 60 fps on the O3DS (oracle NATIVE17; edit in place)
Tags: FACT (console capture or code read), GUESS (estimate), OPEN (not measured). Detail and file:line: plan_perf.md sections 6, 6b, 11.

## 1. The target as a budget
- Paper Mario runs its game logic at 30 steps per second. 60 fps on the O3DS means 30 steps per second plus one interpolated key frame per step (fps60 option).
- So each step has 33.3 ms. Core 1 (game logic and recording) must finish a step in 33.3 ms. Core 0 must draw 2 frames (the step frame and the key frame)
  in 33.3 ms. The GPU must finish 2 frames in 33.3 ms. Presents wait for the vblank (16.7 ms), so a step that takes 34 ms costs 50 ms: keep a margin.
- Working budget (GUESS, about 15 percent margin for scene changes): core 1 at most 28 ms per step, core 0 at most 28 ms per step (2 frames).
- Now (FACT, build 7680, O3DS .170, mac_01 0 0 200, prof): ft 42.9 ms per step (23.3 steps per second), fps 45.8, core 1 busy 35.2 (+ wait bw 6.25),
  core 0 30.7 (wwait 2.0), GPU 113 draws and 6574 words per frame, GPU time OPEN. Old display list build 3542: ft 54.8, fps 27.6.
- Gap: core 1 -7.2 ms, core 0 -2.7 ms to the working budget (GUESS budget, FACT now). 30 fps without key frames needs 33.3 ms per step: core 1 -1.9 ms.

## 2. Hot areas (ms per step on the O3DS)
| area | core | ms now | share of core | cause (one line) | minimum (GUESS) | row |
|---|---|---|---|---|---|---|
| game logic (step_game_loop) | 1 | 2.40 FACT | 7% | scripts, AI, physics | 2.4 (floor) | none |
| game draw code (render queue, appendGfx, sprite animation) | 1 | about 9.7 GUESS | 28% | the game's own draw work, as in the old build | 9.7 without DRAWOBJ, 4 to 6 with it | PROF3 measures it |
| untimed recorder work and cache misses | 1 | about 8.7 GUESS | 25% | a call chain per unit between game code: I misses 73.6K against 33.7K (FACT) | 1 to 2 | PROF3, then D2 batching or DRAWOBJ |
| snap builds (172 per step) | 1 | 4.78 FACT | 14% | a full 630 to 760 B snapshot per triangle group, hashed and copied | 1 to 1.5 | MSNAP2 (-2.2), C1 delta snaps (-1 to -2) |
| model records (about 93 per frame) | 1 | 4.33 FACT | 12% | unchanged models are recorded again every step | 0.5 to 1 | C3 model record memo (-2 to -3) |
| per-unit record (693 units) | 1 | 3.65 FACT | 10% | decode of each unit into the core 1 state (the old interpreter switch) | 0.5 | D1, D2 |
| alt copies (477 units) | 1 | 1.85 FACT | 5% | entity copies written every step, core 0 runs only the gaps | 0.5 | C4 (-1) |
| submit (113 draws) | 0 | 5.97 FACT | 19% | 53 us per draw: shadow compare, template words, buffer setup | 0.2 to 0.5 | K2 pre-baked state blocks (-4) |
| stream walk (rest, raw) | 0 | 7.68 FACT | 25% | core 0 walks the recorded frame again, one runner per record | 1 to 2 | D1 (DRAWOBJ) |
| model hit (key, matrix, hit) | 0 | 5.82 FACT | 19% | per model: texture lookup, template touch, matrix, palette, flush | 1 | K1 MDLHIT2 (-1.5 to -2.5) |
| entity, sprite, list, record classes | 0 | 3.85 FACT | 13% | entity pools, sprite builds, list tables | 2 | later |
| GPU | GPU | OPEN | - | 6574 words, 113 draws per frame (FACT); fill rate OPEN | OPEN | capture |

## 3. Order of attack (GUESS gains; core 1 / core 0 busy ms per step after each row)
| step | row | core 1 | core 0 | expected result |
|---|---|---|---|---|
| 0 | now (FACT) | 35.2 | 30.7 | 23 steps per second, fps 45.8 |
| 1 | PROF3 (timers) | 35.2 | 30.7 | the 8.7 ms untimed and the 9.7 ms game draw code split by subsystem |
| 2 | MSNAP2 | 33.0 | 30.7 | - |
| 3 | C3 model record memo | 30.5 | 30.7 | 30 steps per second when both cores fit 33.3 ms in each step (GUESS, no margin) |
| 4 | K1 MDLHIT2 | 30.5 | 28.7 | - |
| 5 | C1 delta snaps | 29.0 | 28.7 | - |
| 6 | K2 pre-baked state blocks | 29.0 | 24.7 | - |
| 7 | C4 entity copy gaps only | 28.0 | 24.7 | 60 fps (30 steps plus key frames) in mac_01 at the edge of the budget (GUESS) |
| 8 | rows from the PROF3 split (D2 batching) | 24 to 25 | 24.7 | 60 fps with a margin in mac_01 (GUESS) |
| 9 | DRAWOBJ rows DO1 to DO4 (user yes 20 Nov, plan_drawobj.md) | 23 to 25 | 20 to 22 | 60 fps with margin in mac_01; heavy scenes OPEN (GUESS) |
| 10 | DO5 and more game-side draw work (user decision) | 15 to 20 | 20 | margin for heavy scenes (GUESS) |

## 4. What rows cannot solve
- Game logic floor: 2.4 ms (FACT). The game draw code (about 9.7 ms, GUESS) stays unless DRAWOBJ replaces the per-frame draw calls of static objects.
- The O3DS gives the app at most 80 percent of core 1 (FACT hw_guidelines.md); audio threads also run there (user decision 20 Nov: audio stays on core 1).
  A step that fits on Azahar can miss on the console: only console captures decide.
- Immediate mode: every step the game issues every draw again, and core 0 walks the frame again (D1). Rows 2 to 8 make that cheaper, not gone.
  DRAWOBJ (retained native draw objects per model, entity and sprite; core 1 writes only changed matrices and colours) is the design change for that.
- The PICA: GPU time per frame is OPEN. If the GPU needs more than 16.7 ms per frame (fill rate of alpha layers, 400 x 240 targets), no CPU row helps:
  then the content must change (fewer layers or effects, lower resolution in hard scenes).
- Heavy scenes (battles, many NPCs, particle storms) cost more than mac_01 (GUESS). The plan reaches 60 fps in mac_01 first; other scenes need their own capture.
- User decisions: Q2 (captures), Q6 (DRAWOBJ design pass), content cuts if the GPU is the limit, and that 60 fps means 30 game steps plus key frames.
- Honest summary: rows 2 to 8 can reach 60 fps in mac_01 with a small margin (GUESS). DRAWOBJ rows add margin (core 1 -3.5 to -5, core 0 -4 to -7 ms, GUESS).
  Heavy scenes may also need game-side draw changes (DO5) or content cuts: the captures decide.

## 5. Measurement plan
- Capture 1, after PROF3 and MSNAP2 merge: words `--profile prof phase=on warp=mac_01:0 pos=0:0:200 profdisk=1` (phase=on is needed for c1t, c1d, c1g),
  17 steady windows; capture_check first with the same words.
  Read: ft, fps, lgms, bw, c1s, c1g (new), c1t, c1d, c1n, ic1, dc1, cls, c0r, gpu d and w, the GPU time line if the build has one (OPEN).
- Capture 2, after C3, K1 and C1: the same line set. Check the expected numbers of section 3 and change the order if a guess was wrong.
- Capture 3, a heavy scene (a battle with many actors, user's choice of spot) once mac_01 reaches 30 steps per second.
- Every row keeps byte-exact Azahar gates; Azahar numbers never change the plan (memory rule); console captures do.
