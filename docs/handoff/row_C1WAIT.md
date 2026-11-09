# Row C1WAIT: where the logic step on core 1 spends its wall time

## Goal
O3DS XL kmr_20: core2 lgms 12.7 ms wall per logic step, cyc1 1.84M (about 6.9 ms). The oracle traced the gap to waits, not logging:
the log path is lock-free (pm_log.c:421-473, drop on full); the step clock (pm_logic.c:70-72) includes wq_block waits (nd_wq.c:77-83):
c_walked at nd_wq.c:124 (fr_push, pm_nusys.c:131, depth 1: step N+1 waits until frame N is walked), c_space :95, c_empty :136
(pm_walk_drain: pm_os.c:433 nuPiReadRom, pm_map.c:374/577/601, background_gfx.c:271), c_done :148. Audio joins also wait.
The PMU counts the whole core (no virtual counters), so cyc1 includes audio and system threads. Add counters that prove the split.

## Start state
Worktree build3ds/wt/C1WAIT, branch C1WAIT from main 98c707022. Copy assets/us and ver/us/build into it.

## Touch only
3ds/src/nd_wq.c, 3ds/src/pm_audio.c, 3ds/src/pm_au2.c, 3ds/src/pm_phase.c. Read only: pm_logic.c 64-76.

## Plan (prof only: every read is gated by pm_pf_on; use svcGetSystemTick, not pm_ticks, which bumps pm_svc_n)
1. nd_wq.c: `extern int pm_pf_on; static unsigned wq_bw[2];`. In wq_block: t0 before wq_blocked is set when pf; after the loop add d to
   wq_bw[0], and to wq_bw[1] when cond == c_walked. Add `void pm_wq_pf(unsigned out[2])`.
2. pm_audio.c: globals pm_au_pf_lgj, pm_au_pf_job. pm_audio_join_at: lj = pm_pf_on && pm_wq_on_logic(); read t0 when tk|pf|lj; both early
   returns include lj; if (lj) pm_au_pf_lgj += dt. pm_audio_kick (about 195): au_j_t0 also when pm_pf_on. Job end (about 143):
   if (pm_pf_on) pm_au_pf_job += dt.
3. pm_au2.c render_main (about 166): t0 when st|pf; after the job, if (pf) pm_au2_pf_busy += dt. Keep the stats() path.
4. pm_phase.c: lg0_x[5] filled in pf_lg_mark (about 440); append to the core2 line (about 568)
   ` bw %u.%02u fw %u.%02u aj %u.%02u au %u.%02u mx %u.%02u` (ms per logic step dn; one division per window). Fix the comment at 560:
   lgms is wall time and includes bw.
5. bench.py show spot_mac01_prof: if a metric list parses core2 fields, add the new ones.

## Done criteria
1. nd_host PASS.
2. One Azahar prof run that prints the core2 line (spot_mac01_pic with prof, as PMU2 used): new fields print, bw and fw non-zero.
3. memcheck OK (expect about +40 B).
The console run (user) decides: case A bw >= 70% of (lgms - cyc1/268k): the logic is not starved, only a faster walk or depth 2 helps.
Affected scenarios (no others): spot_mac01_pic.

## Rules
CLAUDE.md rules 5, 7, 9, 11. PM_ROW=C1WAIT PM_LANE=C1WAIT on every build.sh and bench call. No git stash. Commit after each passed step.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=C1WAIT PM_LANE=C1WAIT sh 3ds/build.sh; restore 3ds/src/map_restore_table.c.
- Test: PM_ROW=C1WAIT PM_LANE=C1WAIT python 3ds/tools/bench.py run <bench> --row C1WAIT --lane C1WAIT --why "<text>"

## At the end
Delete this row file in the commit that completes the row. SendMessage main "C1WAIT ready <hash>".
