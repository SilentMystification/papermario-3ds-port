# Row PAUSEDR: pause frames serialize on message ROM reads (drain since PAUSEC)

## User facts
User (2026-11-09): "start menu rotate effect is still a little laggy." Console pause window (O3DS XL, main 6333cec32): ft 37.5 ms,
wk 35.8, core2 wait bw 33.53 fw 0.00 (the logic waits 33 ms per step outside the frame wait).

## Cause (oracle)
PAUSEC changed nd_walk.c:516 to `if (pm_walk_side && !pm_wq_on_logic()) return;`: a logic-thread nuPiReadRom (pm_os.c:433
pm_walk_drain("rom")) now waits for the whole queue (pm_wq_idle -> wq_block(c_empty), nd_wq.c:168). In the pause, draw_msg (msg.c:2032)
-> dma_load_msg (msg.c:1428-1447: 3 dma_copy = 3 nuPiReadRom), get_msg_properties again (msg.c:2066 -> 1765), each get_msg_width 3 more:
tens of ROM reads per step. Each read also posts pm_tex_invalidate (pm_os.c:407 -> pm_tex.c:1252, pm_wq_post); > 64 per step blocks on
c_space (ring 64, nd_wq.c:35). Player raster loads (101b90_len_8f0.c:352) drain too: mac_01 may pay it as well.

## Start state
Worktree build3ds/wt/PAUSEDR, branch PAUSEDR from main (hash in the first commit). PM_ROW=PAUSEDR, PM_LANE=PAUSEDR.

## Plan
1. Counters on the prof `core2 wait` line: dr (logic drains per step), drw (drains with a non-empty queue at entry), wqf (c_space
   waits). Cold debug switch drainlog: per 60 steps the count by why and the top 4 nuPiReadRom callers (return address). Run the threaded
   pause (pause_thr_pic variant) on Stats, Party, Spirits, and once in mac_01. Commit.
2. Fix: TARGET_3DS dma_load_msg copies from pm_rom_ptr(addr, n) with memcpy (fallback dma_copy if NULL). Confirm: no msg buffer is a
   texture or list source (msg_draw.c), and the MSG_ROM range is outside pm_rom_swap16 and the asset table (pm_os.c:436-445).
3. Grey pause background missing on the console (user, 2026-11-09: "top screen doesn't go grey on console, does on Azahar", the pause
   menu). Suspect: PAUSEC reserves the 150 KB z-buffer at boot (pm_zbuf_reserve, pm_main.c ~812); if it fails on the O3DS heap the
   filter is skipped by design (no grey). Print the reserve result as a prof-mode line (pm_log_prof_line, once at boot:
   `prof zbuf reserved <0/1> heap_free <KB>`) so a prof push shows it; if it fails, reserve it before the ROM block or take it from
   the leave budget (memcheck mem_budget.json leave), and check the filter runs (log line per pause open). Gate: console pause grey (user).
4. Report the other drw sources in the pause and in mac_01; fix only pause ones in this row, list the rest.

## Done criteria
nd_host PASS; memcheck OK; pause_spin_grab_det_w300 40/40 byte-equal to build-at base; pause_thr_pic no CRASH, menu shown; Azahar drw per
pause step about 0. Console bench spot_pause_prof (new: mac_01, START, prof windows on Stats, Party, Spirits; cap 45 s, 3-window
measurement): bw near fw, ft down.

## Rules
CLAUDE.md rules 5, 7, 9, 11. No git stash, no toggles, no stubs. Never commit 3ds/src/map_restore_table.c.

## At the end
Delete this row file in the completing commit. SendMessage main "PAUSEDR ready <hash>".
