# Row CORE: record mode, NdRec kinds, rect/fill and range records

## Goal
Producers write typed native records (NdRec) into the main list instead of N64 words, so pm_gbi.c can be deleted (S10). Plan: docs/handoff/plan_s10_path.md (read Producer inventory, Streams, Stream CORE, Decisions D1 to D10). Plan decisions are fixed.

## Start state
Branch nds1, HEAD 4f3daf97a. nd_gbi.h bridge leaves force-included in main_loop.c, cam_main.c, background.c, background_gfx.c (CMakeLists.txt near line 338). gbi_task (pm_nusys.c) cuts the main list as one LEGACY span.

## Rows (in order, one commit each)
CORE-1 RECORD MODE, CORE-2 NdRec KINDS, CORE-3 RECT/FILL (after RECT-1 lands: grep git log for "RECT-1"; if not in, hand back after CORE-2 with CORE-3 open), CORE-4 RANGE (after RECT-2b lands, same rule). Text: plan Stream CORE. CORE-5 to CORE-8 are not in this run.

## Touch only
3ds/tools/gen_nd_gbi.py, 3ds/include/nd_gbi.h, 3ds/include/nd_gbi_leaf.h (generated), new 3ds/include/nd_rec.h, 3ds/include/nd.h (NdRec kinds), 3ds/src/nd_core.c (nd_emit, nd_bridge_cut), 3ds/src/nd_state.c, 3ds/src/pm_nusys.c gbi_task, 3ds/CMakeLists.txt (opt-in list), 3ds/tests/ or 3ds/tools/nat_host_test.c (host test). pm_gpu.c only as a caller of its API (pm_gpu_fill_rect, pm_gpu_tex_rect, pm_gpu_gtrip), never an edit. pm_gbi.c: no edit.

## Done criteria
1. CORE-1: host test setter state = nd_words state for every leaf; spot_mac01_pic byte-equal with no file opted in.
2. CORE-2: main_loop.c and cam_main.c opted in; spot_mac01_pic byte-equal; nd_emit order log equal to the walker op order (ndref).
3. CORE-3: background.c, background_gfx.c opted in; walker ops of camera_render and the 2 background lists 0 (WCEN census); mac_00 sky right by eye.
4. CORE-4: walking_dust.c opted in; its walker ops 0; dust right by eye at mac_01.

## State (2026-11-06, after CORE agent 1)
CORE-1 done: 1f5e0db04, host test pass, spot_mac01_pic byte-equal (dc7f1c438 vs 1f5e0db04). bench.py build-at fixed: 813a8bfed.
Worktree build3ds/wt/core, branch core, at 813a8bfed, assets/us and ver/us/build copied, map_restore_table.c copied (never commit it). Work ONLY in this worktree. Build there (PM_LANE=core), azt -Build <worktree 3dsx path>. Rebase on nds1 before each gate.
CORE-2a split into small commits (each: build, spot_mac01_pic byte-equal against the previous commit, then commit):
  2a-1: struct NdWs in nd.h, `NdWs nd_ws` in nd_state.c, and in pm_gbi.c `#define omode_h (nd_ws.omode_h)` style aliases for omode_h, omode_l, cimg, cimg_siz, zimg, geom, prim, env, fog, blend, fill (delete the old globals). Small diff.
  2a-2: same for td[0..7] and the tile state (tst1 and the other tile fields), segments, combine words.
  2a-3: delete nd_st; nd_words writes NdWs only for nd_dlc compile callers (not during the walk); replace the aliases by `ws->x` only if the diff stays under 150 lines, else keep the aliases.
Then 2b (setters, one op group per commit) and 2c (record word) as the oracle plan says. A big file or a slow build is not a reason to hand back.

## Oracle plan CORE-2 (2026-11-06, Opus; replaces CORE-2 to CORE-4 of plan_s10_path.md Stream CORE)
The CORE-1 leaves decode at macro time, and that is the root of (a) and (b). State decoded at macro time is not the state at walk time: replays walk the same memory again, the walker has its own globals, and the bg and key lists run later. So a record-mode leaf must encode a record into its own slots, and the state change must happen when the record runs, through the one owner of the state.
(a) Where records live. They live in list memory, in the leaf's own slots. There is no nd_rec array per task. Replays (key_list, bg_list, multi-task frames) then walk the same records again, exactly as they walk words today. nd_frame_begin keeps clearing only the LEGACY array; nothing a record needs is in it.
(b) Keeping walker and native state in step. They become one state. The walker globals become the native state struct, and each op has one setter that the walker op and the record both call. There are never two states.
(c) Cursor. A record fills exactly the leaf's nslots slots, never more or fewer, so caller cursor arithmetic does not change.

Record word, decided:
- New opcode G_PM_ND (take the next free custom code after 0x11; check gbi_custom.h for free codes).
- w0 = G_PM_ND << 24 | setter id << 16 | skip << 8 | small arg; w1 = the argument (a colour, a pointer, a packed field).
- A leaf with more than one slot (gSPClipRatio 4, gSPTextureRectangle 3, gSPVertex 1, gDPLoadTextureBlock as its leaves) writes one record in slot 0 with skip = nslots - 1, and the payload in the following slots (8 bytes each).
- The walker case G_PM_ND calls nd_set_tab[id](slots) and advances by skip. No word decode.
- At S10, this dispatch is the whole nd_emit loop over list memory (about 20 lines in nd_core.c).

