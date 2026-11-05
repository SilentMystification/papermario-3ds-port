# STEREOR: right eye garble on pause and transitions (diagnosis)

Scope: read only. Source: commit dc760f5c2 (stereo second pass). No source edits. No Azahar start.

## Mechanism of the second pass

- pm_nusys.c:81 (`gbi_task`): each task runs `pm_gbi_run` twice. Pass 2 is `pm_gpu_eye(1)`, then `pm_gbi_run`, then `pm_gpu_eye(0)`. The two passes share all statics. Only `pm_gpu_begin_wait` resets them (pm_gpu.c:1809-1858).
- pm_gpu.c:1655-1665 (`pm_gpu_eye`): flushes, ends any off-screen pass, clears `targetR` once per frame if `r_wantclear`, switches `cur_target`, sets `pm_eye_e`, calls `pm_gbi_eye_changed`.

## Candidates

| # | Cause | file:line | Status |
|---|---|---|---|
| 1 | One-shot per-frame state is not per pass. `grab_split`, `grab_n`, `grab_dirty`, `kick_done`, `draws_frame`, `want_clear` are set once per frame. Pass 1 uses them, pass 2 sees the leftovers. | pm_gpu.c:1815-1819 (reset), 314-316 (split clear), 577 (kick clear), 1729-1731 (grab clear), 1882 (end clear) | CONFIRMED (code). Visual effect INFERRED. |
| 2 | Grab reuse across passes. Pass 2 returns the pass-1 copy when no draw dirtied the rows. The right eye then samples left-eye pixels. | pm_gpu.c:1717-1718 | CONFIRMED (code). Effect INFERRED. |
| 3 | One shared copy block. `copy_get` returns one block. The previous-frame copy (queued first in `begin_wait`) and every in-frame grab (T2) write the same memory. Pass-2 draws that read the previous-frame background see the pass-1 grab data. Water, crystal-ball and bulb textures (T2) and the pause background (T3) share it. | pm_gpu.c:355-363, 1690-1695 (copy queued from `cur_target`), 1850 (prev copy in `begin_wait`) | CONFIRMED (structure). Effect INFERRED. This is the best candidate for garbled right-eye texture content. |
| 4 | Pause background is left-eye only. `pm_gpu_read_frame` reads `target` (left). The prev-frame copy reads `cur_target`, which is left at frame start. Right eye shows the left-eye background with no shift. | pm_gpu.c:1530 (CPU read of `target`); pm_main.c:242 (prev-frame source) | CONFIRMED (code). Effect: no parallax, not garble. |
| 5 | Record replay ignores the eye. The key holds slot, version, texture generation and `gen`. No eye term. A record captured in pass 1 replays in pass 2 with the left matrices. Captured uniforms include `shader_3d`, which is eye-specific. | pm_gbi.c:3431-3441 (key), 3669 (`rec_try` call); pm_gpu.c:920-921 (uniforms sent under capture), 982 (`shader_3d` upload) | CONFIRMED (key has no eye). Effect INFERRED: wrong depth for recorded models, not garble. |
| 6 | Raw palette path bypasses the shift in `gv_post`. `pcomp` = `gv_post` x `pal_tab`. `gv_post` uses `projection` (no shift). `pal_tab` = `gm_tab` = `combined` (has the shift). Raw and CPU paths disagree on position. | pm_gpu.c:208-211 (`gv_post` from `projection`), 214 (`shader_3d` with `game_mvp`), 796-801 (`pcomp_calc`); pm_gbi.c:1409-1417 (`gm_tab` entry = `combined`) | CONFIRMED (code). Effect INFERRED: draws on the two paths land at different x. Not pause-specific. |
| 7 | Matrix stack and `gm_tab` carry across tasks in a frame. The reset block is in `pm_gbi_end_frame` (per frame), not per task, despite the PT70 comment. Pass 2 starts from pass 1 end state. Fine for balanced DLs, wrong for DLs that leave pushes or loads. | pm_gbi.c:2286-2288 (reset, inside `pm_gbi_end_frame` which starts at 2234), 2303 (init) | CONFIRMED (where reset runs). Effect INFERRED. |
| 8 | Interp (fps60) replacements (`pm_interp_repl`, `rtab`) are not consumed per walk. Pass 2 reads the same table. | pm_interp.c (`pm_interp_find`, `pm_interp_lerp`) | Checked, not a cause. |
| 9 | Texture staging. Upload runs once per frame in `begin_wait`. Staging frees happen in `pm_tex_begin_frame`. Pass 2 hits the same slots. | pm_tex.c:543-549, 598-600 | Not traced to a failure. Not a per-pass cause (INFERRED). |

Not checked in this pass: the fade, wipe and star transition code, `dlc` self-modifying-DL handling, and the scissor and stencil state per pass. These need a second look.

## Azahar

Not started. Three reasons:
1. No pause scenario with stereo on exists in 3ds/tools/azt_scenarios.json. The only stereo scenarios are yard_stereo_l and yard_stereo_r (unverified, yard only).
2. azt.ps1 has no debug override, so a pause and stereo run needs a new scenario. Rule 5 requires a new scenario to come with its catalog row in one commit.
3. An azahar.exe process (PID 26372) was running at check time. It was not started by this agent and was not stopped.

## What settles it

- Add a scenario `pause_stereo` (pause as in `pause_menu`, debug adds `wide stereo shoteye`). Read the right eye shot at pause open. Compare with the left eye.
- Diagnostic switch (not in this pass): reset `grab_split`, `grab_n`, `grab_dirty`, `want_clear` and `mv_sp` at the start of each pass. If the right eye garble goes away, cause 1 or 7 is confirmed. If it stays, test cause 3 by giving pass 2 its own copy block.
- Log line per pass: grab return code (1 reuse, 2 copy), prev-frame path (GPU or CPU) and `mv_sp` at pass start. One shot each.
