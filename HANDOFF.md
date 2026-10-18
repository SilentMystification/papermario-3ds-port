# Handoff (short, read in full by every agent)

RULE: Each slice adds at most 3 lines to this file. Put detail in docs/HISTORY.md (append at the end). Agent rules: 3ds/AGENT_RULES.md.
Pointers: docs/HISTORY.md (old slice results), docs/SLICES_DONE.md (done rows), docs/INDEX.md (where things are), 3ds/ORCHESTRATOR.md (workflow, model rule), 3ds/SLICES.md (open queue), 3ds/QUESTIONS.md (open questions), docs/ORCH_STATE.md (orchestrator resume state), docs/plans/ (plans, long row text), reference/ (paperboat, soh-3ds, sm64-3ds: learn the method, never copy a stand-in).

## RULE: NO STUBS WITHOUT ASKING
- Build the real decomp file. Do not add a stub or stand-in.
- If a stub looks like the only option, write `BLOCKED: <reason>` here. Name the unported system. Ask the user. Wait.
- Top priority: remove existing stubs (list at the end of this file).
- ALL functionality is ported before S28. Later work is hardware, missed fixes and optimization only (3ds/SLICES.md, row S28).
- Fix root causes. Never hide a fault with a guard. The `spriteIdx < 0` guard of S22 was removed.

## Current state (2026-10-17)
- Branch `main`. Tag `s19-pre-merge` keeps the old tip of branch `s19-full-battle` (merged by M03). No push. Lane B: worktree ..\papermario-3ds-port-laneB, branch lane-b, `PM_LANE=laneB` (docs/LANES.md).
- The game boots in Azahar: logos, title, file select, intro (hos_05), kmr_20, osr_00, mac_00, mac_01, kkj_00. The real battle tree is merged. Encounters, level up, flee, game over and the title demo loop (17 scenes, repeats after MS01) run.
- All 421 maps are linked (S30b). 3ds/maps_skip.txt is empty. 6 new maps warped in Azahar. The other 148 are not.
- All 421 maps are linked (merge 93cab18e2). 3dsx is 13,013,472 B. Image is about 18.9 MB. Estimated O3DS spare is about 4.7 MB with all maps (docs/MEMORY_BUDGET.md, 64 MB assumed, 14 MB linear pool after H03). Azahar shows a commit limit of 98304 KB. This is not an O3DS number. Only S28 can measure the real limit.
- The O3DS and the N3DS were never run. S28 is the first hardware run. All speed numbers are Azahar numbers. They are not valid for the O3DS.
- The game runs at exactly 30 fps (PC01: 29.91 steps per second, 2 vblanks per step). 60 fps (F06) and widescreen (F07) come later.
- Port layer files and tools: see docs/INDEX.md. Fixes in the platform layer: docs/HISTORY.md.
- Golden frames (G01g): `python 3ds/tools/golden.py [--update] [scene..]` runs 10 scenes (title, fileselect, intro, toad, yard, battle, pause1 to 4). It prints a table of pixel differences against `3ds/golden/*.png`. About 25 s per scene. Run it after a graphics change. Update the references only after you check the change by eye.
- Boot line `mem split H03`: linear pool 14 MB (was 20 MB). A full texture cache skips the draw (no white quad).

## Debug switches (put in `debug3ds.txt` or `-Debug "..."`). One line per group. Details in docs/INDEX.md
- Start: `fast autoa freshsave loadslot=<n>`. Presets: intro yard toad toad_east toad_west osr kmr02. Warp: `warp=<map>:<entry> story=<name|n> preset=<name> partner=<n> sp=<n> hp=<n> battle[=<f>:<s>] battlef=<n> demobattle`.
- Run control: `smoke smokeframes=<n> tour=@file tourfx=<n> turbo padscript=<..|@file> padrand padlog taptest`.
- Pictures: `shots shotwin=<f>:<n> shotmode=<m>:<n> shotskip=<n> offdump`.
- Draw A/B and trace: `nocomb nocull nomix nooff noprimz rmoff rmlog drawlog=<f>:<n> ddonly=<lo>:<hi> ddverb depthview=<1|2|3> xtrace=<f>:<n> prevtest nofallback`.
- Checks: `ptrcheck ptrtest romstrict romtrace romfull dataprobe assets scripts faulttest crashtest crashtest_call crashtest_audio crashtest_audio_call`.
- Timing and log: `phase phasecsv phasegbi stats audiostat pacestat logstat lognull logflood noaudio`.
- Run script `3ds/run_azahar.ps1`: `-Until -Timeout -Stall -Turbo -TurboFactor -TurboLimit -Smoke -FaultStop -Capture -Gdb -GdbScript -Interactive`. Exit 0 ok, 1 failed, 2 unmapped access or crash. Main command: `powershell -File 3ds/run_azahar.ps1 -Turbo -Smoke -FaultStop -Debug "fast preset=toad"`. Build: `3ds/build.sh`, `3ds/go.ps1`; the build skips when the 3dsx is newer than all sources, so touch a source after a CMakeLists.txt change. Hardware skills: `push-o`, `push-n`, `pull-o`, `pull-n`, `push-a`.

