# docs/handoff index

Live files only. Older logs are in archive/ (agent_*, diag_*, inv_*, plan_*, trace_*, research_*, HANDOFF_*). Do not read archive/ unless a row file names a file there.
- HANDOFF.md: orchestrator handoff (what runs, the queue, user checks); overwritten at each handoff.

## Rules and process
- ../../CLAUDE.md: shared rules (tests, code, platform).
- orchestrator_rules.md: orchestrator only (workstreams, roles, merge steps, console); its Core block is printed every turn by the hook in .claude/settings.json. subagent_style.txt, subagent_hook.js: style hooks. no_guess_hook.js, no_poll_hook.js: PreToolUse guards (no guess and check; no sleep, ping or tail --pid in Bash).
- ../../.claude/agents/coder.md, oracle.md: agent definitions.
- plan_endgame.md: the closed plan to remove the display list system (census, gates, row families, counters, end proof); row_TESTIDEAS.md: the gate ideas and the host golden-master evaluation.
- oracle_handoff.md: start file of a fresh oracle. ws_TEX.md, ws_FXSELF.md, ws_GKEY1.md, ws_GKEY2.md, ws_GSET1.md, ws_NSTREAM2.md, ws_PSOLK.md, ws_RECROOT.md, ws_FXENT2.md, ws_GSET2.md, ws_GSET3.md, ws_FXENT3.md (the 42 effect nd_rec_dl rest sites: branch keys, state-only log, fxtest variants, chapter_change joins the converter), ws_NSTREAM3.md (core 0 local buffers write native records: O forms in nd_at.h and nd_tx.h, ns3unit, HUD and sprite core 0 setter fix), ws_GSETB.md (bulk gset.py pass: block mode for every source outside src/effects/gfx, hold lines, ND_GCONV_IN host form, set GSETB of 39 sources), ws_ROOTCM.md (codemod --state on 7 root files, codemod --ntab ND_SELF for gSPDisplayList of v2 tables: 59 C1/C2 calls), ws_GTIN.md (build.sh GT_IN lists the own-TU sources of gconv_src.d, host test gtin.sh), ws_LIT1.md (converter light units of Lights1 symbols; level_up.c and starpoint.c join the set and gset.py moves their lists), ws_ISEED.md (inherit seeds: effect C-site captures and entapply, converter call-unit seeds for list callees; Sonnet), ws_HWFIX.md (console-only faults of build3ds/out/audit_hw.txt and audit_hw2.txt: ring colour flush, stream chunks at boot, viewport pool, render mode index, VRAM extent table, depth read drain, menu grey invalidate, nd_rec_data vertex bytes; group 3 sc_sig_now hash fix with a base reset), ws_ISTAR.md (gset.py own rule: a file that defines a moved name keeps it; src/starpoint.c lists move to the gfxin twin; Haiku), ws_PAUSEH.md (the pause keeps the grab_tex copy of its first frame: no dim feedback; after Merge HWFIX; Haiku), ws_MOON1.md (the super jump and ground pound window has priority over the moon jump: pm_moon_allowed, host test moon.sh; Haiku), ws_MOON2.md (air control in a moon jump only: the stick sets targetYaw and curSpeed, pm_moon_air_speed; after Merge MOON1; Haiku), ws_XFT.md (map change timers seam, dlcp, entt in the xfer2 prof line; then a console prof run of one map change; Haiku), ws_COIN.md (black coins and field items: a combine that is not the shaded sprite combine clears the stale dual_on in op_setcombine, nd_dual.h, host test coin.sh; after Merge HWFIX; Haiku) (and design_lookahead.md: end forms for images, vertices, lights, CONV2C2, NEMIT, GSET): workstream goal pages (deleted by the commit that completes each). design_state_calls.md: one native setter per state-call shape, readers, writers and edit order.
- row_TEMPLATE.md: template for the row file each new agent reads first.
- hw_guidelines.md: 3DS cache and ARM11 rules for all code (3 KB).
- git_branches.md: tip hash of every branch deleted or kept in the row GIT cleanup (recovery record).

