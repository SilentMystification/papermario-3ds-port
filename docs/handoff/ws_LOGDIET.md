# ws_LOGDIET (Sonnet): the console log carries only lines the console runs need
Designer: orchestrator. Worktree build3ds/wt/LOGDIET, branch LOGDIET. PM_ROW=LOGDIET PM_LANE=LOGDIET. Start: main d3641712cc or later.
Why Sonnet: the row must find each line's readers (bench scripts, until-lines, expected-diff markers) before it changes the line. A wrong cut breaks a gate.
Objective: the live log ring (32 KB sent on connect, about 2.4 min) holds prof windows and xfer lines, not per-frame gate markers.
FACT (build3ds/out/live_xfer3_170.txt, one console run of build d3641712cc, 10361 lines, 290717 B):
 - `gstex f<N>` 6539 lines, 119782 B (nd_set.c:106 nd_gs_mark, one line per frame that runs a TEXTURE or fog op in a static call).
 - `mmiss f<N>` 2384 lines, 45073 B (nd_recs.c:56 mmiss_note, one line per frame with a model memo miss).
 - these two are 57% of the bytes. They are expected-diff gate markers (marker words gstex, mmiss; bench.py expect_check, bench.py:2434-2516).
 - next: prof:stepx 100 lines 8.6 KB (4 per window), snapi 86 lines 7.2 KB (nd_snap.c:32), xfer5/xfer6 180 lines 11 KB (keep: transition data),
   dlcpre 51 lines (nd_shape.c:378), gtabrun/ntabrun, t1fix (nd_recs.c:61), c0src/epchk/c0fb/clipc/altw/drec lines (pm_gpu.c:688-699, probably drec-only).
## Steps
1. Census, no edit. For every `pm_log_evt(` and `pm_log_prof_line(` call that prints more than once per run, list: file:line, word, rate (per frame, per window, once), readers.
   Readers = grep of the word in 3ds/tools (bench.py, azt_scenarios.json, bench_defs.json, capture_check.py, *.ps1, *.sh, *.py) and docs/handoff/*.md.
   Write the table to build3ds/out/logdiet_census.txt. Read the last 20 lines of build output only.
2. Per-frame and per-event gate markers (gstex, mmiss, t1fix, texfog if it exists, and any other word of the table that has a bench reader and a rate above once per window):
   print only when the process runs a bench or drec (use the existing flag the bench launch line sets: read pm_drec_now(), pm_debug_has() in pm_log.c and the launch words
   in azt_scenarios.json; pick the flag every marker reader already sets, do not invent a new switch the console never sets). Console prof pushes (livelevel=err, no drec) print none.
   The marker words must still print on every bench that reads them: run `python 3ds/tools/bench.py selftest`-style checks the repo has (bench.py:2450 selftest) and one expected-diff
   compare that needs a marker (name the bench from the census; rerun cmp --expect on it: must PASS with the same draw and frame as before).
3. Lines with no reader and a rate above once per window (candidates: snapi, dlcpre, gtabrun, ntabrun, c0src, epchk, c0fb, clipc, altw): delete the line, or move it behind pm_debug_has("<word>"),
   unless the census shows a reader. Keep every prof window line that a plan page names (docs/handoff/plan_*.md) and every xfer line.
4. Counter: lines per 300-step window on the console (Azahar prof run: `python 3ds/tools/bench.py run <bench> --row LOGDIET --why "log lines"`, count lines by word before and after).
   Target: gstex and mmiss 0 lines on the console launch, total lines per window down by at least 60%.
## Gate
- Build, host tests, c0gate (0/0/0). Byte compare of the benches the census names as marker readers: byte-equal (a log line change must not change shots or drec output).
- capture_check.py on the standard prof line set still reports CAPTURE CHECK OK for one prof capture row of your choice from docs/handoff/push_runbook.md.
- Edit with the Edit tool (CRLF sources). Hand back with ONE SubagentHandback message: commit hash, census path, lines before and after, readers kept.