## Open issues (one line each, row id in 3ds/SLICES.md)
- V06 (queued): the ground gap in buildings is correct (user answer). Real fault: the outside must be black in a building room.
- V07 (SHELVED by user 2026-10-18): items flicker. Not reproduced in Azahar. The user reports it again if seen. Do not work on it.
- V09 (SHELVED by user 2026-10-18): intro tints changed by S25, and the nok_01 tour fault. Partial edits on branch shelf/v09-nok01.
- V10: battle Strategies box and level-up box draw as a checkerboard.
- V12 (USER-CONFIRMED 2026-10-18): battle damage star is zoomed in and clipped.
- V13 (USER-CONFIRMED 2026-10-18): lava texture in the volcano looks off.
- F03 done: `pm_depth_query` (3ds/src/pm_depth.c) reads one PICA depth pixel into `DepthCopyBuffer`. The glow draw on screen is not checked.
- F08: fog partly done by V06. The per-vertex fog factor is open.
- F10: tst_12 endless loop (LVar5 = -17, LVarF = -11, N64-identical decomp defect of a cut map, no port fix). Skip tst_12 in tours.
- Not verified by eye: partner moves, items and star power in battle. Pause with items or a partner. Hardware.
- F09 language select: future feature, not in the current batch.
- PO3 RISK (RSP to PICA port): effects that read back N64 memory images (X02, F03, water_splash, grabs) may need rework. Check each at every PO3 slice (docs/plans/PO3.md R7).
- Buffer bridge plan: docs/plans/buffer_bridge.md (B0 is X02).
- S28 only after all Azahar-verifiable rows are done (user decision 2026-10-18).