## Native draw epic (current work)
- ws_MESHST.md: Haiku goal page: animator mesh nodes draw built-once units (nd_mesh.h, nd_rec_ndev), no per-frame heap list (C1 -4, C2 -1, C7 -3); host test meshst.sh; jan09_mesh_drec judged by drecsum.py (user OK of a non-byte-equal gate, 2026-11-15). Parallel with ws_CHCONV.
- ws_CHCONV.md: Haiku goal page: chapter_change joins the converter, 16 lists move to the twin (user decision 2026-11-15; fxr_drec differs from frame 1512 only); revert recipe in the page. Parallel with ws_MESHST.
- ws_SORTQ.md: Haiku goal page: gt_sort and nt_sort (nd_gtab.c) become qsort (audit_hw3 F3, boot cost); host test sort_check in nd_gtab_test.c (conv2c.sh, seeded swap); spot_mac01_drec, sbk14_drec byte-equal. After Merge MESHST.
- ws_PIPEFX.md: Haiku goal page: imgfx_load_anim types the raw ROM anim list (new nd_imgfx.h, bounded by gfxCount); Mario visible in pipes; host pipefx.sh with seed; spot_mac01_drec, sbk14_drec byte-equal.
- ws_DLCRD.md: Haiku goal page: nd_shape.c reads the .dlc whole (fmemopen) and buffers the .seam read (map change lag); spot_mac01_drec, sbk14_drec byte-equal; user sends the xfer2 map line before and after.
- ws_GSETI.md (+ ws_GSETI_code.md): Haiku goal page: gset.py block mode moves the all-ok asset includes of a block source (INC_BLOCK, OWN rule for include names); 25 world, battle and root sources join the converter; dry run C5 -771, C6 -78; host conv2c and 4 drec benches.
- ws_FXVROM.md (+ ws_FXVROM_code.md): Haiku goal page: the converter splits a vertex load past its array end in ROM order (device layout), the host compare (m_tri) maps the index the same way; seeded host test vrom.sh; gset moves 33 effect lists; conv2c bad 0; 4 drec benches byte-equal.
- ws_FXTYPE.md (+ ws_FXTYPE_code.md): Haiku goal page, after Merge FXVROM: fxtype.py gives 89 effect list handle arrays the type NdLst (nd_gtab.h typedef const void*); C7 -89; host test fxtype.sh with seed; 4 drec benches byte-equal.
- ws_LKEY.md (+ ws_LKEY_code.md): Haiku goal page: list readers read the device units of a moved list key (new nd_lkey.h; entity_model.c, energy_orb_wave.c, nd_ent.c, nd_walk.c, nd_rec.c; the VCOUNT fault class) and the reader census host test (lkey_cen.py, allow.txt, seeded); spot_mac01_drec, sbk14_drec byte-equal.
- ws_GRABSEQ.md (+ ws_GRABSEQ_code.md): Haiku goal page: the grab hold (Tidal Wave, fx_underwater) and the splash grab copy once per drawn frame (pm_gpu_seq), not once per frame number (60 FPS walked key frame: flashing blue rectangle); host grabonce.sh with seed and lint; fxr_drec, spot_mac01_drec byte-equal. Disjoint from GSETI, LKEY, FXVROM.
- start_here_native_draw.md: current state and what is left (long-form handoff).
- handoff_native_epic_orchestrator.md: findings and pitfalls of the code session of 2026-11-05.
- plan_native_system.md: design (large; open only the sections a row names).
- plan_native_slices.md: slice table S0 to S11 and rows (large; open only the rows a row names).
- design_native_draw.md, design_producer_models.md: fidelity rule and producer record design.
- frame_budget.md: 16.7 ms stage budget and O3DS XL baseline.
- agent_nds_s0.md to agent_nds_s1e.md, agent_nds1_e6.md: logs of finished agents (state at their end).
- agent_direct.md: DIRECT epic handoff (state after D9: no task list; next D10, the walker removal).
- plan_direct.md: plan DIRECT (2026-11-09): the game records native commands into the frame stream (NdCmd) on the logic thread, core 0 replays them; LIST span bridge, producer API per class, slices D0 to D9, memory and thread rules.
- epic_ASSETFREE.md: PARKED, RELEASE BLOCKER, do not start (user): future epic that removes the game assets from the ELF; the console makes them on first boot.
- plan_nativeapi.md: native draw API plan (2026-11-11, user decision): the game's draw code records native commands (pipeline-state id, texture handle, rect/quad/glyph/mesh/particle calls) instead of N64 state ops; stages S0-S6, each merged before the next; the old path stays until the last call site moves. First row NAPI0.
- plan_datapath.md: data-path design study (2026-11-11): SD to RAM to CPU/GPU hops per asset class, transform-once principle, native-asset generation pass (links to epic_ASSETFREE.md), censuses DPCEN1 and DPCEN2.
- plan_hotcode.md: plan HOTCODE (2026-11-09): shrink the core 0 draw code and data to fit the ARM11 L1 (hot set by class, stale link group, stages HC0 to HC10, size and pmu gates).
- plan_s10_path.md: plan for S10 (oracle 2026-11-06): producers write NdRec records (streams CORE, UI, FXW, WBO), then pm_gbi.c is deleted; decisions D1 to D10.
- agent_S7.md: WBO-1 site list (custom gfx builders in src/world and src/battle: 57 sites, kinds, scenario candidates) and the WBO-2a to WBO-3 row groups.
- agent_s10.md: S10 epic handoff (orchestrator, 2026-11-07): merged rows, open rows, parked bugs, size and push state, next command.
- agent_wcen.md: WCEN walker producer census at 5 scenes (top 8 per scene, function, file, slice owner). Census tool: pm_gbi.c os_report.
- agent_boot.md: row BOOT (boot time): ROM stream at boot (pm_romload.c), boot ms line, boot ROM reads, the elf size listing.
- agent_bg.md: background image faults (BG1 flicker at mac_00, BG2 garbled row), how the background strips are drawn.
- map_e4.md, map_rtb1.md, diag_e53_*.md, diag_s1_1_picture.md, diag_uc3a_bad.md, inv_frame_cost.md, plan_prof_mode.md, nds1_*.patch, nds1_status_2026-11-05.md: findings the plan files cite.
- perf_history.csv: console prof history (python 3ds/tools/prof_record.py add|trend). Its rows are imported to runs.jsonl by `bench.py import-csv`.

