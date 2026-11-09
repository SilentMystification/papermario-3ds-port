# Row BOOT: stream the ROM at boot; start the game after the header check

## User facts
User (2026-11-09): "I want the game to load faster. ... load in whats needed to start the game first and start the game. Then continue
to load the rest of the game as the splash screens play out."

## Start state
Worktree build3ds/wt/boot, branch BOOT from main (hash in the first commit). PM_ROW=BOOT, PM_LANE=boot. One Opus agent (concurrency work).

## Facts (oracle trace, main 6333cec32)
- pm_rom_init (pm_os.c:207-283) on the game thread: fopen papermario.z64 (sdmc:/3ds/PaperMario, baserom fallback), 64 KB header check,
  cap check (:258-261), one memalign(16, PM_ROM_PACKED 25748 KB), then a synchronous fseek+fread per pm_rom_keep segment (69, rom_remap.h).
  Big segments: SBN 0xF00000 10.3 MB, mapfs 0x1E40000 9.7 MB, sprites 2.3 MB, icons 1.3 MB, msg 1 MB.
- Only two readers of the image: pi_read_raw (pm_os.c:343, memcpy :401; nuPiReadRom :433, osPiStartDma, au_read_rom) and pm_rom_ptr
  (pm_os.c:413; find_asset pm_map.c:408/411, map_prefetch pm_map.c:566, load_texture_header_at model.c:2289, dsp_voice.c:210, nd_c2haz.c:152).
- Azahar warm run 3627: rom 4985 ms of first 6259 ms; title 16681. No console boot ms line yet (BOOT-2 open, agent_boot.md).

## Plan
0. Measure: debug switch romorder logs the first touch of each 128 KB chunk (romorder c<idx> f<frame> t<ms>) in pi_read_raw and
   pm_rom_ptr; one Azahar boot; chunk counts before first frame and before title into agent_boot.md STATE. Commit.
1. New 3ds/src/pm_romload.c: rs_st[320] (0 absent, 1 loading, 2 ready, 3 failed), rs_all, rs_err, rs_inflight, counters;
   rs_read_chunk (keep-segment walk, FSFILE_Read with explicit offset straight into the block; handle by FSUSER_OpenFileDirectly as
   pm_mem.c:57); rs_claim (CAS 0->1, release store 2/3); loader thread core 0 prio 0x2F, 8 KB heap stack, sequential claims, rs_all=1 at the
   end; rs_need inline (one acquire load when done) -> rs_need_slow (claim and read on the calling thread, or wait in 1 ms sleeps;
   failed: fatal screen on the game thread); RS_GUARD 256 B past each access (prefetch/DMA coherence).
2. pm_rom_init: keep the header and cap checks and the memalign; delete the fread loop (no toggle); start the stream; replace the RR2
   comment. rs_need before the memcpy in pi_read_raw and before each return of pm_rom_ptr.
3. Sleep/HOME: loader sets rs_inflight, fence, checks pm_io_quiet (sleep 50 ms loop as pm_flash.c:65); io_gate(1) (pm_log.c ~1027)
   waits for rs_inflight == 0 within its deadline (Dekker). Exit: pm_shutdown calls pm_rom_stream_stop first. Frame-top poll shows
   the fatal screen on rs_err until done.
4. Boot line: keep the boot ms fields, add rdone/waits/wms; `rom stream done ms D MB/s X demand C waits W wait_ms M`; bench boot_ms
   metrics rom_done_ms, rom_waits, waits after title (gate 0) in the same commit.
5. Determinism: check from the link map that src/os osGetCount callers are not built; else call osGetCount once before main.c:100.
6. Step B only if waits before the first frame cost > 200 ms: gen_rom_order.py -> pm_rom_boot_order[] (claimed first).
hw_guidelines: shifts, no division, one-load fast path, no per-frame malloc, < 1 KB static.

## Done criteria
nd_host PASS; memcheck OK (margin about 305 KB); boot_ms on Azahar cold before/after: first_ms and title_ms down, rom_done before title,
0 waits after title; spot_mac01_det and battle_start_f182_cold two cold runs each byte-equal to each other and main; a title/logo
picture of the no-warp boot. Console (user): boot ms before/after on O3DS XL and N3DS, HOME and sleep during the logos.

## Rules
CLAUDE.md rules 5, 7, 8, 9, 11. No git stash, no push, no attribution, no toggles, no stubs. Never commit 3ds/src/map_restore_table.c.

## At the end
Delete this row file in the commit that completes the row; update agent_boot.md STATE. SendMessage main "BOOT ready <hash>".
