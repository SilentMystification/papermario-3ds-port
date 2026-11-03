# agent_uc3d: handoff (row UC3, branch uc2c). Read this first. Replaces agent_uc3c.md for the state below.

## 1. State
- Branch uc2c. HEAD f3cc7d08b. Main merged into uc2c (fast-forward to 7b6e3735b, then UC3 commits).
- Commits: ed9165892 UC3a (pal-only mask). a0614a09e UC3c (index arena). 9b4c08cca UC3d (record tail). f3cc7d08b UC3b step two (partial).
- Gates PASS:
  - UC3a and UC3c: mac01t_rec_chk f307 bad 0, miss 0, ibad 0, hit 2872. Yard f348 hit 2003, bad 0, ibad 0.
  - UC3d: mac01t_rec_chk f307 bad 0, miss 0, ibad 0, build 139. Shots byte-equal.
  - UC3b2 (step two partial): mac01t_rec_chk f307 bad 0, miss 0, ibad 0, hit 10328, ref 177. Yard f348 bad 0, miss 0, ibad 0, hit 4089. Shots byte-equal (mac01t 0000_1_2 and 0001_1_2, yard 0000_0_11).
- UC3e (replay at G_DL, scenario yard_rp): NOT started.
- UC3b step two: the items below are NOT in code.

## 2. Open items (UC3b step two, then UC3e)
1. REC_DIRTY_CAP: ctx+0x20 |= mask after sh_clear in pm_gpu_rec_begin (pm_gpu.c), and OR the same mask into g[5] in pm_gpu_grp_get. The mask bits need a read-only objdump check first (plan_uc3.md 2c command; libcitro3d.a in image pm3ds-build:1). A wrong bit shows as bad > 0.
2. Refusals in flush_draw (pm_gpu.c, rec_cap block near line 920): second game matrix (pal_g[0] differs from the first draw, or pal_n != 1), slot change on unit 0 or 1 inside the window (pm_tex_cur_slots), pm_tex_slot_staged (new, pm_tex.c) for a bound slot.
3. Foreign registers: pm_gpu_rec_walk (pm_gpu.c) returns -1 for viewport, framebuffer and TEV update buffer writes. Numbers from libctru 3ds/gpu/registers.h.
4. ia_hold (plan 2a): set when a replay ran in this frame; pm_gpu_rec_idx_reset clears it at pm_gpu_begin. Needed only with UC3e.
5. UC3e: plan_uc3.md section 3. rec_rp, rec_try branch, mg_run G_DL branch, rec_play (noinline), pm_gpu_rec_play, pm_tex_rec_use. New scenario yard_rp in 3ds/tools/azt_scenarios.json and the catalog row in docs/handoff/test_catalog.md in one commit. Gate: mac01t_rec and yard_rp runs > 0, rec line wbad 0, top shots byte-equal; then mac01t_rec_chk once (bad 0).

## 3. Notes
- UC3d layout differs from plan 2c: geom_out is in RecKey (field gout), not in the arena. The patch table (4 B per entry) is in the arena after the body. RecKey is 32 B aligned, size about 96 B (not 64 B as the plan states). Decide before UC3e whether to shrink it.
- Azahar early exits (no game log, no new crash file) hit mac01t starts in UC3c. AZFLAKY owns the cause. Use build3ds/lane-UC3/last_log.txt for rec lines and shots. The shared sdmc log.txt was absent, so the azt marker check fails with "marker missing" even when the lane log has the marker.
- The yard_rec run in UC3b2 passed on the first start.
- Box skip off while a capture is open raised the rec line build from 139 to 522 (mac01t f307). That is the more complete capture.

## 4. Commands
- Build: `python 3ds/tools/gen_map_restore.py`, `docker rm -f pm3ds-build-UC3`, `PM_LANE=UC3 sh 3ds/build.sh`, then restore 3ds/src/map_restore_table.c from C:\Users\Andrew\AppData\Local\Temp\claude\uc3\map_restore_table.c.keep. Never commit the user file.
- Start: `PM_ROW=<row> PM_LANE=UC3 timeout 900 powershell -NoProfile -File 3ds/tools/azt.ps1 <scenario> -Row <row> -Build UC3 -Try`. Justification line in 3ds/logs/perf/status_<row>.txt before each start.
- Shots: build3ds/lane-UC3/shots. Base shots: C:\Users\Andrew\AppData\Local\Temp\claude\uc2c\base_mac01t_rec and base_yard_rec.

