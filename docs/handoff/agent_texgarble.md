# Handoff: wall texture garbling (open work only)

Written for: the next code agent, started by the orchestrator. Date 2026-11-03, branch uc2-percall.

## Task
The user sees garbled terrain wall textures on the console. It happens in many maps. Two places are named:
1. The map after Goomba King and before Toad Town. A band along the top of a low terrain wall (right of Mario, next to a `?` block and terraced cliffs) shows vertical stripes, as if the top texture rows were smeared down. The rest of that wall looks fine.
2. hos_01 (Shooting Star Summit area). The wall is animated (the texture scrolls) and the garbling is visible while it scrolls.

The user describes it as "scrambled or noisy, fixed place, occasional, many maps". The band in 1 reproduces in Azahar (screenshot of the live window, user confirmed it is the garbling). Seen only once, so a legitimate N64 stretch on a steep wall is not ruled out. Compare with the same wall from another angle before a fix.

## What is known
- Azahar shot of `tex_kmr_11` (warp kmr_11:0) does not show the band at the entry view. The user walked to the spot. The map name is not confirmed (kmr_11 is a guess).
- Maps already rendered at entry 0 (no match to the user's view): kmr_02, 04, 06, 07, 09, 10, 11, 12. kmr_05 entry 0 crashes in `SpeakToPlayer` (msg_api.c:21, null deref, 299 unmapped reads per 300 frames) with `story=ch0_toad_town`. Probably a forced-story artifact, not a renderer bug. kmr_05 entry 1 loads.
- Two candidate cause groups (none tested):
  - Texture UV or tile wrap for the scrolling and clamped tiles (`wrap_of`, `pm_tex_set_wrap`, `mk_ws/mk_wt` in 3ds/src/pm_tex.c lines 120-140 and 268-280), or the dual texture path (TEXEL1, `expand_dual`, `t1_fold`) which only the CPU path takes.
  - Console-only timing in the VRAM upload (`upload_vram_slot` queues a DMA, `drop()` frees the staging buffer at once, `pm_tex_upload_vram_before_frame` runs before `C3D_FrameBegin`). Azahar hides this. Review text: docs/handoff/review_tex_cache.md.
- Azahar runs deterministic: two builds gave byte-equal shots of mac_01 and yard.

## Tools added this session
- Scenarios (3ds/tools/azt_scenarios.json, unverified): `tex_kmr_02/04/05/06/07/09/10/11/12`, `tex_hos_01` (shots 200 to 239). `tex_kmr_11` has `drawlog=200:1`. Catalog rows for them are not written yet (rule: add to docs/handoff/test_catalog.md).
- `drawlog=file` (3ds/src/pm_gbi.c, `dd_on` and `dd_end`, UNCOMMITTED, mixed in the working tree with another agent's UC2 edits in the same file, hunks at about lines 99-125 and 168-175). It logs one line per mesh chunk (`dd f.. #idx src dl geom oml omh tex img fmt siz WxH tri scr x0 y0 x1 y1 w0 w1`) for the frame after the file `%APPDATA%/Azahar/sdmc/3ds/PaperMario/drawlog.now` appears. Note: `dd_on` forces the CPU path (no raw path), so the garble may change with it on. Compare the picture with and without.
- `ddonly=<lo>:<hi>` (existing) draws only chunks lo..hi inside a drawlog window, for bisecting.
- `seamstat` (committed) logs CPU path route counts.

## Commits this session (all on uc2-percall)
- 789649bef wide: zoom the original 296x200 view to 240 rows, sky scaled, status bar at the top edge.
- dc760f5c2 stereo 3D: second pass per task for the right eye, depth shift in the projection, mono at slider 0.
- 617540255 seam: CPU path triangles take object-space positions through the same post x game matrix as the raw path.
- Not committed: drawlog=file, the new scenarios (committed together with this file), 3ds/src/map_restore_table.c (never committed).

## Open console checks (only the user can do them)
- Seams: look at the Toad Town building by the gate and the Toad house in Goomba Village (fix 617540255).
- Stereo: slider up, check both eyes, tune `PM_EYE_E` (0.015) in 3ds/src/pm_gpu.c. The sky has no parallax.
- Zoom: status bar position in battle and the wave sky are unchecked.
- The last N3DS push failed (`push_3ds.sh` exit 1, no log). Check the console is reachable at 192.168.1.223 before the next push.

## Next steps
1. Find the exact map and spot. Start `go.ps1 push a -NoBuild -Interactive -GameDebug "warp=<map>:<entry> loglevel=debug drawlog=file"`, let the user walk there, create `drawlog.now`, read the `dd` lines in `%APPDATA%/Azahar/sdmc/3ds/PaperMario/log.txt`, and take a screenshot of the window at the same moment.
2. Match the band (game screen coordinates) to the `dd` line whose `scr` box covers it. Read `img fmt siz WxH` and the wrap state of that chunk. Then `ddonly` to confirm which chunk draws it.
3. Compare Azahar with and without `drawlog`. If the band vanishes with it, the raw path (3ds/src/pm_gpu.c `pm_gpu_gtrip`) is the cause. If it stays, look at the tile and wrap code first.
4. For hos_01, take frames 200 to 239 (`tex_hos_01`) and diff neighbours at the animated wall. The first look showed vertical streaks on the green rock wall, not yet judged.
5. Fix at the shared point (renderer, not per map). Test on the named maps. Then the console decides.

## Commands
- Scenario: `PM_ROW=TEXGARBLE powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 <scenario> -Row TEXGARBLE -Try`. Write one justification line first to 3ds/logs/perf/status_TEXGARBLE.txt.
- Budget reset (8 starts per row): `powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/run_azahar.ps1 -ResetBudget TEXGARBLE`.
- Build: `cp 3ds/src/map_restore_table.c $TEMP/mrt_user.c; python 3ds/tools/gen_map_restore.py; sh 3ds/build.sh; cp $TEMP/mrt_user.c 3ds/src/map_restore_table.c`.
- Shots are PPM 240 by 400 (rotate 90 degrees to view). A screenshot of the live window: PowerShell `System.Drawing` `CopyFromScreen`.
- Resolve a PC: `MSYS_NO_PATHCONV=1 docker run --rm -v "$(pwd -W)/build3ds":/w devkitpro/devkitarm:latest /opt/devkitpro/devkitARM/bin/arm-none-eabi-addr2line -f -C -p -e /w/pm_3ds.elf <addr>`.
- Pitfall: a Python `'\b'` in a normal string is a backspace. Build regexes with `chr(92)` when writing azt_scenarios.json.
- Azahar allows one instance. Close the interactive window before a scenario run.

## Launch prompt (10 lines)
```
Read docs/handoff/agent_texgarble.md. Task: find and fix the wall texture garbling (kmr map after Goomba King, hos_01).
Do next steps 1 to 3 in order. Azahar is the only tool: one instance, interactive first.
The user walks to the spot. You create drawlog.now, read the dd lines, screenshot the window.
Use ddonly to isolate the chunk. Name the tile, wrap and path (raw or CPU) before any edit.
No stubs, no toggles, no attribution. Fix the shared renderer code, not one map.
Commit only your own hunks (pm_gbi.c holds another agent's UC2 edits). Never commit map_restore_table.c.
Write a status line to 3ds/logs/perf/status_TEXGARBLE.txt before each Azahar start.
Add each new scenario to azt_scenarios.json and the test_catalog.md table in the same commit.
Report in 3 lines. The console check is the user's.
```

## Added 2026-11-03 by the orchestrator: related symptom BGROW
The user also sees a garbled row, 1 pixel tall, in the background image of many scenes on the console. Diagnosis docs/handoff/diag_bgrow.md (commit 20bd51ac1): NOT FOUND in the code. The sky draws as one texture bind (src/background.c:296-309) with no strip seam, the texel row mapping shows no off-by-one (pm_gbi.c:3229-3244, pm_gpu.c:2590-2597, pm_tex.c:1157/1194), and Azahar shows no bad row. So it is console-only like the wall garble. Treat the VRAM upload timing candidate (upload_vram_slot DMA queued, staging freed at once) as the first suspect for both symptoms. A partial or late DMA of one texture row fits a 1 pixel row. Check it first (docs/handoff/review_tex_cache.md), and include the sky texture in the test.
