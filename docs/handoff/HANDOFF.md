# HANDOFF (orchestrator, overwritten at each handoff)

State: 20 Nov, about 12:00, main f0a9f642e4 (`git log`). Goal of the session: a fully native draw path (core 0 decides nothing). Start: read this file, docs/handoff/orchestrator_rules.md, CLAUDE.md, docs/handoff/plan_native.md, docs/handoff/oracle_handoff.md, docs/handoff/merge_protocol.md, docs/handoff/push_runbook.md and the memory index. Then `date`, ListAgents, `git log --oneline -15`, `git status`.

## Numbers (FACT, `python 3ds/tools/c0gate.py <ELF>`; calls / interp / state)
- Main f0a9f642e4 (build 7680): c0gate 0 / 0 / 0. The state symbol nd_c0 is gone (NC0T2). elfgate func 0 data 0 gfxnames 0. Host tests, binchk, build key all pass. memcheck margin 402 KB.
- Core 0 state that remains lives in cache objects outside nd_c0: nd_txw 752 B (texture model: bind fallback after a copy texture unit in a static call, glyph bind, TMEM rects, model full key), nd_gm 28 B (GPU memos), nd_bo 20 B (bind outputs). The user decision on whether these count as 'core 0 decides nothing' is open (questions 9, 10, 17; defaults yes).
- Optimization rows merged this session after the capture: NC0B NC0R NC0C NC0S NC0V NC0M NC0T1 NC0F NC0T2 (nd_c0 912 to 0 B), MCOPY1 MCOPY2 (core 0 model copy words 0; core 1 model alt range words 1752196 to 0 in spot_mac01_drec), T1FIX, ALT2C (census). Output changes accepted as expected-diff (oracle decision): MCOPY1 (marker mmiss), NC0V step 1 (marker texfog) and step 2 (marker gstex). Each row was byte-equal against its own start commit; no wave gate against the session start exists (expected-diff rows).

## Console capture (O3DS .170, build 9f6be238aa, mac_01 0 0 200, 38 windows, fps 37.7) - taken BEFORE the rows above
Core 0 = wk - wwait, core 1 = lgms - bw. Last 3 windows: core 0 18 to 25 ms, core 1 44 to 50 ms, ratio 2.0 to 2.45 (limit 1.20). Logs: build3ds/hw_o3ds_log.txt, run 2435651.
Next step needs the user: one capture of main f0a9f642e4 (build 7680) on O3DS .170 to read ic1, lgms, c1a m (expect 0), c1d, c0r. It picks the next frame-time row (core 1 untimed recorder about 30 ms GUESS, I-cache 92K misses per step, MDLHIT2 core 0 rec 6.15 ms). ALT2 is parked (gap runs only; GUESS 0.2 to 0.4 ms).

## Open items
- User word needed: console capture (above), then balance rows from the data.
- User decisions open with defaults: build3ds/out/questions_for_user.md (items 1 to 17).
- ws_WALKSRC.md stays (branch WALKSRC not merged). Empty folders in build3ds/wt (Permission denied on delete): ignore.
- Parked, never start unasked: ASSETFREE, CIA, QTM.
- Gate-definition changes: see below; add: `state nd_c0` now matches no symbol (count 0).

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