Rows:
CORE-2a ONE STATE. Move the walker state globals into NdWs in nd.h/nd_state.c: omode_h/l, cimg and cimg_siz, zimg, td[0..7] and the tile state (tst1 etc.), prim, env, fog, blend, fill, primz, scissor, geometry mode, texture on/scale, segments, combine words. pm_gbi.c reads and writes them through `NdWs* const ws = &nd_ws`. nd_st (the shadow) is deleted; nd_words writes NdWs only for compile (nd_dlc) callers, never during the walk.
- About 150 lines, mostly renames.
- Gate: mac_01 byte-equal; host tests pass; no behaviour change.
CORE-2b SETTERS. One function per state op in nd_state.c: nd_set_om, nd_set_comb, nd_set_prim, ..., nd_set_tile, nd_set_timg, nd_set_load. Each takes the decoded arguments and does the walker side effects in the same place (flush on change, sync_combine, the g2_same shadow, sc_ok/sr_ok). The walker case for each op decodes the word and calls the setter. Do one op group per commit if it passes 150 lines.
- Gate: mac_01 byte-equal; iwa_10 by eye; host test: setter state equals word-decode state for nd_gbi_seq.inc.
CORE-2c RECORD WORD.
- gen_nd_gbi.py changes nd_rec_leaf. When in_main, the leaf writes the G_PM_ND record into its slots (id plus packed arguments); it no longer decodes at macro time. When not in_main, it writes the plain words, as now.
- pm_gbi.c gets the G_PM_ND case (dispatch table).
- The predicate covers every buffer gbi_task walks: mainGfx and backgroundGfx of both display contexts. This is fixed now, so the bg list can opt in later.
- Opt in main_loop.c and cam_main.c.
- Host test: a leaf writes exactly nslots slots, and the record replay state equals the word state.
- Gates:
  - mac_01 byte-equal against HEAD;
  - an interp-on run (pm_interp_on above 0, key replay) byte-equal on 3 frames;
  - yard_stereo_l right by eye;
  - census: the camera_render op count splits into rec_nd n plus words 0 (G_PM_ND is counted as rec, not op).
CORE-3 RECT (after RECT-1b lands). gSPTextureRectangle (3 slots) and gDPFillRectangle (1 slot) records carry their fields by value. The emit calls nd_texrect and nd_fill (gbi_texrect and gbi_fillrect moved to nd_rect.c, reading NdWs, still calling pm_gpu_tex_rect and pm_gpu_fill_rect). Opt in background.c and background_gfx.c.
- Gates: word ops 0 for camera_render and the 2 background lists; mac_00 sky and mac_01 byte-equal against the CORE-2c build; pause sky by eye.
CORE-4 RANGE. Records for gSPVertex (pointer, n, v0; the pointer is read when the record runs) and gSP1Triangle/gSP2Triangles. The emit calls nd_vtx and nd_tri (G_VTX and draw_tri moved to nd_geom.c, after TRI-4). Opt in walking_dust.c.
- Gate: its word ops 0; dust right by eye at mac_01; mac_01 byte-equal.
Replay rule: records are written once at build time and never changed by a walk, like words. A record holds values or pointers to memory that lives as long as the list (the frame heap, matrixStack, static data), never to task-local memory. A record kind that needs more breaks this rule and stays as words.
Transition rule: until S10, the walker is the record dispatcher, so opted-in files and word files mix freely in one list, in order. No LEGACY cut per record is needed. nd_bridge_cut stays as one span per task.
What CORE-1 must change: the macro-time decode in nd_rec_leaf is replaced by the record encode (CORE-2c). The nd_rec_test state compare moves to "replayed record state equals word state".
Gate per row (summary): mac_01 byte-equal for CORE-2a, 2b, 2c, 3, 4 (shared state code); the interp replay and the stereo check at CORE-2c; the census word ops of the opted-in producer 0, with rec_nd counted separately.

## Rules
CLAUDE.md rules 5, 7, 9. Decisions D1 to D10 of docs/handoff/plan_walker_end.md are fixed. Other agents edit pm_gpu.c, pm_gbi.c, pm_tex.c, nd_*.c at the same time: edit only the functions your stream owns (plan table "Streams and ownership"); commit only your hunks through your own temporary index (GIT_INDEX_FILE copied from HEAD), never the shared index; no git add -A, commit -a, reset, stash, checkout, restore. Build in a scratch tree from HEAD plus your diff (git archive HEAD, apply your diff, copy assets/us and ver/us/build, gen_map_restore.py, copy 3ds/src/map_restore_table.c), lane named after your row; azt.ps1 <scenario> -Row <row> -Build <lane>. After each code commit build the committed tree clean. Status line in 3ds/logs/perf/status_<row>.txt before each Azahar start. Budget: reset (powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget <row>) and continue. No toggles (debug switches allowed, removed in the deletion row), no stubs, no push, no attribution lines, never commit 3ds/src/map_restore_table.c. Two tries per named cause, then the next code point. A new scenario goes into azt_scenarios.json and bench_defs.json in the same commit. Delete this file in the commit that completes the last row.
