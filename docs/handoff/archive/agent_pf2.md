# agent_pf2: row PF2 (sampled frames: phase timers on one drawn frame in N)

## State
- PF2 done (commit: see git log, subject "PF2: prof samples one drawn frame in N").
- Build: sh 3ds/build.sh OK. No warning on changed lines. Linker prints the existing 32-bit enum notice for every object (not new).
- No Azahar start (plan: the test is PF3 only).
- map_restore_table.c: saved to the scratchpad, generator run, file restored (same SHA1 as before). Not committed.

## What the code does
- pm_pf_frame (pm_phase.c) counts the sampled and clean frames. Every drawn frame with prof on and phase off counts.
- After a clean frame, pf_cd counts down. At 0, pf_arm() sets pm_ph_on = 1 for the next drawn frame. The phase stack is reset (last, frame_t0, acc[], depth, cur).
- pm_ph_frame_end: for a sampled frame, pf_take(r) adds PH_GBI, PH_TEX, PH_GPU ms and the dlc delta, then returns (no phase window row, no ps_step, no summarize).
- pm_pf_frame, next call: the sampled frame's work (t1 - t0) goes to wks, its pm_ph_on goes to 0, pf_cd = N - 1.
- The wall of a frame is counted at the next call (same as PF1). The prev-sample flag selects the fts or ft sum.
- Window line: prof line format unchanged. s, fts, wks, oh, amort, gbi, dlc, tex, gpu now fill in. pmu fields print 0 (PF3).
- oh = wks - wk (ms). amort = oh x 1000 / N (us per drawn frame). Base cost (arm and edge reads) is not in amort until PF3.
- pm_gbi.c: the opsplit, mdc, sp0 report block runs only when pm_ph_on && !pm_pf_arm. In phase-on mode (pm_pf_arm 0), no change.
- pm_gbi_sp0_dlc() returns sp0_T[2] (system ticks). Map load resets sp0_T; pf_take treats a lower reading as a reset.

## Trace checks (read only)
- tc_on (pm_tex.c:815) is cached on the first pm_tex_begin_frame call. That is the first drawn frame, before the first sample (drawn frame N). It stays 0 with prof only.
- pm_au_tk (pm_os.c:151) reads pm_ph_on. The flip happens after pm_audio_join_at(3), so no audio job reads it mid-flip.
- pm_gbi.c:3354 and pm_nusys.c:80 take the phase path only in sampled frames. Same pictures as the phase switch (plan 2.4).

## Open items
- oh and amort are not measured yet. The first Azahar value comes with PF3.
- Edge case: a skipped step between arm and the sampled draw extends the sample wall. Not counted separately.

## Commands
- python 3ds/tools/gen_map_restore.py; sh 3ds/build.sh; restore map_restore_table.c from a saved copy.
