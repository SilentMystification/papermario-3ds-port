# Investigation: core assignment on O3DS

Scope: read only. No source edits, no Azahar, no commits except this file.
Paths are relative to the repo root C:\Users\Andrew\Documents\GitHub\papermario-3ds-port.

## Result

Core 0 runs the game. Core 1 runs audio, log writer, watchdog, flash and the audio mixer. No game logic, display-list build, gbi walk or GPU call runs on core 1 in the traced code.

| Work | Core | Thread | Evidence |
|---|---|---|---|
| (a) game logic (step_game_loop) | 0 | game_thread | src/main.c:133 (call in gfxRetrace_Callback); 3ds/src/pm_phase.c:510-516 (__wrap_step_game_loop); pm_nusys.c:153-155 (pm_gfx_retrace calls gfx_func); pm_main.c:375, 461 (pm_frame_loop calls pm_gfx_retrace); src/main.c:104 (pm_frame_loop); pm_main.c:636 (boot_main); pm_main.c:682 (game_thread created, core arg 0) |
| (b) display-list build (gfx_task_background, gfx_draw_frame, appendGfx_*) | 0 | game_thread | src/main.c:138-139 (only call sites of gfx_task_background and gfx_draw_frame); src/main_loop.c:158 and 187; pm_phase.c:519-528 (PH_DL wraps); appendGfx_model at src/model.c:1384 is called from the same chain |
| (c) gbi walk (pm_gbi_run) | 0 | game_thread | pm_nusys.c:99-125 (nuGfxTaskStart, called from main_loop.c gfx_task_background and gfx_draw_frame); pm_nusys.c:76-86 (gbi_task); pm_gbi.c:3265 (pm_gbi_run); pm_gfx_bg_flush pm_nusys.c:127 |
| (d) GPU submit, C3D_FrameBegin, C3D_FrameEnd | 0 | game_thread | pm_gpu.c:1671-1700 (pm_gpu_begin_wait, C3D_FrameBegin at line 1700); pm_gpu.c:1718-1727 (pm_gpu_begin, pm_gpu_end); pm_gpu.c:1766 (C3D_FrameEnd, inside pm_gpu_end). Called from pm_main.c:445 and 470, and from gbi_task pm_nusys.c:78 |
| (e) audio control job (nuAuMgr, src/audio/core) | 1 | au_thread | pm_audio.c:103 (threadCreate(..., prio, 1, false)); fallback pm_audio.c:106 (core -2) if the core 1 create fails; src/audio/core/system.c:101-102 (osCreateThread/osStartThread); pm_os.c:111-123 (shim, calls pm_au_thread_start); pm_audio.c:93-114 |
| (e) audio mixer (AU2 render thread) | 1 | rthread | pm_au2.c:169-170 (core = 1, or -2 with switch audiocore0); if the create fails the mixer runs in the audio job (pm_au2.c:171-173) |
| (e) ndsp / DSP firmware | hardware | none in tree | pm_audio.c:41-58 (ndspInit, pm_dspa_init). No thread created in pm_dspaud.c |

## Thread creation sites (all of them in 3ds/src)

Found with a grep for svcCreateThread, threadCreate and threadJoin. Core argument is the 5th argument of threadCreate.

| File:line | Thread | Core arg | Fallback |
|---|---|---|---|
| pm_main.c:682 | game_thread | 0 | none (return 1 on failure, pm_main.c:683) |
| pm_audio.c:103 | au_thread (nuAuMgr) | 1 | pm_audio.c:106, core -2 |
| pm_au2.c:170 | render_main (AU2 mixer) | 1 (-2 with audiocore0) | none, runs in audio job |
| pm_log.c:597 via start_writer | writer_main | 1, then 2 path uses 0 | pm_log.c:760-762: writer core -1, then start_writer(2, 0) |
| pm_log.c:770 | sink_main | writer_core | none |
| pm_log.c:663 | step_main (sleep and exit steps, two slots) | -2 | none |
| pm_flash.c:86 | flash_thread | 1 | pm_flash.c:87, core -2 |
| pm_wdog.c:75 | wd_main (watchdog) | 1 | none (log says "NOT STARTED", pm_wdog.c:76) |
| pm_dsptest.c:167 | test_thread | 1 | only with switch dsptest (pm_audio.c:55) |

## Affinity value meaning

- Core 0 is set explicitly at pm_main.c:682.
- Core 1 is set explicitly at pm_audio.c:103, pm_au2.c:169, pm_log.c:758 (start_writer(1, 1)), pm_flash.c:86, pm_wdog.c:75.
- Core -2: the repo says it is the default core, which is core 0. Source: docs/plans/AU1.md:11 and the comment at pm_audio.c:102. Not confirmed. The libctru header (thread.h) is not in the repo. I did not find it on disk in the time allowed (a search of / timed out).
- writer_core is set from svcGetProcessorID() at pm_log.c:558 (the real core of the writer, not the requested one).
- The game thread logs its own core at pm_log.c log line via svcGetProcessorID (pm_main.c:626). Not read from a live log.

## Core 1 game work

Not found. The three core 1 threads that touch game data are:
- au_thread runs the N64 audio engine (nuAuMgr, src/audio/core). This is game-owned audio code, not game logic. Not checked: whether the audio engine reads game state. The game thread waits at the join points (pm_audio.c:198-212) before it runs game code (join sites: pm_nusys.c:148, 154; pm_phase.c:509; pm_main.c:484, 655).
- The log writer and sink take con_lock (pm_log.c:44, 494) for the bottom screen. The game thread takes the same lock for HUD text. This is a shared lock, not game work. Not confirmed for every HUD call.
- The watchdog (pm_wdog.c) reads progress counters. Not checked line by line.

## Not confirmed

- libctru meaning of core -2 (see above).
- libctru service threads (GSP event, ndsp, APT). The repo has no source for them. docs/plans/topology.md:39 says process default core 0. Not checked.
- Whether writer_main or the flash thread call any function that reads game state. Only the thread entry and the call sites were checked.
- Timing numbers. This report makes no claim about speed.