## Tests
- 3ds/tools/bench.py: entry point for every test and profiling run (`bench.py list`, `show`, `run`, `ingest`, `compare`, `trend`). Benches in 3ds/tools/bench_defs.json, runs in 3ds/logs/perf/runs.jsonl.
- test_catalog.md: THE bench process (situations A to F: check a row, before/after, console run, area lookup, add a bench, rebuild). Read it before any test. Old scenario tables: archive/.
- route_test.md, sweep_420.md: tour and sweep how-to.
- audit_system.md: system audit of the 3DS runtime (row AUDIT, 2026-11-08): interface map with the 8 threads, findings A (console cost), B (correctness and seams), C (maintainability) with file:line, and the work list W1 to W10 with each one done or open.
- plan_core2.md: plan CORE2 (oracle 2026-11-08): more work off the game thread (audio one step ahead, walker thread on N3DS core 2, texture worker), rows CORE2-0 to CORE2-4.
- plan_tours.md: plan TOURS (oracle 2026-11-08): map tour and boss battle tour in one session with prof windows per warp, curated list, perfect action cheat; rows TOUR1, CHEATA, TOUR2, BT1, CUR1.
- audit_warm_runs.md: audit of warm restarts (stale ROM, AZT8 and AZT9c) and picture gates (stale shot, AZT7) for 2026-11-06 12:00 to AZT9c: per gate OK or SUSPECT with run lines, and the rerun list.
- work_queue.md: older queue (stale; check git log first).
- ws_DECDOCK.md: decode_dump.py runs addr2line in docker and reads a crash.txt entry (tool fix, Haiku).
- ws_VCOUNT.md: map change crash fix: mdl_get_vertex_count reads the device units of a moved list (hidden panel stomps 2.9 MB of .data), Haiku.
