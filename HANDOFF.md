# Handoff (short, read in full by every agent)

RULE: Each slice adds at most 3 lines to this file. Put detail in docs/HISTORY.md (append at the end).
Pointers: docs/HISTORY.md (all old slice results), docs/SLICES_DONE.md (done rows), docs/INDEX.md (where things are), 3ds/ORCHESTRATOR.md (workflow, reading rules, model rule), 3ds/SLICES.md (open queue), 3ds/QUESTIONS.md (open questions).

## RULE: NO STUBS WITHOUT ASKING
- Do not add a stub. Do not write a stand-in for decomp code or data. Build the real decomp file.
- A port-layer shim is allowed only for real N64 OS or hardware functions that hold no game logic.
- If a stub looks like the only option, stop. Write `BLOCKED: <reason>` here. Name the unported system that forces it. Ask the user. Wait.
- Top priority: remove existing stubs (list at the end of this file).
- Fix root causes. Never hide a fault with a guard (the `spriteIdx < 0` guard of S22 was removed).

## Rules the project depends on (Do not)
- Do not push to git. Add no Claude or Anthropic attribution to commits or PRs. Commit locally, plain English, only your own files.
- Do not run `git checkout` on tracked files. Do not commit logs, traces or shots. They go under `3ds/logs/` (git-ignored). Only `3ds/PROGRESS.log` is tracked.
- Do not start `azahar.exe` with any argument except the .3dsx path (a modal dialog blocks the run). Use `3ds/run_azahar.ps1`, not `go.ps1`. Only one Azahar window can run.
- Do not leave `fast` in `debug3ds.txt` (it skips the logos). The build skips when the 3dsx is newer than all sources: touch a source file after a CMakeLists.txt change.
- Testing: feature tests use the warp (`preset=` or `warp=<map>:<entry>`) with `-Turbo -Smoke -FaultStop` and a fixed length. Never wait open ended. Add `padscript=` where input is needed.
- Lighter check rule (2026-10): after a fix run (1) the smoke check on presets `yard` and `toad` with `-Turbo -Smoke` (about 16 s each), (2) shots of only the scenes the change touches. No 4-scene A/B and no full tour, unless the slice touches the map change path, boot, title, file select or the intro. Then run the full normal chain `-Debug "fast autoa freshsave" -Stall 40`. A by-eye A/B with a `nocull` style switch stays allowed to prove that the symptom is fixed.
- NARROW TESTS (2026-10-18): test only what the change touches. Use one target scene or map, a fixed short length and one text check (log line, draw count or one pixel diff). Do not build large test setups. Do not skip a fault the narrow test finds: fix it. Say in the result which areas were not tested. Build or run checks per slice must be short.
- Tours: iterate on the failing set. Run the full tour once at the end of a slice. Say in the result which kind of check you used.
- Threading and sync use libctru natives (threadCreate, LightLock, LightEvent, LightSemaphore, svc handles). No hand-rolled locks. No sleep loops. The custom yield in `pm_os.c` (`pm_au_thread_yield`) and `pm_audio.c` is to be reviewed in the audio slice (AU1).
- Memory rule: the estimated O3DS spare is about 4.7 MB with all 421 maps (docs/MEMORY_BUDGET.md). Add at most about 10 KB of code or data per slice. Exception: S30b may add about 2 MB of static image (user approved). Write the size change (3dsx bytes) in the slice result.
- Screenshot rule: use screenshots only when really needed. Prefer text evidence (draw logs, depth view, checksums, pixel diffs, summary scripts). When a picture is needed, make ONE contact sheet.
- Log rule: each agent appends one line per step to `3ds/PROGRESS.log` (`HH:MM ID text`). Read logs with the last 40 lines only. Game log: `%APPDATA%\Azahar\sdmc\3ds\PaperMario\log.txt`. The log is non-blocking (L01). Never commit it.
- Notes and replies use ASD-STE100 English: short sentences, one instruction each, active voice, no semicolons, no praise.
- Reference ports in `reference/` (paperboat first, soh-3ds and sm64-3ds for 3DS GPU, audio, memory): read to learn the method. Never copy a hand-written stand-in.

