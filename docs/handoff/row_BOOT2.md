# Row BOOT2: faster start to the first logo (user, 2026-11-09)

## User facts
O3DS XL: "startup time on actual hardware is still pretty slow, it takes a long time to hit the first company splash screen" (both
netloader and SD). User chose (2) and (3) below now; (1) (map/battle data out of the 3dsx) goes into the parked ASSETFREE epic.

## Causes (shared oracle, code read, estimates)
(2) About 59 of 320 ROM chunks are touched before the first frame (57 audio-bank chunks: SBN header and tables,
au_load_BK_to_bank at audio init) plus about 205 small direct reads (pi_read_raw -> pm_rs_read_direct, pm_os.c:426-430) while the
stream has not reached them; each an FSFILE_Read with SD latency (3-8 ms): about 0.5-1.5 s. See agent_boot.md (romorder run 81263).
(3) Audio init before the logos (ndspInit loads dspfirm.cdc, SBN/INIT parse, bank loads): about 100-300 ms.

## State and plan (2026-11-09, coder run)
auread check (run 181942, warm kept Azahar, same numbers as the cold base): `auread n 139 bytes 116536 max 20544 chunks 54` at the
first draw. Direct reads before the first draw: 37. Base: first draw 1836 ms, title 12763, rdone 4617, waits 80 wms 1099.
Fix (a) done: pm_rom_read_exact (pm_os.c) in au_read_rom while pm_rs_all is 0 (pm_rs_read_direct, drop ranges skipped).
Boot line after fix (a): first draw 907 ms (run 182629) and 894 ms (repeat); title 11812 and 11795 (earlier than 12763); rdone 5750
and 5734 (later than 4617: the 203 exact reads share the SD file with the loader); waits 203 wms 8-9; demand 0.
Gates: boot_logo_pic PASS (logo frame 200, shot read: Intelligent Systems logo); aud_mac01 PASS (census no channel 0, cut 0);
memcheck OK (static 20756, margin 1230); nd_host not run (no draw code changed). Open: rdone later by about 1.1 s; first draw
907 on one run, 894 on the repeat. Fix (b) (ROM reorder) and (3) (audio deferral) not started (user: fix (a) only).

## Start state
`sh 3ds/tools/wt_new.sh BOOT2`, branch BOOT2. PM_ROW=BOOT2, PM_LANE=BOOT2. One coder agent.

## Steps
1. Measure on Azahar (cold): bench boot_ms (boot line `boot ms main log opt rom gfx audio first title rdone waits wms`) and
   boot_romorder (chunks before the first frame, direct reads). Base numbers first.
2. (2) Reorder the packed ROM (gen_rom_remap.py / pm_rom_keep order) so the boot chunks come first (SBN tables and audio banks, then
   the logo data c8/c15/c16), and read those chunks as one sequential read before the loader thread continues; no direct reads at boot.
3. (3) Start the audio init on its job thread after the first logo frame is presented (the logos are silent until the jingle); the
   first sound must still play (log line of the first jingle).
4. Gates: boot_ms first-frame time down, direct reads before the first frame about 0; boot_logo_pic shows the first logo; nd_host;
   memcheck; the intro jingle plays (audio log). Console (user): the boot line on a cold start.
5. Commit; delete this row file in the completing commit. Budget: 6 Azahar starts.

## Touch only
3ds/tools/gen_rom_remap.py (or the ROM order source), 3ds/src/pm_romload.c, pm_os.c (boot reads), pm_main.c (init order),
the audio init entry (pm_audio / pm_dspaud.c init only).

## Rules
CLAUDE.md. STOP RULE: 90 minutes or 150 tool calls. No Monitor or tail loops.
