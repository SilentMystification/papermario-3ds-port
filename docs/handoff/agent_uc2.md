# agent_uc2 handoff (2026-11-01)

## State
- Log-only stencil probe in 3ds/src/pm_gpu.c: `stc_set(site, en, fn, ref, im, wm)` wraps C3D_StencilTest at the six call sites (sites 1 to 6 = lines 811, 816, 821, 832, 1000, 1010 before the edit). Switch `stprobe`. Logs `UC2P site ...` lines, first call per site and each change of the site's word, max 200 lines. Draw output unchanged.
- Word logged = `im<<24 | ref<<16 | wm<<8 | (fn<<4)&0x70 | en`, computed from the args (encoding in diag_uc2_0x110.md section 3). It is not read from the citro3d context.
- Scenario `yard_stc` (3ds/tools/azt_scenarios.json, status unverified, debug adds `stprobe`). Not yet in docs/handoff/test_catalog.md. Both files hold uncommitted changes from other work, so this scenario entry is not committed.

## Runs (row UC2)
- Start 7: unplanned. yard_stc was started without -Build and without a justification line, so it ran the default build3ds\pm_3ds.3dsx (no probe). Result not used.
- Start 8: lane UC2 build (pre-fix filter, see below). Log: 3ds/logs/perf (Azahar game log %APPDATA%\Azahar\sdmc\3ds\PaperMario\log.txt, 200 UC2P lines, frames 35 to 91).

## Logged arguments (start 8)
- Site 1 (811, decal2 pass 1): en 1, ALWAYS, ref 1, im 0xff, wm 0xff -> word 0xFF01FF11
- Site 2 (816, decal2 pass 2): en 1, EQUAL, ref 1, im 0xff, wm 0xff -> word 0xFF01FF21
- Site 3 (821, decal2 end): en 0, ALWAYS, ref 0, im 0xff, wm 0x00 -> word 0xFF000010
- Sites 4, 5, 6 (832, 1000, 1010): not reached in yard within frames 35 to 91 (the cap of 200 lines was hit at frame 91). Not logged.
- No port call stores 0x00000110 or 0x00000000. Frame value 0x110 has no source among the logged sites.

## Open items
- The filter bug (logged on every call, cap reached at frame 91) is fixed in pm_gpu.c after start 8. The fixed build compiles (build rc 0). It is not run.
- The source of the 0x110 at ctx+0xF4 is not found. Either the context offset assumption in diag_uc2_0x110.md is wrong, or a write comes from a path that does not call C3D_StencilTest.
- Stencil site 4, 5 and 6 need a run with a window at frame 349 or a model draw (r_zmark path) to log them.

## Next steps
1. Investigation (fresh agent, not code): disassemble C3Di_UpdateContext in libcitro3d.a and check the words it writes to 0x104 to 0x107 (diag_uc2_0x110.md section 6, step 1).
2. Code agent: commit the stencil probe with the scenario and catalog row when the scenario JSON and catalog are clean, or after the orchestrator resolves the other uncommitted changes.
3. Budget: about 2 starts left for row UC2 (starts 7 and 8 used).

## Commands
- Build: `PM_LANE=UC2 PM_ROW=UC2 sh 3ds/build.sh` (Git Bash, repo root). Output build3ds/lane-UC2/pm_3ds.3dsx.
- Run: `PM_ROW=UC2 powershell -File 3ds/tools/azt.ps1 yard_stc -Row UC2 -Build UC2 -Try`
- Not run: 3ds/tools/gen_map_restore.py. It writes 3ds/src/map_restore_table.c, which this task forbids. The build does not call it. map_restore.txt has no change in git status.
