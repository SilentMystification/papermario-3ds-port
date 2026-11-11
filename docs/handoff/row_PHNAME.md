# Row PHNAME: name the phases for what they cover (PH_GBI -> PH_DRAW, PH_DL -> PH_GAMEDRAW); log keys stay

(Model: Haiku. Base: main. Edits identifiers and comments only: 3ds/include/pm_port.h (enum :87), 3ds/src/pm_phase.c, 3ds/src/pm_nusys.c
(:151, :192), 3ds/src/pm_wdog.c (:27 text), 3ds/tools/pmu_report.py (display names). Disjoint from PALGEN, DPCEN2, PBATCH2. Reads this file and CLAUDE.md.)

## Facts (oracle code read)
- PH_GBI is pushed in nuGfxTaskStart (pm_nusys.c:151) and pm_key_walk (:192) around gbi_task_w: the walk of the recorded typed-op stream
  on core 0 (nd_walk, nd_emit, pm_gpu, nd_tex_set, nd_comb): the native draw path. No N64 word is decoded there (HC4e4).
- PH_DL wraps __wrap_gfx_draw_frame and __wrap_gfx_task_background (pm_phase.c:1112-1122): the game's draw functions that record typed
  ops through the nd_gbi.h macros, on core 1 (the logic thread); in a prof sample the step runs inline on core 0.
- The log words "gbi" and "dl" (pname[], pm_phase.c:28; the `ph gbi` field, :596) are data keys: bench.py metric `gbi` (:1989-1997),
  prof_by_scene.fields, console_numbers.py, attract_report.py, perf_az_full.py, perf_speed.py, phase_report.py, phs_report.py, and
  every runs.jsonl record. Renaming them would split the history: they stay, with a comment.

## Start
`sh 3ds/tools/wt_new.sh PHNAME`. PM_ROW=PHNAME PM_LANE=PHNAME on every build and bench call.

## Exact edits
1. pm_port.h enum (:87): `PH_GBI` -> `PH_DRAW`, `PH_DL` -> `PH_GAMEDRAW`; comment: `PH_DRAW: core 0 walk of the typed-op stream and the
   native draws (log key "gbi"); PH_GAMEDRAW: the game's draw functions recording typed ops, core 1 (log key "dl")`.
2. Every use: `grep -rn "PH_GBI\|PH_DL\b" 3ds src` (pm_nusys.c:151, :192; pm_phase.c:116, :720, :1024-1025, :1113, :1119): the new names.
3. pm_phase.c pname[] (:28): keep "gbi" and "dl", add the comment `log keys, unchanged (bench.py, runs.jsonl): gbi = PH_DRAW, dl = PH_GAMEDRAW`.
4. pm_wdog.c:27 phase text "game step and display list build (gfx retrace)" -> "game step and its draw recording (gfx retrace)"; the
   "gpu end (gbi run, submit, frame end)" text -> "gpu end (stream walk, submit, frame end)". Same for any crash-page copy of these texts
   (grep the two old strings).
5. pmu_report.py: print phase "gbi" as "draw (gbi)" and "dl" as "gamedraw (dl)" in its tables (display only).
6. Comments that say "display list build" for PH_DL (pm_gpu.c:2474, pm_main.c:322): "draw recording".

## Gate (CLAUDE.md 2; Azahar budget 2 of 8)
- Build, memcheck OK; nd_host PASS; `git grep -n "PH_GBI\|PH_DL\b"`: no hit.
- `bench.py base spot_mac01_det`, run, `bench.py cmp`: 6/6 byte-equal; its log still has the `ph gbi` prof line.

## Stop and end
A failed gate, an edit that does not fit, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in
the completing commit. Hand back the hash, gates, memcheck line.