## 5. Acronyms
UC3 = replay row. UC3a = pal-only compare mask. UC3b = self-contained capture. UC3c = index arena. UC3d = record tail. UC3e = replay at G_DL. ibad = index words that differ. ia = index arena (linear, u16). rb_gi, rb_ge = gidx fill at window start and end. io, ic = arena index and count of a record span. npt = patch entries. nd = draws. gin, gout = geom at list entry and exit. fl = flags (bit 0 wind <= 0, bit 1 tri_since_vtx). alen = arena words used. rec_on = capture open. rec_dln = static lists in the capture. adr = address words. pal = palette uniform words. uni = other uniform words. oth = other differing words. hit = compares. bad = compared words that differ. miss = patch tables that differ. ref = refusals. stg = captures refused for a staged slot. VRAM = video RAM. FCRAM = main RAM. sdmc = Azahar virtual SD folder. AZFLAKY = investigation of the Azahar early exit.

## 6. State after the step-two round (agent_uc3d, second pass)
- UNCOMMITTED in the tree (3ds/src/pm_gpu.c): REC_DIRTY_CAP = 0xFF800010 (Effect, Tex, TexEnv only; wider superset sent the framebuffer group and was dropped), per-draw slot check (rec_sa, rec_sb), one game matrix check (rec_gm), pm_tex pending check per draw, foreign-register check in pm_gpu_rec_walk (viewport 0x41 to 0x44, 0x68; TEXENV update 0xE0; fog 0xE1 to 0xEF; framebuffer 0x112 to 0x117, 0x11C to 0x11E). Diagnostic log line `UC3b2 foreign reg` (log once per run): remove before the commit.
- Gate mac01t_rec_chk f307 (start 7): build 450, ref 155, wbad 46, hit 8658, bad 0, miss 0, ibad 0, shots byte-equal. wbad 46 = windows refused by 0xE0 (TEXENV update). The plan literal "wbad 0" is not met by design.
- Gate yard_rec f348: NOT RUN. Row UC3b2 is at its start budget (8 of 8). Reset with run_azahar.ps1 -ResetBudget UC3b2, then run yard_rec.
- Not measured: ia_hold, UC3e. Replay (UC3e) not started.
- Findings: 0x11C and 0x118 writes (depth buffer location; early depth test) occur inside windows. 0x118 is GPUREG_EARLYDEPTH_TEST2 and is not refused. Flush/invalidate 0x110, 0x111 are ordering commands and are not refused.

## 7. Committed: ba1cce07f (UC3b step two)
- Gates pass: mac01t_rec_chk f307 (bad 0, miss 0, ibad 0, hit 8658, wbad 46 by design, shots byte-equal); yard_rec f348 (bad 0, miss 0, ibad 0, hit 3826, shot byte-equal). Plan row UC3b gate text updated.
- REC_DIRTY_CAP is 0xFF800010 (Effect bit 4, Tex 23 to 25, TexEnv 26 to 31). Still open: AttrInfo, BufInfo, Scissor, Program bits (identify with the objdump tst list: C3Di_UpdateContext tests 0x4 to 0x20000, bits 0 and 1 excluded; the superset 0xFC3B47FC sent the framebuffer group and broke the walk).
- UC3e NOT started. Needed before it: ia_hold; rec_rp, rec_try branch, mg_run G_DL branch, rec_play, pm_gpu_rec_play (GPUCMD copy, pal patch from pcomp of the live matrix), pm_tex_rec_use; scenario yard_rp in azt_scenarios.json and the catalog row in one commit. The citro3d and GPU call forms (GPUCMD_AddRawCommands, C3D_FVUnif access) are not verified in this session.
