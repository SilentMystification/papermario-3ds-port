# capture_NAT8: one O3DS prof capture of main for the per-core load (the standing even-load gate, plan_native.md section 3). Designer: oracle NATIVE7.
WAIT: `Merge NAT8A` (the c1t and c1n lines). No code change. Build main as push_runbook.md Order 1-2 says.
## Steps
1. Capture check first (CLAUDE.md rule 3): `python 3ds/tools/capture_check.py --row NAT8 --proof "core2 lgms,core2 wait,cls rec,c1t snap,c1n n" --profile prof warp=mac_01:0 pos=0:0:200 profdisk=1`. Push only after CAPTURE CHECK OK.
2. Push (push_runbook.md, perf line): `powershell -NoProfile -File 3ds/go.ps1 push o -ip <address the user names> -NoBuild -Seconds 90 -Profile prof -GameDebug "warp=mac_01:0 pos=0:0:200 profdisk=1"`.
3. User (about 2 min): boot into mac_01. Stand still 40 s with no input. Then walk through Toad Town for 30 s.
4. Lines: `grep -E "prof f[0-9]+ w[0-9]+ (fps|s[0-9]+ fts|pmu|cls|ndc|nd |at |core2 lgms|core2 wait|c1t|c1n)" build3ds/hw_live_log_<last octet>.txt > build3ds/out/nat8_lines.txt`. Backup with the game still open: `python 3ds/tools/pm_logrecv.py --ip <address> --label nat8 --out build3ds`.
Links: switch `prof=30` (bench_defs.json:6) and `profdisk=1` (pm_log.c:362); sink the live socket and log.txt (pm_log.c:1008); tool push_3ds.sh writes the hw_live_log file; proof the lines of step 4, one set per 300 drawn frames.
## How to read one window (same f<N>, compare windows of the same `at` scene only)
- C0 (core 0 busy, ms per drawn frame) = `wk` (fps line) minus `wwait` (core2 lgms line). FACT wk is the game thread time from step start to step end (pm_main.c:682, :758-760). wwait is its wait for the logic (pm_logic.c:151-154).
- C1 (core 1 busy, ms per logic step) = `lgms` minus `bw` (core2 wait line). FACT lgms is the wall time of a logic step (pm_logic.c:74-76) and bw, the logic wait for the walker, is inside it (pm_phase.c:655-656, nd_wq.c:92-104). lgms alone is NOT the core 1 load.
- Cycle check: core 0 `cyc / 268112` (pmu line, CCNT per clean frame, pm_phase.c:586). Core 1 `cyc1 / 268112` (core2 lgms line, CCNT of core 1 in the step interval, all threads of core 1 incl. audio and mixer, pm_logic.c:73-79). GUESS the CCNT stops in WFI, so these are busy cycles.
- Do NOT use wks, fts or oh for the load. FACT a sampled frame runs the logic step inline on core 0 (pm_main.c:533 lp_serial, pm_logic.c:135), so oh is not only prof overhead.
- cls, c1t: ms per sampled frame. c1n: window totals over n logic steps. True c1t part = value minus (pairs / n) x cal / 268112, with pairs: snap = snb, lsc = lsf + lse, alt = altu + altr.
- Frame time: ft (ms per drawn frame) and fps. Target 33.4 ms (30 fps floor). Core 0 idle per frame (GUESS GPU and present waits) = ft minus cyc / 268112.
## Gate (standing, user 2026-11-18)
R = C1 / C0 per window. R above 1.20 in 2 or more windows: the next wave moves the pure decisions to core 0 at their call sites (UD-N1 P2, NAT7 rows), no toggle. Else core 1 keeps them (P1). Report R and ft per window.
## Baseline (FACT build3ds/hw_o3ds_live.txt:236-250, NATM f1618 w300, build of 9cd355b1af, before NAT1B..NAT5A)
ft 42.00, wk 39.40, wwait 0.54: C0 38.86. lgms 40.41, bw 21.47: C1 18.94. R 0.49. cyc 6474503 = 24.15 ms, cyc1 6724350 = 25.08 ms. kb 85.
## Expected now (main 9576c03383 plus NAT5T, NAT8A). Every value GUESS from the code read, the measure in brackets.
| part | NATM FACT | now GUESS | reason (FACT code) |
|---|---|---|---|
| core 0 comb | 3.24 | 0.5-1.0 [cls comb] | no core 0 decision, skip state only (nd_comb.c:345-355); marked combine units not recorded (nd_rec.c:429-430) |
| core 0 rest | 4.74 | 3.5-4.5 [cls rest] | nd_scomp_run gone (NAT5C); alt ranges jumped (nd_rec.c:641); adds snap copies of NdSnapChk (nd_snap.c:138, :220) and sc_adopt (:173) |
| core 0 ent, rec | 3.69, 2.45 | 3.0-3.7, 1.8-2.4 [cls ent, rec] | entity gaps run slices of a flat copy (nd_rec.c:253-255); model units run only on refuse (nd_walk.c:563) |
| core 0 tex, list | 1.00, 0.26 | 0.6-1.0, 0.2 [cls tex, list] | NAT5T: TILESIZE window from core 1 (ws_NAT5T) |
| core 0 sub, raw | 4.19, 1.93 | same [cls sub, raw] | native submit, not touched |
| C0 | 38.86 | 35-38 | sum of the rows above |
| core 1 snap | not split | 1.0-2.0 [c1t snap, c1n snb snn snm] | snap_build per stream triangle, rect and range: 2 memsets, inputs, render state, 2 binds before the memo (nd_snap.c:43-53), about 1000 cycles; a new snap adds find, fix, colours; a miss adds nd_comb_core1 (:58) |
| core 1 list tables | not split | 0.3-1.0 [c1t lsc, c1n lsf lsb lse] | ls_find hashes NdSnapChk byte by byte (nd_snap.c:76) and scans up to 256 hashes (:77); nd_lsc_end_at copies NdDs per list (:91) |
| core 1 alt | not split | 0.2-0.6 [c1t alt, c1n altu altr] | one st_put per copied unit (nd_rec.c:450-452); nd_rec_alt records fallback units through nd_rec_nr (:442-444); NAT5T adds txr_take per TILESIZE |
| core 1 ep rule | not split | below 0.1 [none] | nd_ds_ep_rule per unit (nd_ds.c:405-411) |
| C1 | 18.94 | 21-23 | baseline plus the core 1 rows |
| R | 0.49 | about 0.6 | gate holds: no move to core 0 |
## Hand-back to the orchestrator: the path of nat8_lines.txt, then per window: at, ft, C0, C1, R, cyc and cyc1 in ms, c1t and c1n values.
