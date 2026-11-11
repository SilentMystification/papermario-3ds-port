# Row LOGX1: the live log stops dropping lines (send from the ring, resume after EAGAIN)

(Model: Haiku. Base: main. Edits 3ds/src/pm_log.c: RING, ring_add, ring_replay, accept_client, out_batch's live send block, writer_main
(one call). Disjoint from TILEWIN, PROFDISK, R13NOP. LOGX2 waits for this row (same file). Related: row_LOGNET.md (connect failures,
older): this row changes only the send path. Reads this file and CLAUDE.md.)

## Cause (oracle code read, main)
- out_batch (pm_log.c:721) sends each batch straight from lv; on EAGAIN (socket buffer full: the PC or Wi-Fi is slower than the
  lines) it does `break; /* a full socket drops the rest for the live view only */` (:757): the rest of the batch is lost for the live
  view. On a new client ring_replay (:87) sends the 32 KB ring (RING :19) on a non-blocking socket and returns at the first EAGAIN.
  With fxtest=all only 35 of 128 fxcen lines arrived; log.txt had all.
- Fix: every line goes into the ring (as now); the socket is fed from a send position in the ring (live_sent), which only moves when
  bytes are sent. EAGAIN keeps the position; the next writer pass sends on. A ring overrun (the PC is more than RING behind) sends one
  marker line and skips to the oldest byte still in the ring. RING 32 KB -> 256 KB (static +224 KB; memcheck margin is 1042 KB).

## Start
`sh 3ds/tools/wt_new.sh LOGX1`. PM_ROW=LOGX1 PM_LANE=LOGX1 on every build and bench call.

## Exact edits (3ds/src/pm_log.c)
1. `#define RING 32768` -> `#define RING 262144u`. `ring_pos` and the new `static unsigned live_sent;` count bytes since boot (no modulo
   of the stored value; the index is `% RING`, RING a power of two).
2. New `static void live_pump(void)` after ring_replay: if client < 0 return; `if (ring_pos - live_sent > RING) { char m[64]; int n =
   snprintf(m, sizeof m, "\n#live gap %u bytes\n", ring_pos - live_sent - RING); send(client, m, n, 0); live_sent = ring_pos - RING; }`;
   then while live_sent != ring_pos: idx = live_sent % RING, chunk = min(RING - idx, ring_pos - live_sent); r = send(client, ring + idx,
   chunk, 0); r > 0: live_sent += r, live_ok_tick = 0; r < 0 and errno not EAGAIN/EWOULDBLOCK: close(client), client = -1, return;
   EAGAIN: the stall rule moved from out_batch (live_eagain++, live_ok_tick, the 3 s close and its pm_log_w line), then return.
3. out_batch: keep `ring_add(lv, ln);` and replace the whole `if (client >= 0) { ... }` send block by `live_pump();`.
4. accept_client: replace `ring_replay(fd);` by `live_sent = ring_pos - ring_len;` (the pump sends the ring from its oldest byte).
   Delete ring_replay (no other caller: grep).
5. writer_main: after each `drain_locked();` (both places, :983 and :996) add `live_pump(); /* LOGX1: pending live bytes */`.

## Gate (CLAUDE.md 2; Azahar budget 2 of 8)
- Build, memcheck OK (static +224 KB); `bench.py run nd_host --row LOGX1 --why "pm_log.c"`: PASS.
- `bench.py base spot_mac01_det`, `bench.py run spot_mac01_det --row LOGX1 --why "no draw change"`, `bench.py cmp`: 6/6 byte-equal.
- Console check for the user (orchestrator push, log profile): `fxtest=all` push with the live log: 129 fxcena lines in
  build3ds/hw_live_log_<ip>.txt (128 effects and the base), no `#live gap` line.

## Stop and end
A failed gate, an edit that does not fit, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in
the completing commit. Hand back the hash, gates, memcheck line.
