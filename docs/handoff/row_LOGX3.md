# Row LOGX3: host tool pm_logrecv.py (receive log.txt and crash.txt over the live socket)

(Model: Haiku. Base: main. New file 3ds/tools/pm_logrecv.py and two help lines in 3ds/go.ps1 (the pull text, not the PULLCRASH lines
if PULLCRASH changed them: add after them). Runs in parallel with LOGX1/LOGX2. Reads this file and CLAUDE.md.)

## Contract (row LOGX2)
The game listens on TCP 17492 (pm_log.c live log). The tool sends `sendlog\n`; the game answers with live text lines and, in order,
two frames: a line `#PMFILE <name> <size>` (after a newline), exactly <size> raw bytes, then a line `#PMEND <name>`. Names: crash.txt,
log.txt. At the first client of a boot the game may send a crash.txt frame before any live text.

## Exact edits
1. 3ds/tools/pm_logrecv.py: argparse `--ip` (required), `--label` (default o3ds), `--timeout` (s, default 60), `--out` (default build3ds).
   Connect (timeout 8 s; a failure prints `[logrecv] could not connect to <ip>:17492: <error>` and exits 1). Send `sendlog\n`. Read the
   stream into a buffer; outside a frame, text goes to <out>/hw_<label>_live.txt; at `#PMFILE <name> <size>` read exactly size bytes into
   <out>/hw_<label>_<log|crash>.txt (log.txt -> _log, crash.txt -> _crash), then expect `#PMEND <name>`; print
   `[logrecv] <name> -> <path> (<size> bytes)`. A repeated crash.txt frame overwrites the file. Stop after the log.txt frame or at the
   timeout (`[logrecv] timeout: got <list>`, exit 2). Python standard library only.
2. 3ds/tools/test_pm_logrecv.py: a local socket server thread that sends text, a crash frame of 10 bytes, more text and a log frame of
   100000 bytes split in odd chunks; the test runs the tool's parse function on it and checks both files and sizes.
3. go.ps1 pull help text: one line `Without FTP: python 3ds/tools/pm_logrecv.py --ip <addr> --label o3ds (game running, live log on).`

## Gate
- `python 3ds/tools/test_pm_logrecv.py`: OK. `git diff main --stat`: the three files only. No console run in this row.

## End
Delete this row file in the completing commit. Hand back the hash and the test output.