## Current state (2026-10-17)
- Branch `main`. Tag `s19-pre-merge` keeps the old tip of branch `s19-full-battle` (merged by M03). No push.
- The game boots in Azahar: logos, title, file select, intro (hos_05), kmr_20, osr_00, mac_00, mac_01, kkj_00 (the real chain). The real battle tree is merged. Real encounters, level up, flee, game over and the title demo loop (17 scenes, repeats after MS01) run.
- All 421 maps are linked (S30b). 3ds/maps_skip.txt is empty. 6 new maps warped in Azahar, the other 148 not.
- Image about 16.98 MB. 3dsx about 10.9 MB. Estimated O3DS spare about 518 KB. This is an estimate from Azahar numbers (64 MB limit assumed, 20 MB linear pool, N64 RAM block 4 MB).
- The O3DS and the N3DS were never run. S28 is the first hardware run. All speed numbers are Azahar numbers and are not valid for the O3DS.
- The game runs at exactly 30 fps (PC01: 29.91 steps per second, 2 vblanks per step). 60 fps (F06) and widescreen (F07) come later.
- Fixed in the platform layer (details in docs/HISTORY.md): combiner (S25), transparency (S26), file select (S27), screen transitions (X01), brown wall by culling (V02), menu flip (V05), HUD (H04), pause (D03), V06 blender mix.
- Port layer files and tools: see docs/INDEX.md.
- Golden frames (G01g): `python 3ds/tools/golden.py [--update] [scene..]` runs 10 scenes (title, fileselect, intro, toad, yard, battle, pause1 to 4), prints a table of pixel differences against `3ds/golden/*.png`. About 25 s per scene. Run it after a graphics change. Update the references only after a change that you checked by eye.
- H03: linear pool 14 MB (was 20 MB), boot line `mem split H03`. A full texture cache skips the draw (no white quad). Detail in docs/HISTORY.md.
- V08 done: the save block is translucent by design (decomp XLU, prim alpha 128). The port draws it exact. No code change.
- Last commits: V05 (menu flip, scissor), V06 (fog colour mix).

## Debug switches (put in `debug3ds.txt` or `-Debug "..."`). One line per group, details in docs/INDEX.md
- Start: `fast autoa freshsave loadslot=<n>`. Warp: `warp=<map>:<entry> story=<name|n> preset=<name> partner=<n> sp=<n> hp=<n> battle[=<f>:<s>] battlef=<n> demobattle`.
- Run control: `smoke smokeframes=<n> tour=@file tourfx=<n> turbo padscript=<..|@file> padrand padlog taptest`.
- Pictures: `shots shotwin=<f>:<n> shotmode=<m>:<n> shotskip=<n> offdump`.
- Draw A/B and trace: `nocomb nocull nomix nooff noprimz rmoff rmlog drawlog=<f>:<n> ddonly=<lo>:<hi> ddverb depthview=<1|2|3> xtrace=<f>:<n> prevtest nofallback`.
- Checks: `ptrcheck ptrtest romstrict romtrace romfull dataprobe assets scripts faulttest crashtest crashtest_call crashtest_audio crashtest_audio_call`.
- Timing and log: `phase phasecsv phasegbi stats audiostat pacestat logstat lognull logflood noaudio`.
- Presets: intro yard toad toad_east toad_west osr kmr02.
- Run script `3ds/run_azahar.ps1`: `-Until -Timeout -Stall -Turbo -TurboFactor -TurboLimit -Smoke -FaultStop -Capture -Gdb -GdbScript -Interactive`. Exit 0 ok, 1 failed, 2 unmapped access or crash. Main command: `powershell -File 3ds/run_azahar.ps1 -Turbo -Smoke -FaultStop -Debug "fast preset=toad"`.
- Build: `3ds/build.sh` and `3ds/go.ps1` (build glue). Hardware: skills `push-o`, `push-n`, `pull-o`, `pull-n`, `push-a`.

