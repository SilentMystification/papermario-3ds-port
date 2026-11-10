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

## Code paths (oracle batch 2, main cfb138b8e)
Paths that can crash or corrupt in a stereo frame (3ds/src/pm_gpu.c):
- P1, real defect. sr_frame_begin (:2001-2003) mallocs sr_ent (8 KB) and sr_fb on the first stereo frame. If sr_ent is NULL, then sr_on = 1 and sr_over = 1, and sr_take (:1983) and pm_gx_drain (:2145) write through NULL. That is a crash at the first stereo frame, which matches crash (a).
- P1b. If only sr_fb fails, the `if (!sr_ent)` retry never comes back. Each frame is then a fallback copy, so the 3D is flat.
- P2. pm_gpu_menu_present_w (:1622-1628) sets stereo_frame and gfxSet3D without the u_eye test and without sr_frame_begin. It runs in the frozen menu pass, so a slider change there leaves sr state and eye_e out of step for that present.
- P3. A replayed GX copy (sr_replay :2077-2079) maps only its source. If a copy's destination is the left target, the replay writes right-eye pixels into the left eye after its pass. That is a wrong picture, not a crash.
- No crash path found in: the fill mapping (data[1,3,4,6] is the correct MemoryFill layout); the vertex ring (ring_pick never reuses a chunk of the current frame, pm_gpu_rect.c:38-58); the command buffer (1.5 MB, no wrap within a frame); targetR (its own 240x400 colour and depth); gfxSet3D (called only at a frame begin). The dark room (appendGfx_darkness_stencil, screen_render_util.c:130: a 64x64 off-screen pass in nuGfxZBuffer) and the Merlee darken (shroud tint, npc_merlee.c:126-150) reach the off-screen bind (bind_main) and the copy replay, both covered above.
- Remaining risk for crash (b): CRASHALL logs PC/LR on the next console crash. That log goes to the oracle with the build hash.
Fix row: row_STEREOFX1.md.

## Crash on Azahar (AZTSTEREO run, oracle batch 4)
What the crash files show (lane-AZTSTEREO/slot1, pm_3ds.map, and objdump of pm_3ds.elf):
- At the crash, R4 = taskList = 0 and R5 = RenderTaskCount = 93. LR 0x1977b4 is the `blx r7` that calls appendGfx, and R7 = 0.
- Azahar logged `unmapped Read32 @ 0x0855CD70` twice: once in queue_render_task (inlined in the block at 0x1E0A18, radiating_energy_orb_render) and once in execute_render_tasks (0x197370). This is the read of `RenderTaskLists[RenderTaskListIdx]`.
- RenderTaskLists is at 0x01041960. So `RenderTaskListIdx` (0x0104196C) = (0x0855CD70 - 0x01041960) / 4 = 0x01D46D04. That value is a newlib heap pointer: `__end__` is 0x0153E548.
- The unmapped read returns 0, so taskList = NULL. The unmapped writes are only task 92 (0x5C0..0x5CC, appendGfx = 0x1E0A60 radiating_energy_orb_appendGfx, arg 0x00D6BC00). Tasks 0..91 of the same frame were queued into a valid list.
- So a stray CPU store of a heap pointer hit RenderTaskListIdx in this frame (f650 or later, after "merlee: darken done" f633), between the 92nd and the 93rd queue_render_task, during render_effects_scene. RenderTaskCount (next word) is intact, so it is a single 32-bit store, not a GPU block copy.

What I ruled out by code read:
- The stereo replay: it skips DMA and display transfers and replays only fills and copies into VRAM (pm_gpu.c:2060-2083). The GPU cannot write one word into .bss.
- pm_depth_query: it checks id 0..15.
- The texture panner setters, nd_ent and imgfx: none fits.
- The code that writes the value is not named, so I give no code fix (repro gate: wrong value printed, writer unknown). The cheapest exact next step is a gdb write watchpoint.

