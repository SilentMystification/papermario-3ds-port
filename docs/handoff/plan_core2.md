# Plan CORE2: more work off the game thread (oracle, 2026-11-08, nds1 71a457148)

## Today
- Core 0: game thread (pm_main.c:832): walk, geometry, GPU submit, texture decode, game logic.
- Core 1 (system core, APT_SetAppCpuTimeLimit 80, retry 79, 30 on HOME/sleep, switch cpulimit=; pm_log.c:1203-1216): audio control (pm_audio.c:112), audio mixer (pm_au2.c:177), log writer (pm_log.c:996), log sink (pm_log.c:1231), flash/save (pm_flash.c:86), watchdog (pm_wdog.c:154).
- Core -2 (default): log step thread (pm_log.c:1071); audio, flash and the log writer fall back here when core 1 is refused.
- N3DS: osSetSpeedupEnable(true) (pm_main.c:769); no thread on core 2.
- Console cost (O3DS XL, runs 25158/25044): mac_01 ft 32.4-32.6 ms, gbi 29.7-36.4, oh 7.9-12.5; logic about 3.3 ms (frame_budget.md). The game thread is walk-bound.

## Candidates (ranked by console gain against risk)
1. Audio one step ahead. The game thread waits in pm_audio_join 1-4 times per step (pm_main.c:599, pm_nusys.c:157/163, pm_phase.c:754); core 1 is time-limited and shared. Kick step N+1, join step N only when its ring slot is needed (AU2 ring, au_start/au_done events exist). Det keeps 2 ticks per step. Ring indices on separate 32-byte lines.
2. Walker thread on N3DS core 2: game logic of frame N+1 in parallel with the walk of frame N. The walker owns nd_ws, pm_gpu, pm_tex, the model cache, C3D and the GX queue; the game thread calls no pm_gpu_* (HUD, menu present, GREY, shots move to the walker). Handoff: the finished gfx task (gbi_task, pm_nusys.c) into a 1-deep queue. Det walks in series. Gain up to about 3-12 ms per frame on N3DS. Not on O3DS (walk ~30 ms, core 1 at most 80% shared). Risk high: every game buffer the list points to and the game rewrites in place (effect vertex buffers, ent_dynlist, sprite records, G_PM_SPR reads) must be found first.
3. Texture decode/staging worker: 0.02-0.4 ms per frame steady; helps map-change hitches only. Only if a console hitch measurement points there.
4. Model cache builds, log formatting: no gain (part of the walk; logging is off in prof mode).

## Premise and check
Premise: core 0 is walk-bound and part of its time waits on core 1. Check (0 starts): read pm_audio_join_timed (pm_audio.c:234) and its call sites; look for a join-wait figure or coreprobe output in the last console prof log.

## Rows (in order)
All CORE2 rows: one Opus agent (user, 2026-11-08), worktree core20, no subagents. Console pushes only with user consent.
- CORE2-0 measure (done): the prof writer line ends with `aujoin_us <us per drawn frame> aujoin_max <longest single join, us> core1_busy <% or n/a>` (pm_audio.c au_pf_add, pm_phase.c c1_main; bench keys aujoin_us, aujoin_max, core1_busy). The probe thread exists in prof mode only. Console run still owed (user consent): spot_mac01_prof and spot_iwa10_prof on the O3DS, then the same on the N3DS; read aujoin_us against ft_ms and core1_busy. CORE2-1 is worth its risk only when aujoin_us is a visible part of ft_ms; core1_busy near 100 means core 1 has no room for more work. The probe yields after each 20 us chunk; if the kernel counts its time against the app core-1 limit, it can slow the audio job: one run with and one without prof shows that through ft_ms.
- CORE2-1 audio one step ahead. Gate: det byte gates, audiohash equal under det, console ft gain.
- CORE2-2 design document for the walker thread on N3DS core 2 (ownership, list of in-place rewritten buffers).
- CORE2-3 implementation.
- CORE2-4 texture decode worker, only if CORE2-0 shows hitches.
Proof of any gain: console prof only (spot_mac01_prof, spot_iwa10_prof on O3DS; N3DS for CORE2-2/3).