## Open issues (one line each, row id in 3ds/SLICES.md)
- V06 (queued, after X02): user confirmed the ground gap in buildings is correct. Real fault: the outside must be black in a building room, the port draws the normal background.
- V07 (SHELVED by user 2026-10-18): items flicker. Not reproduced in Azahar. The user reports it again if seen. Do not work on it.
- V09 (two rows): intro tints changed by S25 (and nok_01 tour fault). kmr_22 card shows a rainbow bar.
- V10: battle Strategies box and level-up box draw as a checkerboard.
- V09 intro tints and nok_01 shelved by the user, partial edits on branch shelf/v09-nok01.
- X02 done (B0 of docs/plans/buffer_bridge.md): darkness runs the real decomp function with 64x64 slots keyed by (address, pixel size). kzn_08 untested (not linked). Detail in docs/HISTORY.md.
- F03: `is_point_visible` reads a NULL z-buffer (hos_05 lamp, `tex ptr 0000fd7e`).
- F08: fog partly done by V06 (blend and fog colour mix). The per-vertex fog factor is open.
- S30b done: all 421 maps linked, 3dsx 13,011,156 B (+2.09 MB), GotoMap guard removed. Spare about 4.7 MB by estimate (docs/MEMORY_BUDGET.md).
- S31: crash screen and debug output check (Q5).
- T02 Phase 1 done: run_azahar.ps1 holds mutex `Global\pm_azahar_run` (-LockWait s, exit 3 on timeout), restores qt-config.ini byte for byte in finally, and recovers after a hard kill. Lanes (Phase 2, 3) are open in docs/PARALLEL_PLAN.md.
- F01 tst_12: cause measured (LVar5 = -17, LVarF = -11, endless Loop, N64-identical decomp defect of a cut map, no port fix). Skip tst_12 in tours. Detail in docs/HISTORY.md.
- Tour faults: kmr_23, nok_01, trd_06 (falling.c:148) and partners.c:1136 fixed by F01 (lane B, tour-only: a tour now uses partner 1). Detail in docs/HISTORY.md.
- Not verified by eye: partner moves, items and star power in battle. Pause with items or a partner. Hardware.
- F09 language select: future feature, not in the current batch.
- PO3 RISK (RSP to PICA port): effects that read back N64 memory images (X02, F03, water_splash, grabs) may need rework. Check each at every PO3 slice (docs/plans/PO3.md R7).
- Buffer bridge plan: docs/plans/buffer_bridge.md (B0 is X02)
- S28 only after all Azahar-verifiable rows are done (user decision 2026-10-18).

## Stand-ins still open (checked against docs/HISTORY.md, S20 audit A, B, C, D and later results)
- Placeholder aliases (class D, code only, no data read): 47 `--defsym X=gPartnerStatus` in `3ds/CMakeLists.txt` (`world_partner_*`, `world_use_item_*`, `pause_*`, `filemenu_*`). Code overlays linked in place. Verified in P01 and R01.
- `((u8*)1)` ROM ranges for linked code overlays in `3ds/stub_include/ld_addrs.h`, `effect_ld_addrs.h`, `battle_ld_addrs.h`. Code only.
- GotoMap guard: removed by S30b.
- Empty shims: `crash_screen_init`, `is_debug_init`, `load_obfuscation_shims` (`3ds/src/game_stubs.c`), `crash_screen_set_draw_info` (`3ds/src/pm_later.c`). User decision Q5: obfuscation shims stay empty. Row S31 checks the crash screen.
- Storage at real N64 addresses (class C, no game logic): `gBackgroundImage` and `pm_world_entity_heap` (`3ds/src/pm_kmr_stubs.c`).
- Gone (do not look for them): `area_table.c`, `pm_title.c`, `pm_evt_stub.c`, `pm_actor_link.c`, `pm_shadow.c`, `pm_fx.c`, battle stand-ins, effect graphics zeros (A17), shadow headers (B3 to B6), the `spriteIdx` guard, `note_door`.
- Not stand-ins but still approximate: fixed light direction for `G_LIGHTING` vertices (`pm_gbi.c`), no fog table (F08), no texture generation, LOD or `G_BRANCH_Z`.
- Lane B (worktree ..\papermario-3ds-port-laneB, branch lane-b, `PM_LANE=laneB`): see docs/LANES.md. Build parallel, run one at a time.

- S18 and S19c closed (lane B copy run via PM_LANE=s18chk scratch lane). poly_scan.py thresholds too loose (flags natural dirt): tune before reuse.
