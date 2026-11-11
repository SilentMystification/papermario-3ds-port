# Row LOGX4: pm_logrecv.py reads the newline the game writes before #PMEND (and the test sends the game's exact frames)

(Model: Haiku. Base: main. Edits 3ds/tools/pm_logrecv.py (the frame end read, :83-85) and 3ds/tools/test_pm_logrecv.py (its frames).
Tool only, no game change. Reads this file and CLAUDE.md.)

## Fact (XL 192.168.1.131, main c16fd02a0): `pm_logrecv.py --ip 192.168.1.131 --label xl` twice: `protocol error: bad frame end for
crash.txt: b''`; live text 8193 bytes, then the crash.txt frame header.

## Cause (oracle code read)
- The game writes a frame as `\n#PMFILE <name> <size>\n`, <size> raw bytes, then `\n#PMEND <name>\n` (pm_log.c send_file: the header
  and the trailer both start with a newline, so they stand on their own line after any live text or file byte).
- pm_logrecv.py reads exactly <size> bytes, then `end = next_line()` and compares it with `#PMEND <name>` (:83-85). The next line is the
  empty line made by the trailer's leading newline: `end == b''`, the protocol error. test_pm_logrecv.py sends frames without that
  newline (:29, :31: `CRASH_PAYLOAD + b"#PMEND crash.txt\n"`), so the test passed while the console format failed.
- The 8193 live bytes are normal: at accept the writer sends the ring until the socket buffer is full (EAGAIN, about 8 KB), then the
  crash.txt frame (send_all blocks); the rest of the ring follows after the frames. Nothing is lost on the game side.

## Start
`sh 3ds/tools/wt_new.sh LOGX4`. PM_ROW=LOGX4 on every call.

## Exact edits
1. pm_logrecv.py: replace `end = next_line()` by `end = next_line()` followed by `if end == b"": end = next_line()  # LOGX4: the game's
   trailer starts with a newline (pm_log.c send_file)`. Update the format comment at :8: `\n#PMFILE <name> <size>\n <size> bytes \n#PMEND <name>\n`.
2. test_pm_logrecv.py: send the frames as the game does: `b"\n#PMFILE crash.txt 10\n" + CRASH_PAYLOAD + b"\n#PMEND crash.txt\n"` and the same
   for log.txt (:29, :31); keep one frame without the leading newlines as a second case (an older game build), so both forms pass.

## Gate
- `python 3ds/tools/test_pm_logrecv.py`: OK (both forms). `git diff main --stat`: the two files only.
- The user's console check: `python 3ds/tools/pm_logrecv.py --ip 192.168.1.131 --label xl` writes hw_xl_crash.txt and hw_xl_log.txt with
  the sizes of their #PMFILE lines.

## Step 2 (after 6b4e9a8b0: the end fix works; the live assertion fails on the frames' leading newline)
Rule (pm_log.c send_file): the game writes `
#PMFILE` and `
#PMEND` always, whether the live text before ended with a newline or not.
Game log lines end with a newline, except a line cut by EAGAIN in the ring replay; then the frame's newline ends that partial line.
So the tool drops an empty line only when the next line is a #PMFILE line: 1. pm_logrecv.py, in the live-text branch: an empty line is
held (not written); when the next line is a #PMFILE header the held line is dropped, else it is written before that next line (and at
EOF). A partial line ("abc" then `
#PMFILE`) reads as the line "abc", written as "abc
": correct, no rule needed.
2. test_pm_logrecv.py: case A (game form): b"live one
" + b"
#PMFILE crash.txt 10
" + CRASH_PAYLOAD + b"
#PMEND crash.txt
" +
b"more text
" + b"
#PMFILE log.txt %d
" % n + LOG_PAYLOAD + b"
#PMEND log.txt
": live file b"live one
more text
". Case B (old form,
no leading newlines): the same live expectation. Case C (partial line): b"partial" + case A's frames: live file starts with b"partial
".
Both frames' sizes as now. Then the gate as written; the coder can finish in build3ds/wt/LOGX4.

## End
Delete this row file in the completing commit. Hand back the hash and the test output.