Next (needs the user's yes, rule 1 exception): (Diagnostic, 1 Azahar start. `sh 3ds/tools/wt_new.sh STEREOCRASH` on branch AZTSTEREO 2c29fb04a; it has the merlee_stereoat scenario. PM_ROW=PM_LANE=STEREOCRASH. CLAUDE.md rules 1-7. No git stash. Foreground waits.) One gdb write watchpoint run on RenderTaskListIdx (text in the oracle report, scratchpad oracle_b4.txt).

## Slider hang on the XL (oracle batch 8)
How the hang looks in the evidence (build3ds/hw_live_log_131.txt, build #3869):
- The push was a prof push with log=off, so no `stereo: replay` lines were logged.
- The last lines show a map load mac_05 -> kgr_01 (xfer lines #847-849). Then `watchdog ... phase 7`, `walker op ed, last flush pm_gpu_end_w:2485`.
- The HOME hang log (row_HOMEHANG.md) had the same `op ed, last flush pm_gpu_end_w` signature while the game sat idle in phase 10. So this signature is the normal state after any frame end. It only says the walk of frame N finished and pm_gpu_end_w entered flush.
- The game thread is the walker (pm_logic.c:159 pm_lg_show pumps the work queue). The stall is either inside a pumped GPU wait or in pm_lg_show waiting on lg_ends.

What libctru actually does (gxqueue.o disassembly):
- An entry counts as done only on a PSC0, PPF, P3D or DMA interrupt. PSC1 and VBlank are ignored.
- At most 3 entries are in flight (DoCommands: lastEntry + 3 - curEntry). The return code of gspSubmitGxCommand is ignored.
- gxCmdQueueWait(-1) loops on gspWaitForAnyEvent while isRunning. So a hang means exactly one thing: an entry was submitted (curEntry > lastEntry) and its counted interrupt never arrived.
- Why Azahar completes it: its GSP runs each GX command synchronously at submit and raises the interrupt at once. It has no PICA lockup and no rejected command, so the counted interrupt always comes.

Candidates I excluded, with reasons:
- gfxSet3D mid-session is safe. gfx.c:27-28 only sets gfxTopMode. The next swap (citro3d onQueueFinish) presents with 3D.
- The right framebuffer exists from gfxInit. The top size is 400*240*2*bpp (gfxSetScreenFormat). In 3D mode gfxGetFramebuffer(TOP, RIGHT) returns fb + MaxSize/2, which is inside the allocation.
- Fills on targetR are mapped correctly by sr_map (the end address uses `<=`). targetR is plain VRAM. Fills always have buf0 set, so PSC0 fires.
- Queue size is bounded: pm_gx_room keeps it under 24, and FrameEnd adds at most 3 (32 maximum).
- Command-list alignment: GPUCMD_Split pads every list (the twin too) to 16 bytes with FINALIZE.
- Entries queued between C3D_FrameBegin and sr_frame_begin (the PT12 copy) are recorded and mapped. The queue is empty after FrameBegin.
- C3D_SyncTextureCopy / gspWaitForPPF (upload before frame) always has a pending sticky PPF, so it cannot block.
- The vertex and index ring is never reused inside a frame (pm_gpu_rect.c:38-45).

What is left: a replayed entry whose interrupt never comes on hardware. Most likely a PICA lockup inside a replayed command list (no P3D), from GPU state at the start of the right pass that sr_force_all does not rewrite. I could not tie it to a register by code read.

Next step: rule 2 says no fix without a printed wrong value, so I give no fix row. Row ESCAPE (section 2) dumps the stuck queue to crash.txt at the stall: n/cur/last, the id and addresses of each in-flight entry, and the sr state. It also keeps the process alive so the user can escape.

User action for the next XL push:
- Use log on, not prof log=off, so the `stereo: replay` lines show. With 1 or more lines, the first replay was queued and the hang is in the GPU run or the next begin. With 0 lines, the hang is before the replay.
- Move the slider (kgr_01 is the last known spot), wait about 12 s, hold START+SELECT, then pull crash.txt. The dumped entry names the fix.

## At the end
Keep this file (the fix row follows). Hand back "STEREOFX <FAULT SHOWN|NOT REPRODUCED> <hash>".
