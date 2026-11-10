# Row STEREOFX: crashes with the 3D slider on (Watt in a dark room, Merlee talk), O3DS XL

(Model: Haiku. Diagnostic plus repro: 3 Azahar starts.)

## User facts (2026-11-10, XL .131, build 8085ef1d0, which has STEREO S2 99014ae5e)
1. Shy Guy's Toy Box (omo), a dark room, Watt is the partner. The user moves the 3D slider up from 0. The game crashes at the slider move.
2. Talk to Merlee with 3D on. The game crashes right before the price screen. No dump (FTP down). The XL build has no crash page.

## Trace (oracle, code read, main fe241dc42, 3ds/src/pm_gpu.c)
Slider read per frame at frame begin (:2434): stereo_frame = targetR && u_eye >= 0 && slider > 0.02; gfxSet3D; sr_frame_begin (:1993).
Mono to stereo: the first stereo frame mallocs sr_ent (256 x gxCmdEntry_s) and sr_fb (:1996), on the walker thread.
FAULT PATH 1: if a malloc fails, sr_on = 1 and sr_over = 1. Then sr_take (:1981, `sr_ent[sr_n++] = ...`) and pm_gx_drain (:2145) write through
NULL. If only sr_fb fails, sr_ent is kept, the retry is skipped, and bind_main (:2034) writes sr_fb[]. A failure at the first stereo frame matches crash 1.
The replay (:2059) re-queues the left lists; fills and copies of target move to targetR (sr_map). Overflow (SR_MAX 256, SR_FB_MAX 32) falls back to a copy.
Case 2 (3D already on) is not explained by path 1: that is the reason for the repro run.

## Goal
Show the fault on Azahar with the user's timing: the slider moves at run time, then the Merlee talk with 3D on. Add one log line that prints the replay state.

## Start state
`sh 3ds/tools/wt_new.sh STEREOFX` (branch STEREOFX from main, which has CRASHALL: a crash shows the crash page and logs PC/LR). PM_ROW=PM_LANE=STEREOFX.

## Touch only
3ds/src/pm_gpu.c (pm_gpu begin at :2435, sr_frame_begin, sr_replay), azt_scenarios.json, bench_defs.json.

## Exact edits (pm_gpu.c)
1. Debug tool, after `if (PMDL("stereo")) eye_slider = 1.f;` (:2435):
   { static int st_at = -2; if (st_at == -2) { char v[12]; st_at = pm_debug_value("stereoat", v, sizeof v) ? atoi(v) : -1; }
     if (st_at >= 0) eye_slider = pm_frame_count() >= (unsigned)st_at ? 1.f : 0.f; } /* STEREOFX debug: slider 0 -> 1 at frame st_at */
2. In sr_frame_begin, first line: `static int sr_prev; if (stereo_frame && !sr_prev) sr_logn = 0; sr_prev = stereo_frame;`
   with `static unsigned sr_logn = 4u;` next to sr_n (:1963).
3. At the end of sr_replay (before the pm_ph_on line), the one log line:
   if (sr_logn < 4u) { sr_logn++; pm_log("stereo: replay f%u n %u fbn %u over %d ent %p fb %p", pm_frame_count(), sr_n, sr_fbn, sr_over, (void*)sr_ent, (void*)sr_fb); }

## Scenario and bench (same commit)
merlee_stereoat: the merlee_spell debug string plus " stereoat=500" (in dro_02 after the live warp at 120, before the talk; the darken is at
about f601), until "merlee: undarken done", cap 45 (as merlee_spell), first_marker and expected_events as merlee_spell, plus event "stereo: replay f5".
Bench merlee_stereoat_pic (azahar_pic).

## Done criteria (REPRO GATE, at most 2 starts)
1. Build OK, memcheck OK. Run: `python 3ds/tools/bench.py run merlee_stereoat_pic --row STEREOFX --lane STEREOFX --why "STEREOFX slider switch + Merlee 3D"`.
2. Fault shown (crash page or no until-line): report the PC/LR and the last 4 "stereo: replay" lines. Hand back FAULT SHOWN. The oracle names the fix.
3. No fault: report the 4 replay lines (expected n < 256, fbn < 32, over 0, ent and fb not 0). Commit. Hand back NOT REPRODUCED with
   the build hash, the debug string and the push recipe for the XL (same string; the user moves the real slider in the omo dark room). Note fault path 1.
Affected scenarios: merlee_stereoat only.

## Stop
3 Azahar starts. No fix in this row. STOP RULE: 90 min or 150 tool calls.

## Rules
CLAUDE.md rules 1-7. PM_LANE=STEREOFX. No git stash. Foreground waits.

## Commands
- Build: PM_ROW=STEREOFX PM_LANE=STEREOFX sh 3ds/build.sh
- Budget reset: powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget STEREOFX

## State (2026-11-10)
Diagnostics merged (8f4e2997a, branch 81b14fa59). Azahar: NOT REPRODUCED. merlee_stereoat (slider 0 to 1 at f500, Merlee spell to undarken f992):
`stereo: replay f500..f503 n 5 fbn 1 over 0`, ent and fb not null. Fault path 1 not hit. Case 2 (3D already on) not reached. NEEDS CONSOLE:
XL push of main, the user moves the real slider in the omo dark room and talks to Merlee with 3D on; the crash page (CRASHALL) and the
`stereo: replay` lines are the evidence. Next: the oracle reads the console log.

## At the end
Keep this file (the fix row follows). Hand back "STEREOFX <FAULT SHOWN|NOT REPRODUCED> <hash>".
