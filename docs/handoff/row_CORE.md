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

## State (2026-11-06, after CORE agent 2, round 2)
Branch core head e6bec7e73 (worktree build3ds/wt/core): 2a-1 a63423977, 2a-2 ed88f0c44, 2a-3 e6bec7e73 (nd_st deleted, om_kh/om_kl/prim_lod in NdWs, g2_note on skipped ops), each spot_mac01_pic byte-equal. Not merged into nds1 yet (shared pm_gbi.c held by the super block agent).
Decision on the record word (orchestrator, 2026-11-06), replaces the G_PM_ND encoding of the oracle plan for one-slot leaves:
- A one-slot STATE leaf keeps its N64 word as its record: the opcode byte is the setter id and the 64 bits are the payload. The walker case for that opcode calls the setter (2b) with the decoded fields. No re-encoding, so no bit-width problem. At S10 this setter table moves to nd_state.c as the record dispatcher (plan_s10_path.md D3: the decoder that stays).
- G_PM_ND is used only where a record must carry more than the leaf's words or a pointer read at emit time (texrect by value, vertex pointer records): 2-slot or n-slot form, skip = nslots - 1, payload in the following slots.
- So CORE-2c shrinks to: the record-mode predicate covers mainGfx and backgroundGfx of both display contexts; opt in main_loop.c and cam_main.c; host test (a leaf writes exactly nslots slots; record replay state equals word state); gates as the plan says.
2b scope: all setter groups, but gate in batches: one spot_mac01_pic byte-equal run per 4 to 5 setter groups (one commit per batch). If a batch differs, bisect inside it. Setters stay static in pm_gbi.c for now (2b); they move to nd_state.c with CORE-5 to CORE-8.

## Oracle review CORE record and CORE-3 (2026-11-06, Opus; replaces the CORE-3 G_PM_ND reply and the opt-in steps of the oracle plan CORE-2)
(1) Record word, 64d344659: confirmed. The N64 word is self-contained: opcode plus fields, and a pointer that is read when the op runs. Re-encoding it buys nothing, so the word is the record, and decoding its fields in the setter is the record reader. My G_PM_ND layout was the wrong tool for this.
What follows from it (decided):
- At 2da2a6947, nd_rec_leaf is a memcpy of the leaf words, and the bridge leaves (nd_gbi.h) no longer decode. Both force-includes are now layers that change nothing. Delete nd_rec.h, nd_rec_leaf.h, nd_gbi.h, nd_gbi_leaf.h, the gen_nd_gbi.py record and bridge output, and the CMake lines 338-339, in CORE-3 (rule 7: no dead layers). Keep nd_rec_test's "replay state equals word state" check, rewritten against the dispatcher.
- So "opt a file in" no longer means anything. Producer work in UI, FXW and WBO shrinks to:
  1. replace the custom word hacks (G_PM_TEXI, G_PM_RM, the gPmSplashMark NOOP) with handled ops or native-only records;
  2. the WEND draw paths;
  3. side-buffer lists compiled through nd_dlc.
  Per-file force-includes are dropped from plan_s10_path.md.
- S10 now means: the dispatcher (opcode to handler table) and every handler live in nd_state.c, nd_rect.c, nd_geom.c, nd_mtx.c, nd_dlc.c and nd_core.c; pm_gbi.c is gone. There is one decoder for both walked lists and nd_dlc compile, never a second copy.
- Rule for G_PM_ND: only for native-only payloads that have no N64 encoding (glyph by sheet and index, splash on/off, target role, the TEXI/RM replacements). It never wraps N64 words.
(2) CORE-3: replaced. Texrect and fillrect keep their words. G_TEXRECT is the fixed F3DEX2 three-slot form (TEXRECT, then RDPHALF_1, then RDPHALF_2), and gbi_texrect already reads the two halves. G_FILLRECT is one slot, so wrapping it in G_PM_ND would also break rule (1).
CORE-3 corrected:
- Move gbi_texrect and gbi_fillrect (plus FILL_SX10 and the HUDB1 inclusive-corner rule) to nd_rect.c as the handlers of G_TEXRECT and G_FILLRECT. They read nd_ws and call pm_gpu_tex_rect / pm_gpu_fill_rect.
- The pm_gbi.c pc loop and the g1/g2 tables call them through the dispatcher entry.
- No G_PM_ND, no opt-in. Delete the bridge and record layers as above.
- The census counts ops by handler module, not by "word vs record".
- Gate: for camera_render and the 2 background lists, ops served by pm_gbi.c handlers = 0 (all in module nd_rect); mac_00 sky and mac_01 byte-equal against the CORE-2c build; pause sky by eye.
S10 precondition (census over the full catalog plus a grep). All must hold:
1. Per-opcode census of every walked list (main, bg, key replay, ND_CALL side lists): every opcode seen has its handler outside pm_gbi.c, and the "legacy handler" counter is 0.
2. G_PM_TEXI, G_PM_RM and the splash NOOP mark: 0 hits, and grep `G_PM_TEXI|gSPPmRenderMode|SPR_RM_LIST|gsSPPmTexInline|gPmSplashMark` over src and 3ds is empty.
3. wd = 0 (WEND).
4. No pm_gbi.c function has a caller outside pm_gbi.c (nm or grep of every exported pm_gbi_* symbol). The structured records G_PM_REC, G_PM_SPR and G_PM_ENT are allowed; their handlers move to nd_model.c, nd_sprite.c and nd_dlc.c in CORE-8.
5. Opcodes with 0 hits and no macro caller are deleted, not moved (WEND END-1).

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
