# Row STEREO: 3D mode walks the frame once; the right eye replays the GPU entries

## User facts
User (2026-11-09): "Framerate is lower than what I want in 3d mode." Oracle asked to check if multi-eye rendering is performant.

## Start state
Worktree build3ds/wt/STEREO, branch STEREO from main (hash in the first commit). PM_ROW=STEREO, PM_LANE=STEREO. One Opus agent.

## Facts (oracle, main 6333cec32)
- pm_nusys.c:112 `gbi_walk(s); if (pm_gpu_stereo()) { pm_gpu_eye(1); gbi_walk(s); pm_gpu_eye(0); }` for every task, and the interp key
  frame (pm_nusys.c:182) too: the whole walk (stream replay, producers, texture binds, combiners, records, ring copies, matrices,
  uniforms) runs twice on core 0. Estimated +20 ms on the O3DS (mono walk 20.6 ms in mac_01); not measured (no stereo prof run).
- Eye shift on the CPU: nd_mtx_combine (nd_mtx.c:90-100) folds x' = x + e*(w - conv*|p23|) into combined (perspective only);
  callers upload_mvp (nd_mtx_set.c:41), nd_model_draw (nd_model_set.c:28), model-cache hit (pm_gpu.c:3144); culling uses the shifted
  matrix (nd_geom.c:328, nd_shape.c:407/453/511); gm_tab doubles. pm_gpu_eye (pm_gpu.c:1916-1927): flush, sh_clear, FrameDrawOn,
  upload_mvp. pm_gpu_begin_wait (2193-2200) reads the slider (PMDL("stereo") lookup every frame).
- targetR (pm_gpu.c:1462) RGBA8 + D24S8 = 750 KB VRAM held even in 2D.

## Plan
S0: prof field `eye2 <ms>` (ticks of the second gbi_walk, sampled frames) and bench spot_mac01_stereo_prof (console_prof,
    warp=mac_01:0 pos=0:0:200 prof=30 stereo); scenarios spot_mac01_stereo_det_l/_r (det + stereo, shoteye for the right eye) with
    benches. Commit; report the S0 hash so the user can run the console bench. Do not wait for it: continue.
S1: eye shift in the vertex shader: uniforms eye (x = e, per eye), D = persp*post_col0 and Dc = persp*cw*post_col0 (per batch, from
    gv_post, shadowed); pm4/pm5.v.pica: `mad r9, D, r5.wwww, -Dc` then `mad r5.x, eye.xxxx, r9, r5` (pm4 90 -> 93 of 96 registers,
    pm5 81 -> 84); ortho/rect D = Dc = 0. nd_mtx_combine loses e (returns the persp flag and cw); cull with the centre matrix and an x
    margin |e_max|*(w + cw); delete pm_eye_e from the CPU paths (nd_mtx_set.c:38-45, nd_model_set.c:16/28, pm_gpu.c:3144,
    pm_gbi_eye_changed). Update the nd_host test of nd_mtx_combine.
S2: record the GX queue entries of the left pass (LIST via GPUCMD_GetBuffer around C3D_FrameSplit, CLEAR, COPY, EYEFB) in a static
    table (about 48 x 12 B, static assert vs GRAB_MAX, off-screen slots, kick); a left prefix chunk forces all state (not recorded);
    the right pass in pm_gpu_end_w (pm_gpu.c:2223) before C3D_FrameEnd: split, clear targetR, right prefix (FrameDrawOn(targetR),
    eye = +e, all state dirty), replay LIST/COPY (target -> targetR)/CLEAR/EYEFB with pm_gx_room. Delete the second gbi_walk, pm_gpu_eye,
    r_cleared, per-task cur_target switching. Named difference: CPU read-backs (pm_gpu_read_frame, pm_gpu_depth_at) see the left eye
    only. Update plan_direct.md section 1 and 6 (eye 2 replays GPU entries, not the stream); close audit_system.md R10, agent_s10.md
    item 9, design_producer_models.md P5.
S3 (optional, own premise check: every drawn frame clears depth first): targetR shares target's depth buffer (375 KB VRAM back).
hw_guidelines; no toggles (S2 deletes the old path); no stubs; memcheck margin about 298 -> 294 KB (>= 256).

## Done criteria
All: nd_host PASS, memcheck OK. S1: 2D det byte-equal (spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold); two S1 runs of
spot_mac01_stereo_det_l/_r byte-equal to each other; both eyes look right; diff vs base only at edges; yard_stereo_l/r picture.
S2: both eyes byte-equal to the S1 build on spot_mac01_stereo_det_l/_r, yard_stereo_l/r, and stereo variants of
pause_spin_grab_det_w300, the nok11 splash, a pause off-screen pass, gameover_fps60; 2D det byte-equal. Console (user):
spot_mac01_stereo_prof vs spot_mac01_prof: stereo ft within about 1-2 ms of mono, eye2 about 0; watch gb (GPU limit).

## Rules
CLAUDE.md rules 5, 7, 8, 9, 11. No git stash, no push, no attribution. Never commit 3ds/src/map_restore_table.c. DIRECT (Opus) edits
pm_nusys.c and nd_* in parallel: merge main before each step.

## At the end
Delete this row file in the commit that completes the row. SendMessage main "STEREO ready <hash>".
