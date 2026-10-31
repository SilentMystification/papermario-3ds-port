# agent_quit1: PC command "quit" ends the game through the normal exit path

## State
Row QUIT1. Code, PC tool and handoff in one commit on main. Build passed. Not tested on hardware or Azahar (no start, per the task).

## Steps as built
1. Socket read on the game side. Before this row the game sent on the live socket but never read it.
2. The writer thread (pm_log.c, `writer_main`) calls `read_cmd()` after `accept_client()`, every pass (at most 250 ms). The read is non-blocking (socket is O_NONBLOCK). It takes one line per connection, max 15 bytes. The line `quit` (CR allowed) sets `quit_req`.
3. The game thread (`pm_log_poll`, called once per loop pass in `pm_frame_loop`) sees `quit_req`, clears it, logs `quit received`, and sets `pm_exit_request = 1`. That flag already exists (PT28, menu row Exit to Homebrew). The frame loop ends, `pm_frame_loop` returns to `boot_main` (src/main.c), then `game_thread` logs `boot_main returned`, then `main` calls `pm_shutdown` (`exit: audio job done ...`, `pm_log_stop`, `C3D_Fini`, `gfxExit`, `pm_log_soc_exit`) and returns 0.
4. PC side: `3ds/tools/quitgame.py <ip> [seconds]` connects to port 17492, sends `quit\n`, then reads and discards game output until the game closes the connection or the wait ends (default 10 s). Reading matters: closing with unread data sends a reset to the game.
5. livelog.py is unchanged.

## Changes (file, lines in the new tree)
- `3ds/src/pm_log.c` lines 78-96: new `cmd_buf`, `cmd_n`, `cmd_done`, `quit_req`, and `read_cmd()`.
- `3ds/src/pm_log.c` lines 98-112, `accept_client`: resets `cmd_n` and `cmd_done` on each new client.
- `3ds/src/pm_log.c` lines 113-122, `pm_log_poll`: extern `pm_exit_request`, quit handling.
- `3ds/src/pm_log.c` lines 608-611, `writer_main`: `if (log_file_on) { accept_client(); read_cmd(); }`.
- `3ds/tools/quitgame.py`: new, about 45 lines.

## Tests done
- `python -m py_compile 3ds/tools/quitgame.py`: OK. livelog.py parses: OK.
- Fake server on 127.0.0.1:17492 (scratchpad, not in repo) sent 100 lines of replay, then waited for input. quitgame sent exactly `b"quit\n"` and saw the close. Result: "server got: b'quit\n'".
- Build: `python 3ds/tools/gen_map_restore.py`, then `sh 3ds/build.sh`: exit 0. No warning names pm_log.c. The user's map_restore_table.c was saved to scratchpad before and restored after the build (not committed).

## Commands
- Quit: `py -3 -u 3ds/tools/quitgame.py 192.168.1.170` (or the O3DS IP 192.168.1.180 as used in docs).
- After exit: see the netloader answer.

## Netloader answer (what the code does)
- The repo code ends at `return 0` from `main` (pm_main.c line ~690). Nothing in the repo calls `exit()`, `envSet`, or any nxlink/netloader return. `netloaded` (pm_main.c 674) only turns on the live log.
- Whether the Homebrew Launcher's netloader waits for the next 3dslink push is not in this repo. Not verified on hardware.
- Repo docs (docs/handoff/HANDOFF_2026-10-23.md line 51) say the console needs the Homebrew Launcher netloader running (press Y) before each push. So a new push after `quit` probably needs the user to press Y again on the console. This is from the docs, not checked.

## Open items
- The writer reads commands only when `log_file_on` is set. With log=off, quit is not read.
- Not tested on hardware: the time from `quit` to the console returning to the menu.
