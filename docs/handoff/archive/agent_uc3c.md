# agent_uc3c: handoff (row UC3, branch uc2c). Read this first.

## 1. State
- Branch uc2c. Commits: ed9165892 (UC3a pal-only compare mask, pm_gpu.c), a0614a09e (UC3c index arena, pm_gpu.c and pm_gbi.c).
- UC3a gate PASS: mac01t_rec_chk f307 bad 0, miss 0, hit 2872, cls adr 0 pal 0 uni 0. Top shots byte-equal to base.
- UC3c gate PASS: mac01t_rec_chk f307 bad 0, miss 0, hit 2872, ibad 0. yard_rec f348 hit 2003, bad 0, miss 0, ibad 0. Top shots byte-equal (mac01t 0000_1_2, 0001_1_2; yard 0000_0_11).
- UC3d gate PASS: commit 9b4c08cca. mac01t_rec_chk f307 bad 0, miss 0, hit 2872, ibad 0, build 139 (as UC3c), top shots byte-equal. Records still not drawn.
- UC3b step two is NOT in the code (only step one, b728404af). Missing per plan_uc3.md section 4 row UC3b: REC_DIRTY_CAP at rec_begin and in pm_gpu_grp_get; refuse ring draws in pm_gpu_gtrip and pm_gpu_gappend; no box skip while rec_on in pm_dlc.inc.c (line 481 test); refuse a second game matrix (pal_g differs); refuse a slot change on unit 0 or 1 inside the window; pm_tex_slot_staged; foreign-register refusal in pm_gpu_rec_walk. Replay (UC3e) is not correct without these.
- UC3e (replay) NOT started, blocked on UC3b step two. Plan: plan_uc3.md section 3 and section 4, row UC3e (new scenario yard_rp). ia_hold (plan 2a) is also missing; add it with UC3e.
- 3ds/src/map_restore_table.c is the user file: never commit, never discard. Saved copy: C:\Users\Andrew\AppData\Local\Temp\claude\uc3\map_restore_table.c.keep.

## 2. Code changed
- pm_gpu.c pm_gpu_rec_same: mask = palette words (class 2) and 0x227 p0 only. Address words (0x85, 0x95, 0x9D) are compared (class 1).
- pm_gpu.c pm_gpu_rec_begin sets rb_gi = gidx_n. pm_gpu_rec_end sets rb_ge = gidx_n.
- pm_gpu.c pm_gpu_rec_idx(w, n, &io, &ic): copies gidx + rb_gi (cnt = rb_ge - rb_gi) to the arena ia (u16, 65536 entries, linearAlloc at first capture), flushes the span, rewrites 0x227 p0 of the record copy by d = phys(arena) - phys(gidx + rb_gi). Refuses when nd != window draws. pm_gpu_rec_idx_reset() sets ia_n = 0 (called from rec_reset).
- pm_gpu.c pm_gpu_rec_ichk(io, ic): counts index words of the span that differ from gidx + rb_gi (live window).
- pm_gbi.c RecKey gains io, ic. rec_done calls pm_gpu_rec_idx after the memcpy into ra; refusal increments rec_ref and keeps state 2. rec_check adds rec_ibad. The rec line ends with `ibad <n>`.
- ia_hold (plan 2a) is NOT in the code. It is needed for UC3e (a replay reads the arena while a reset can run).

## 3. Open items
- Azahar early exits (no game log, no new crash file) on 2 of 3 mac01t starts in row UC3c. AZFLAKY owns the cause. The rec line is read from build3ds/lane-UC3/last_log.txt.
- The azt scenario marker check failed in both UC3c runs ("marker missing") although the lane log has the marker lines. The shared sdmc log.txt was absent. Read the lane log for the rec lines and shots.
- oth class counts now include the rewritten 0x227 p0 words (they differ from the live words by design). They are masked, so bad is not affected.

## 4. Next steps
1. UC3b step two (plan_uc3.md section 4, row UC3b). Gate: mac01t_rec_chk and yard_rec: bad 0, miss 0, wbad 0, hit > 0, build > 0, top shots byte-equal.
2. UC3e: plan_uc3.md section 3. Add ia_hold. New scenario yard_rp in 3ds/tools/azt_scenarios.json and the catalog row in docs/handoff/test_catalog.md in one commit.
3. Revert rule: a gate fails, `git checkout -- <row files>` before the next stage.
4. UC3d record layout differs from plan 2c in one point: geom_out sits in RecKey (gout), not in the arena. The patch table is in the arena after the body.

## 5. Commands
- Build: `python 3ds/tools/gen_map_restore.py`, `docker rm -f pm3ds-build-UC3`, `PM_LANE=UC3 sh 3ds/build.sh`, then restore 3ds/src/map_restore_table.c from C:\Users\Andrew\AppData\Local\Temp\claude\uc3\map_restore_table.c.keep.
- Start: `PM_ROW=<row> PM_LANE=UC3 timeout 900 powershell -NoProfile -File 3ds/tools/azt.ps1 <scenario> -Row <row> -Build UC3 -Try`. The UC3a and UC3c starts used rows UC3 and UC3c.
- Justification line: append to 3ds/logs/perf/status_<row>.txt before each start.

## 6. Acronyms
UC3 = replay row. UC3a = pal-only compare mask. UC3c = index arena. UC3d = record tail. UC3e = replay at G_DL. ibad = index words that differ. ia = index arena (linear, u16). rb_gi, rb_ge = gidx fill at window start and end. io, ic = arena index and count of a record span. adr = texture address words. pal = palette uniform words. uni = other uniform words. oth = other differing words. hit = compares. bad = compared words that differ. miss = patch tables that differ. VRAM = video RAM. FCRAM = main RAM. sdmc = Azahar virtual SD folder. AZFLAKY = investigation of the Azahar early exit.
