# Row PULLCRASH: ftppull also pulls crash.txt

(Model: Haiku. Base: main 337a5d4fe. Tool files only, disjoint from every running row. Reads only this file and CLAUDE.md.)

## Facts (oracle code read)
- 3ds/tools/ftppull.py:36-62 pulls only /3ds/PaperMario/log.txt and /luma/dumps/arm11/*.dmp.
- The game writes crash.txt in pm_crash_line (3ds/src/pm_log.c:1338-1356): `open("crash.txt", O_WRONLY | O_CREAT | O_APPEND)` on its own
  fd, a relative path. The cwd is sdmc:/3ds/PaperMario (pm_main.c chdir before the game thread starts, about :950), so the file is
  /3ds/PaperMario/crash.txt on the SD (the same folder as log.txt). A crash before that chdir writes crash.txt in the boot cwd.
- It needs no log state: the write to crash_fd comes before crash_flush (which returns at once when the writer thread is off,
  :1303-1304); log_fd and the live client are used only when open. So crash.txt is written with logging off, no live log, no log file.
- The handler is installed on every pm_thread_create thread (pm_thread.c th_entry :39) and on the main thread (pm_thread_adopt("main"),
  pm_main.c:944). Threads made without pm_thread_create (libctru's own threads) have no handler: their crash gives no crash.txt line.

## Start
`sh 3ds/tools/wt_new.sh PULLCRASH`. PM_ROW=PULLCRASH PM_LANE=PULLCRASH on every call.

## Touch only
3ds/tools/ftppull.py (main, after the log.txt block); 3ds/go.ps1 (the pull help text lines 40-41).

## Exact edits
1. ftppull.py: after the log.txt try/except block add the same block for crash.txt:
   `crash_path = os.path.join(out_dir, f"hw_{label}_crash.txt")`, `ftp.retrbinary("RETR /3ds/PaperMario/crash.txt", f.write)`, print
   `[ftppull] crash.txt -> <path> (<size> bytes)`; on an error print `[ftppull] no crash.txt (<error>)` and remove the empty file.
2. go.ps1 help text: `Downloads the game log, crash.txt and any Luma crash dumps to` and
   `  build3ds/hw_o3ds_log.txt, build3ds/hw_o3ds_crash.txt and build3ds/hw_o3ds_dumps/.`

## Gate
- `python -c "import ast,sys; ast.parse(open('3ds/tools/ftppull.py').read())"`: no error.
- `git diff main --stat`: only the two files. No console run (the user pulls on the next crash).

## Stop and end
An edit that does not fit: commit, hand back the exact output. Delete this row file in the completing commit. Hand back the hash.
