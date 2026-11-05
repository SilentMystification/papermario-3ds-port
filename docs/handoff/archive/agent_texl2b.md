# agent_texl2b: TEXL2 L2a gate, state at 2026-11-03

## Gate status
- L2a picture gate: NOT closed. The volcano top shot frame 200 differs from HEAD in 20 bytes (x166-168, y289-293).
- Yard: not re-run on the committed L2a (223717bce). Earlier top shot 0000_0_11 equal to HEAD (see agent_texl2.md).
- L2b to L2d: not started.

## Evidence (volcano, top shot)
- Parent 7d19c5808 in lane TEXL2BASE equals old HEAD baseline: 0 bytes.
- Layout control (unused 4 KB static, lane TEXL2PAD): 0 bytes against HEAD.
- Split: part A (load_miss source hash move, gh_check) is not the cause. Part B (counters) gives the 20 bytes.
- Single removals inside part B, each still 20 bytes against HEAD: VA (upload tick read, lane TEXL2VA), VC (ghost table writes, lane TEXL2VC).
- VB (texcap pm_debug_value call removed, lane TEXL2VB) is invalid: the run loaded kmr_20, not kzn_19. The call also parses the debug switches.
- Animation check (free, from shots): HEAD frame 200 vs frame 204 differs in 2638 bytes (x146-189, y236-305). 13 of 15 spot pixels change.
- VC frame 204 equals HEAD frame 204: 0 bytes. VC frame 200 differs from HEAD frame 200 in 20 bytes only.
- Read: the difference sits on an animated spot and only at frame 200. Not proven as a timing effect. A frame 199 or 201 shot from HEAD and from VC is needed.

## Next steps
1. Add scenario volcano_nb (shotwin=199:3 on lane TEXL2BK or VC, and HEAD parent lane TEXL2BASE) to azt_scenarios.json with a catalog row in the same commit. Compare VC frame 200 with HEAD frame 199 and 201.
2. If VC frame 200 equals HEAD frame 199 or 201 at the spot: document the animation phase as the explanation. Gate rule: counters plus top shots equal on yard, volcano difference explained.
3. Run yard on the committed L2a binary (lane TEXL2, built from 223717bce) against HEAD yard shots.
4. Then L2b (plan section 4).

## Budget
- TEXL2 after the coordinator reset: 3 starts used (VA, VB, VC). Cap 8.

## Commands
- Build: from the parent worktree (..\papermario-3ds-port-texlbase), `PM_LANE=<lane> sh 3ds/build.sh`. Then copy the 3dsx into the texl2 lane folder.
- Test: `PM_ROW=TEXL2 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 volcano -Row TEXL2 -Build <lane>`.
- Read shots from build3ds/lane-<lane>/shots/ (a failed marker check does not update build3ds/azt/shots).
- Diff: scratchpad ppmdiff.py (PPM header is 15 bytes, width 240, height 400).

## Gate closed (2026-11-03, later)
- Merge main (7b6e3735b) into texl2: conflict in test_catalog.md, both sections kept (commit 35503bd3a).
- Shots taken every 4 frames only (pm_gpu.c shotwin hook). Frame 201 cannot be shot. Used 196, 200, 204 (scenario volcano_nb).
- Volcano: texl2 merged (lane TEXL2M) equals main (lane TEXL2MB) at frames 196, 200, 204: 0 bytes each.
- Yard top shot 0000_0_11: 0 bytes. Yard bottom shots differ (rows 64 to 230, 23 KB): the bottom screen shows the log and build string, not gated. Yard log has no texghost line.
- The earlier 20 bytes were against the old base (7d19c5808 HUD and older main). Not reproduced against current main. Cause not fully known.
- L2a gate: CLOSED on current main.
- Budget: 7 of 8 starts used (VA, VB, VC, MB, M, yard MB, yard M). One start left.
- L2b to L2d: NOT started. Context rotation point reached. Next agent starts at plan_tex_l2.md section 4, L2b.
