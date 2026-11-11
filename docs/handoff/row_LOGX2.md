# Row LOGX2: log.txt and crash.txt sent over the live socket on request, crash.txt offered at the next boot

(Model: Haiku. Waits: LOGX1 merged (pm_log.c). Edits 3ds/src/pm_log.c read_cmd, writer_main, one new function. LOGX3 (host tool)
runs in parallel: the frame format below is the contract. Reads this file and CLAUDE.md.)

## Design (oracle code read)
- The PC already sends one command line on the live socket (QUIT1, read_cmd pm_log.c:107: "quit"); the writer thread (core 1, prio
  0x3F) reads it in writer_main (:979-980, :992-993). New command `sendlog`: the writer flushes log.txt (fsync log_fd) and sends
  crash.txt then log.txt, each framed, on the same socket, with blocking retry. While it sends, live lines wait in the ring (LOGX1).
- Frame: a line `\n#PMFILE <name> <size>\n`, then exactly <size> raw bytes, then `\n#PMEND <name>\n`. A missing file: `#PMFILE <name> 0`.
- Crash at the last boot: crash.txt is appended by every crash (pm_crash_line :1342) and stays on the SD. At the first client of a boot,
  when crash.txt is not empty, the writer sends it framed before the live lines (once per boot).

## Start
`sh 3ds/tools/wt_new.sh LOGX2` after the LOGX1 merge. PM_ROW=LOGX2 PM_LANE=LOGX2 on every build and bench call.

## Exact edits (3ds/src/pm_log.c)
1. `static volatile int dump_req; static int crash_offered;`. read_cmd: after the quit test add
   `if (!strncmp(cmd_buf, "sendlog", 7) && (cmd_buf[7] == 0 || cmd_buf[7] == '\r')) dump_req = 1;`.
2. New `static void send_file(const char* name)`: fd = open(name, O_RDONLY); size = fd < 0 ? 0 : lseek(fd, 0, SEEK_END), lseek(fd, 0,
   SEEK_SET); header via snprintf and send_all; then read 4096-byte chunks and send_all each; close; trailer `\n#PMEND %s\n` via send_all.
   `static int send_all(const char* p, int n)`: loop send(client, ...); EAGAIN: svcSleepThread(2000000) and retry, give up (close the
   client, return -1) after 5 s without progress; another error: close the client, return -1. Static buffer for the chunk (no stack 4 KB).
3. writer_main, in both places after `live_pump();` (LOGX1): `if (client >= 0 && !crash_offered) { crash_offered = 1; struct stat st;
   if (stat("crash.txt", &st) == 0 && st.st_size > 0) send_file("crash.txt"); }` and `if (dump_req && client >= 0) { dump_req = 0; if
   (log_fd >= 0) fsync(log_fd); send_file("crash.txt"); send_file("log.txt"); }`. Includes: sys/stat.h, fcntl.h if not present.

## Gate (CLAUDE.md 2; Azahar budget 2 of 8)
- Build, memcheck OK; `bench.py run nd_host --row LOGX2 --why "pm_log.c"`: PASS.
- `bench.py base spot_mac01_det`, run, `bench.py cmp spot_mac01_det --row LOGX2`: 6/6 byte-equal.
- Console check (orchestrator, after LOGX3 merges): `python 3ds/tools/pm_logrecv.py --ip 192.168.1.131 --label o3ds` during a run:
  build3ds/hw_o3ds_log.txt has the size the #PMFILE line names.

## Stop and end
A failed gate, an edit that does not fit, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in
the completing commit. Hand back the hash, gates, memcheck line.
