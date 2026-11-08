# Row MAPX: door and map-change stutter (oracle plan 2026-11-08, nds1 98e76eff1)

## User facts
"entering and exiting buildings can be kind of laggy. Seems to happen with the fade in / fade out effect that plays. Entering and exiting new maps can also be a bit laggy ... Since we're all in RAM this should not produce any stuttering." Console, wide mode. No console data yet (prof windows drop at a scene change; the tour skips 60 settle frames). The user wants the code written by Opus right after the plan.

## Start state
Worktree build3ds/wt/mapx, branch mapx from nds1 (current). Copy assets/us and ver/us/build into it. PERF2 runs in parallel and changes pm_gpu.c (submit, NdMEnt/NdDraw layout, sprites) and nd_ent.c; PROF2 changes pm_phase.c. Merge nds1 at each step boundary; keep your pm_gpu.c change to the model key and the warm budget.

## What the code does (oracle trace)
- Map change (src/state_map_transitions.c): CHANGE_MAP_INIT fade out to alpha 255 (:134-142) -> DELAY -> LOAD_MAP (4 black frames, load_map_by_IDs in one frame, :150-168) -> AWAIT_MAIN (2 frames, :171-181) -> FADE_IN (:184). Load frame: __wrap_load_map_by_IDs (pm_map.c:568-584) pm_walk_drain + pm_audio_drain(1) (pm_au2_sync -> wait_jobs, pm_au2.c:144-150); nuPiReadRom (pm_os.c:429-460) copy + swaps; decode_yay0 (pm_map.c:360-391) on the game thread for shape (up to 0x40000), hit, textures, each followed by pm_tex_invalidate (tex_invalidate_body pm_tex.c:1120-1160 bumps pm_tex_epoch_v every time, scans slots) and fix_shape + pm_dlc_gen bump (:380).
- First world frame: pm_dlc_gen change -> dlc_reset, pm_gpu_gstat_reset (nd_shape.c:353), ent_reset (nd_ent.c:234), model key change (pm_gpu.c:2708); then in ONE frame: dlc_build for every shape list, model_draw for every model, ent_compile for every entity list, every texture decode and VRAM upload, sprite raster loads. Azahar wcen_mac01 at map entry: one frame gbi 48.1 ms, tex 22.2 ms (texmiss 60, 418 KB).
- Door (room change, same map): walls swing or fade; the model cache key copies the record words (memcpy(k->w, rec, NDMC_RW), pm_gpu.c ~2696) plus prim, prim_lod, cconst, mix colour, so a fading wall misses every frame (model_draw + new entry + pool draws, nd_model.c:476, 506) and at 75% fill mc_clear_all empties the cache. Console log shows key churn (ndmdiff w28, w20; ndm miss 1432 new 683 at f1500).

## Steps (premise check first; old path deleted in the same commit)
1. MEASURE. One line per transition in prof mode, no per-op ticks: start = first frame with gMapTransitionState not idle in GAME_MODE_CHANGE_MAP, or the front overlay alpha (ScreenOverlays[0], screen_overlays.c:14) leaving 0; end = 30 frames after the first world frame of the new map, or 30 frames after the alpha is back at 0. Format: `xfer <door|map> <from>-><to> f<frames> max <ms> o34 <n> | fade <ms> load <ms> rom <n>/<KB> yay0 <n>/<KB>/<ms> inv <n> au <ms> init <ms> | dlc <n>/<ms> mdl <miss>/<clr> ent <n> tex <n>/<KB>/<ms>` (integer math, pm_log_prof_line, under 256 B). Stage ticks once per transition at the stage call. The tour prints one xfer line per warp. bench.py ingest stores kind xfer (from, to). Scenarios + benches (same commit): xfer_door_mac01 (Toad Town house door, det, padscript walk-in, until `xfer door`), xfer_map_mac01 (mac_01 -> mac_02 loading zone, det, until `xfer map`), console benches xfer_door_prof and xfer_map_prof.
2. DOOR KEY. Run the door scenario first; the ndm/ndmdiff lines (mg_cls, key word w<n>) name the changing field (Azahar counts are valid). Colour-only words (prim/env alpha, fog colour) leave the key; on a hit the cached draws get the current colours patched into tev_col/buf_col (recomputed only for that record). A render-mode switch to translucent stays in the key (two entries per door, both stay cached). A key with the same rp and structure replaces its own entry, no insert per frame. Extra gates: spot_mac01_ndmcheck diff 0; door scenario ndm clears 0 and misses about 0 per frame after the first door.
3. WARM FRAMES. Premise: after LOAD_MAP the AWAIT_MAIN and first FADE_IN frames are fully covered (gMapTransitionAlpha 255, black front overlay); check update_enter_map_screen_overlay and one shot burst. While the transition alpha is 255, the walk builds caches with a count budget per frame (deterministic: e.g. 8 dlc lists, 16 model stores, 8 entity compiles, 16 texture decodes); over budget is skipped in that covered frame; the first frame with alpha < 255 builds the rest. Check that det shotmode frames never fall in a covered frame. Extra gates: covered-frame shots black; the first uncovered frame byte-equal to the base.
4. PREFETCH ON CORE 1. Premise: gGameStatusPtr->areaID/mapID hold the target from CHANGE_MAP_INIT; yay0 sources are in the RAM ROM image (pi_read_raw); core 1 has room. At CHANGE_MAP_INIT post a job to a new core-1 prefetch thread (priority below audio, pm_audio.c threadCreate pattern) that decodes <map>_shape and _hit into side buffers; decode_yay0 takes a ready buffer with the same source and length (memcpy) then fix_shape/invalidate/gen bump as before; not finished -> decode on the game thread as today (join first). Same bytes: det equal. Gate extra: xfer yay0 ms.
5. SMALL. (a) tex_invalidate_body bumps pm_tex_epoch_v (pm_tex.c:1121) only when a slot was hit (today every ROM read, sprite raster reads included, resets the tsz_last skip, nd_tex_set.c:198). (b) pm_audio_drain(1) on map load: if au > 2 ms, keep only the job join (check au2_exec reads no RAM the load overwrites). (c) pm_walk_drain calls stay.
6. TEXTURE UPLOADS: only if the console tex field is still > 5 ms: spread VRAM uploads over frames (count budget, same warm rule).

## Done criteria
Every step: build, nd_host PASS, byte-equal on spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det 0000-0015; tour10_det prints 10 windows and 10 xfer lines. Last: the user pushes xfer_door_prof and xfer_map_prof (prof=30) to the O3DS XL. Pass: no frame over 50 ms in either transition, o34 0 outside the load frame. Azahar timing never drops a fix; the console decides.
Affected scenarios (no others): the 4 byte gates, tour10_det, spot_mac01_ndmcheck, xfer_door_mac01, xfer_map_mac01.

## Rules
CLAUDE.md rules 5, 7, 9, 11, docs/handoff/hw_guidelines.md. PM_LANE=mapx PM_ROW=mapx on every build.sh and bench call. No git stash. No toggles, no stubs. src/ changes under #ifdef TARGET_3DS. Commit after each passed step.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=mapx PM_LANE=mapx sh 3ds/build.sh; restore 3ds/src/map_restore_table.c.
- Test: PM_ROW=mapx PM_LANE=mapx python 3ds/tools/bench.py run <bench> --row mapx --lane mapx --why "<text>"
- Budget reset: powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget mapx

## At the end
Delete this row file and .wt_keep in the last commit. SendMessage main "mapx ready <hash>".