## Stand-ins still open (checked against docs/HISTORY.md, S20 audit A, B, C, D and later results)
- Placeholder aliases (class D, code only, no data read): 47 `--defsym X=gPartnerStatus` in `3ds/CMakeLists.txt` (`world_partner_*`, `world_use_item_*`, `pause_*`, `filemenu_*`). Code overlays linked in place. Verified in P01 and R01.
- `((u8*)1)` ROM ranges for linked code overlays in `3ds/stub_include/ld_addrs.h`, `effect_ld_addrs.h`, `battle_ld_addrs.h`. Code only.
- Empty shims: `crash_screen_init`, `is_debug_init`, `load_obfuscation_shims` (`3ds/src/game_stubs.c`), `crash_screen_set_draw_info` (`3ds/src/pm_later.c`). User decision Q5: obfuscation shims stay empty. Row S31 checks the crash screen.
- Storage at real N64 addresses (class C, no game logic): `gBackgroundImage` and `pm_world_entity_heap` (`3ds/src/pm_kmr_stubs.c`).
- Gone (do not look for them): `area_table.c`, `pm_title.c`, `pm_evt_stub.c`, `pm_actor_link.c`, `pm_shadow.c`, `pm_fx.c`, battle stand-ins, effect graphics zeros (A17), shadow headers (B3 to B6), the `spriteIdx` guard, `note_door`, the GotoMap guard (removed by S30b).
- Approximate, not stand-ins: fixed light direction for `G_LIGHTING` vertices (`pm_gbi.c`), no fog table (F08), no texture generation, LOD or `G_BRANCH_Z`.
- V06 done: `pm_tex.c` cache hit now needs the CI palette hash (FNV-1a), so a palette edited in place (room fade) re-converts. Russ T door shows black outside. Not checked: mac_00 other houses, kmr_20 and kmr_02 doors, 3dsx size change.
- V12 done: `pm_gbi.c` stores G_SETTILE shifts (`tile_shift`) and `vert_uv` applies them (RDP rule). Battle hit shows the whole star. Golden battle 0.19, toad 0.01, yard 0.00. Not checked: the other 13 effects, texrect shift, 3dsx +344 B vs F03.
- V13 done: `pm_gbi.c` no longer adds the SetTileSize pan to a block-load source (it read past the 32x64 lava image = noise) and tile 1 now reads its TMEM half (stale LoadTile record cleared by LoadBlock). Lava in kzn_02 shows orange lava. Golden battle 1.05, toad 0.01, yard 0.00, 3dsx +288 B.
- V13 not done: sub-texel pan (uls >> 2) and bilinear (`pm_tex.c` GPU_NEAREST) are unchanged and not proven to matter. Not checked: pixel diff against N64, kzn_03/11/20/22 shots (tour ran with no fault), water and conveyors.
- Found, not fixed: `tex cache full 129x129` on tour start at kzn_03 (also before V13), 1111 skipped draws. See row V13b.
- V12b done (b655891ea): `pm_gbi.c` G_TEXRECT applies the tile shift to s, t and the step (bulb glow uses shift 1 1). Tested one by one with a temp hook (removed): damage_stars, stars_spread, stars_orbiting, explosion, snowflake, big_snowflakes, energy_orb_wave, radiating_energy_orb, damage_indicator, ending_decals: whole, none zoomed or clipped. Golden battle 1.05, toad 0.01, yard 0.00, 3dsx +208 B vs V13.
- V12b not reached or not judged: snowfall (nothing drawn at frame 235), radial_shimmer (nothing visible), fright_jar (black wedge, needs battle), bulb_glow (black block, row V12c). Right shifts 1 to 3 appear only on tiles 1 and 2 at boot, never as render tile 0 in any run.
- V10 done (9a51423b9): in `pm_gbi.c` sync_legacy, a 2-cycle window (box_prod) with a non-white tile 0 now sets box_pat, so box_pattern_rect builds the product. Cause was prep cause 3 variant: box_pat stayed 0. Strategies box readable in a shot, golden battle 0.38, toad 0.01, yard 0.00.
- V10 not checked: level-up box, other draw_box users (dialog, name entry), the rotating or 3D window triangle path (G_TRI2 has no product code).
- B1 done: `pm_gbi.c` has `img_role(for_fill)` (z-buffer siz 2 stays MAIN until B3), one `pm_zbuf_need()` call in it for the VISCVG fill (still one call per fill, not one per frame), `imglog` switch, stats `sp sl gr` (`pm_gpu_off_count()`). Golden toad 0.01, yard 0, battle 1.05, title 0. pause1 DIFF 23.63 is the same value without B1 (existing fault, not B1).
- B1 not checked: water_splash inside a real battle (a temp hook fired it, no image role seen), the flo_10 and mac_01 grab sites (not reached by f275 at entry 0), 3dsx +840 B vs V10.
- B2 done: pm_gpu_grab() in pm_gpu.c (flush, early target clear, C3D_FrameSplit, GX_TextureCopy of the strips, one rotated quad into the slot (cfb, siz 2)), grab_cur() in pm_gbi.c before each tile load from the frame buffer being drawn, GRAB_MAX 8, reuse while no draw hit the main target. Switch grabtest grabs the finished frame (slot equals shot, diff 0.0). Golden toad 0, yard 0, battle 1.06, pause1 23.63 (unchanged, row GP1). 3dsx +2856 B vs B1.
- B2 not checked: hardware (queue order and timing are Azahar only), water_splash in a real battle, mac_01 crystal ball, the 5th slot case (OFF_N stays 4), flo_10 entry 0 (the dead water code UnkModelFunc001 is never called by the map, so a temp hook called it at entry 2, hook removed, entry 2 faults in partner_walking_enable without a partner).
- GP1 done: cause was F03 (6885fd88d), its z-buffer tile skip in pm_gbi.c also skipped the pause background tiles (tall tiles from nuGfxZBuffer). Now it skips 1 row tiles only. golden pause1 0.00 (reference unchanged, shows the toad yard behind the menu), toad 0, yard 0, battle 1.04. Not checked: depth query in a real scene after the change, hardware.
