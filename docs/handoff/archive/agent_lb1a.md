# Handoff: LB1A epic (ARCHIVE, LB1B, FINAL9)

## State
- ARCHIVE: done. Commits d64f39f06 and 00f20b211. Archive names are pm_3ds_<build>_<hash>.3dsx. push_3ds.sh archives on every push (archive_build). --archive-only archives and stops (no push). build.sh keeps the newest 10 .elf/.map pairs.
- LB1B: PASSED. Committed c294e993f (LB1 lazy FrameBegin). After the budget reset: partner_menu and text_menu HEAD repeats byte-equal (lane final9 = build 1490 binary), patched build 1504 equal to HEAD. fsplit gwait 0.0 ms on HEAD and patched (Azahar does not show the wait; the console must prove it). No code change, so no rebuild.
- FINAL9: build 1504, git c294e993f, archive 3ds/logs/builds/pm_3ds_1504_c294e993f.3dsx (sha256 d5a6eddc...25d7, equal to build3ds/pm_3ds.3dsx). Catalog: 8 scenarios log checks pass, plus sun_sbk (log 3 of 3, shot read: desert, Mario, partner; sun disc not clearly visible). Not pushed.
- Working tree: 3ds/src/map_restore_table.c (user file; restored byte-equal to 3ds/src/map_restore_table.c.bak after each build; never committed). docs/handoff/test_catalog.md is modified by the TOADFOUNT agent (unstaged). Not touched by this epic.

## Archive renames (3ds/logs/builds)
- pm_3ds_final2_83e903455 -> pm_3ds_1428_83e903455
- pm_3ds_final3_b5c7b5226 -> pm_3ds_1433_b5c7b5226
- pm_3ds_final4_6e965c84b -> pm_3ds_1456_6e965c84b (build 1456 from diag_hwlog2.md)
- pm_3ds_final5_835be2fd7 -> pm_3ds_1462_835be2fd7
- pm_3ds_final6_3dcfc661d -> pm_3ds_1467_3dcfc661d
- pm_3ds_final7_311532db5 -> pm_3ds_1474_311532db5
- pm_3ds_final8_f230e46c6 -> pm_3ds_1488_f230e46c6
- pm_3ds_final9_fd05e686a -> pm_3ds_1490_fd05e686a
- pm_3ds_final_29f504d7b -> pm_3ds_bunknown_29f504d7b (build number not recorded. Commit-count formula gives 1409, not verified. Windows does not allow "?" in names.)
- Hashes checked against pushed_builds.txt sha256 values.

## LB1B results (lane lb1b, build 1498; FINAL9 build 1504 same src)
- Top shots vs HEAD build 1490 binary: mac01 2 of 2, mac01t 2 of 2, yard 1 of 1, pause_menu 1 of 1, file_select 1 of 1 byte-equal.
- battle_start 0000 to 0003 byte-equal. 0004 to 0007 differ in sprite pixels only (x 58 to 215, y 98 to 302). Read by eye: same scene and HUD. HEAD not reproducible there.
- GPU readers with pm_gpu_begin_wait: pm_gpu_read_frame, grab_impl (pm_gpu_grab, pm_gpu_grab_prev), pm_gpu_off_begin, pm_gpu_depth_at, pm_gpu_drew, pm_gpu_end, gbi_task (pm_nusys.c; covers every gbi task, key replay and fallback). The HUD and menu draws use the libctru console only (no wait).
- Status: 3ds/logs/perf/status_LB1B.txt.

## Open
- fsplit gwait is 0.0 ms in Azahar. The console must confirm the wait.
- sun disc: top shot does not clearly show it. Read on the console.
- Old docs (agent_*.md, diag_*.md) still name the old archive files. Only FINAL_WRITEUP.md and pushed_builds.txt were updated.
- test_catalog.md has no pm_3ds_final references. Not edited (TOADFOUNT agent's unstaged edits).

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=<row> [PM_LANE=<lane>] PM_BUILD_CAP=900 sh 3ds/build.sh; restore 3ds/src/map_restore_table.c from a saved copy (equal to .bak).
- Test: PM_ROW=<row> powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 <scenario> -Row <row> [-Build <lane>].
- Archive only (no push): sh 3ds/push_3ds.sh --archive-only --no-build
- Push (user runs): PM_ROW=PUSH sh /c/Users/Andrew/Documents/GitHub/papermario-3ds-port/3ds/push_3ds.sh --no-build --ip 192.168.1.170 pacestat logstat dcsum fsplit loglevel=debug
