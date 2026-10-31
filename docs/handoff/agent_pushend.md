# agent_pushend: live log ends by itself

## State
- 3ds/tools/livelog.py (the reader that push_3ds.sh calls at line 208) now stops on:
  - `exit: audio job done` (pm_main.c)
  - `boot_main returned` (pm_main.c)
  - `cpu limit: set <n> (exit (main))` (pm_log.c)
  - `watchdog: exiting the process` (pm_wdog.c)
  - no data for 5 s after the game closes or loses the connection (reconnect attempts fail)
- Final stdout line: `[livelog] live log: ended (<reason>)`. The reason is the end line label, `connection closed for 5 s`, or `time limit` (the seconds cap).
- File output (build3ds/hw_live_log_<octet>.txt) is unchanged. The status lines are stdout only.

## Commits
- See `git log -1 -- 3ds/tools/livelog.py`.

## Test
- py_compile livelog.py: OK.
- Local fake server on 127.0.0.1:17492 (scratchpad script, not in repo):
  - exit line sent, socket kept open: reader stopped at 0 s, reason `exit: audio job done`.
  - server sent lines, closed, stopped listening: reader stopped at 5 s, reason `connection closed for 5 s`.
- Not run on hardware or Azahar (no console used).

## Open items
- A crash exit prints `[CRASH] exiting the process` (pm_main.c:115). It is not in END_LINES. The 5 s close rule covers it.
- A game that keeps the socket open and silent without an end line runs to the seconds cap (PM_LOG_MAX, 3600 s).

## Next steps
- On the next console push, confirm the final line is `live log: ended (...)`.

## Commands
- `python -m py_compile 3ds/tools/livelog.py`
- `py -3 -u 3ds/tools/livelog.py <ip> <secs> <out>`
