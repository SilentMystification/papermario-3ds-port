# HANDOFF (orchestrator, overwritten at each handoff)

State: 20 Nov, about 06:25, main near e5953a4e31 (`git log`). Goal of the session: a fully native draw path (core 0 decides nothing). Start: read this file, docs/handoff/orchestrator_rules.md, CLAUDE.md, docs/handoff/plan_native.md, docs/handoff/oracle_handoff.md, docs/handoff/merge_protocol.md, docs/handoff/push_runbook.md and the memory index. Then `date`, ListAgents, `git log --oneline -15`, `git status`.

## Numbers (FACT, `python 3ds/tools/c0gate.py <ELF>`; calls / interp / state)
- Start of the effort 127 / 37 / 23. Main now 0 / 0 / 1: the one state symbol is `nd_c0`, the struct that took the fields of nd_ws and 7 globals (NWS3). The user decided that nd_c0 counts (names list). Removing that state is an open goal, not a rename.
- `elfgate` func 0 data 0 gfxnames 0 (display list goal met). listcen totals 0.
- c0gate counts calls from the SOURCE TREE it runs in: run it from a clean `git archive` of the commit, with the ELF of that same commit for interp and state (basebin: `python 3ds/tools/bench.py base --prebuild --commit <hash>`). Never the tip lane ELF for a base.
- Wave gate WAVE1 (all 4 drec benches byte-equal to the session start 9576c03383) passed on main 728348b9a9. NWS3 then passed all 5 drec benches (spot_mac01, entcape_kmr03, fxparts, wscmb_pause, hudfb) byte-equal.
- Gate measure changes the user accepted: NAT6G2 removed `interp gt_run` (gt_run runs no unit); NWS1 removed 5 names whose symbols were deleted; `state nd_c0` added (CLEAN2).

## Console capture (O3DS .170, build 9f6be238aa, mac_01 0 0 200, 38 windows, fps 37.7)
Core 0 = wk - wwait, core 1 = lgms - bw. Last 3 windows: core 0 18 to 25 ms, core 1 44 to 50 ms, ratio 2.0 to 2.45 (limit 1.20). Core 0 waits 15 to 17 ms per window for core 1. c1t: snap 7 to 9 ms, alt 6.6 to 7.8 ms. Old capture (build 9576c03383): 1.47. Logs: build3ds/hw_o3ds_log.txt, run 2435651 (build number label 7420 is a guess). Next work if the user approves: balance rows (pure decisions back to core 0 at their call sites), designed by an oracle from the c1t and c1n data.

## Open items
- User decision pending: balance rows after the capture (ratio far above 1.20).
- ws_WALKSRC.md stays (branch WALKSRC not merged). Empty folders in build3ds/wt (Permission denied on delete): ignore.
- Parked, never start unasked: ASSETFREE, CIA, QTM.
- Native-path proof beyond counters (user wants core 0 to decide nothing): nd_c0 still holds the core 0 draw state. Read what core 0 decides before saying done.
- The 7 ndmcheck benches stay (the switch still changes the path: pm_gpu.c:3785, nd_recs.c:173, :256).

## How this session worked (lessons, FACT)
- Rows ran in parallel when their edits sat in separate code regions (user rule). Pages were written by Opus oracles (NATIVE8 to NATIVE14), coders were Sonnet or Haiku, one Haiku merger agent was resumed for every merge. Oracles cannot start coders (permission denied): the orchestrator starts every coder.
- Cost: about 40 rows, each with fixed reading cost; large oracles (400K to 500K tokens) were resumed often; timed wakeups were polling. Rule 7 now says no polling. Lean mode: no new oracles, batch merges, one oracle resume per round.
- A coder that ends a turn on a background job goes idle: resume it with the foreground order (happened about 10 times).
- Tiered tests (CLAUDE.md rule 3): host tests and c0gate every commit; 1 to 2 benches per row; a wave gate with all benches about every 5 rows.
- Expected-diff merge: message file with the word expected-diff as merge_row.sh argument 2 (used for NWS4B).
- Merge order matters when rows share files: the later branch merges main, rebuilds and reruns its gate (NAT6F, NSET2).
- Agent ids die with the session: start fresh agents after a restart.

## Gate-definition changes this session (for the user to review; FACT from the merge messages)
The measure is `3ds/tools/c0gate_names.txt` (names) and `3ds/tools/c0gate.py` (rules). Changes, each with its reason:
- NAT6F (merge 6dcbbe695b) and NSET3 (merge 11aae628a2): names of deleted interpreter functions left the list (nd_nr_exec, nd_nr_run, nr_set, nr_unknown, ls_run, nd_unit_same; nd_unit_apply and 16 setters). The symbols were deleted from the source, so the names matched nothing.
- NWS1 (merge 5e3df581bf): `interp nd_comb_sync` and `state comb_gen cmb_gen_seen sc_ok sc_sig` left the list with their symbols.
- NAT6G2 (merge e923f5bce0): `interp gt_run` left the list. Reason: since NAT6E3 gt_run runs no unit (it brackets the snap range and takes a native arm). The same row fixed the c0gate selftest fixture (it called a name that no longer existed).
- CLEAN2 (merge e5953a4e31): `state nd_c0` joined the list (user decision): the struct holds the core 0 draw state that nd_ws held, so main reads 0 / 0 / 1.
- Still on the list but their symbols are gone: sc_adopt, ds_take, nd_comb_set, keyc, keys, fogc, blendc, dual_on, box_folded. A name off the list could no longer catch a regression: keep these.
Review: judge whether `gt_run` (NAT6G2) should return as a name that must have no unit work. Capture data: docs/handoff/plan_perf.md section on the capture of build 7569.
