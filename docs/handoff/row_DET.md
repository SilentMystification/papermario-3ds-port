# Row DET: deterministic cold runs (test mode)

## Why
Two cold runs (turbo, -NoKeep, fixedseed fast fs1off, nok11_splash; logs build3ds/wt/fb1/build3ds/azt/logs/fb_nok11_pad1.txt, pad2.txt) diverge at game frame 100: run 2 has a second `rect f100` line (two GPU frames in one game frame). gv_palup then drifts 25-50 per 60 frames. Warm pairs are equal. This blocks every cold byte gate: FB-1 grab, FB-2 noise (fb2 vs fb2 7/30), battle_start_f182 (pre vs pre 2550 px).

## Oracle plan DET (2026-11-07, code trace)
Causes (options.ini fps60=1, interp on):
1. (top) Interp key draws depend on wall time: pm_main.c:413 draws a key frame (pm_interp_key_draw) in the idle iteration when pm_interp_on > 0 && !last_step_skipped; the 60 fps pacing (pm_main.c ~433, ~463) decides skips and draws from C3D_FrameCounter(0) - step_vb (vblanks). pm_nusys.c:113 pm_interp_skip switches key/lerp by timing. fast only skips sleeps; fs1off covers the FS1 skip only.
2. Per-drawn-frame state inherits (1): pm_gpu.c pm_gpu_begin noise_off = noise_next() per drawn frame (PT07); pm_tex_begin_frame ageing, eviction and the vram_up 2-frame upload delay; dump_shot at GPU frame begin windowed by pm_frame_count (first GPU frame of game frame 410 may be lerp or key).
3. Audio ticks per step from real retraces: pm_main.c pm_audio_step, n = pm_audio_lock() ? 2u : pm_audio_retraces(). Secondary.
4. Wall-clock log lines (t= ms, beat, stats) do not feed the game.
Measurement: debug switch `detlog`, one line per game step 0-420 in two cold runs: step, GPU frames begun in the step (key/lerp flags), vblank delta, audio ticks n, game RNG state, noise_next state, pm_tex evictions, pm_interp_skip. Diff; the first differing field names the input.
Fix: debug switch `det` (implied by fixedseed together with fast): exactly one key draw plus one lerp draw per step (or interp off), no pm_interp_skip, no vblank-based draw decision; audio ticks per step fixed at 2; noise_next per game step; pm_tex ageing and the vram upload delay counted in game steps; dump_shot takes the key frame of the step. No behaviour change outside det.
Gate: two cold runs of nok11_splash: every shot byte-equal, logs equal except bootid, timestamps and beat lines. Same for battle_start (all shots, f182 too). Add det to the scenarios used as cold byte gates (fb_nok11_*, battle_start_f182, nok11_fs) in the same commit.

## Rules
Own worktree from nds1 (build3ds/wt/det, branch det); copy assets/us and ver/us/build. Code first (CLAUDE.md rule 5). Debug switches only (no toggles of the main path). No attribution lines, never commit 3ds/src/map_restore_table.c, no git stash (shared across worktrees), keep Azahar open. Status lines in 3ds/logs/perf/status_DET.txt. Budget: -ResetBudget DET at the cap. Delete this file in the last commit.
