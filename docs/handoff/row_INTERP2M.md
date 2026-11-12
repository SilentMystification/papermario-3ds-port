# Row INTERP2M: how the two walks of an interpolated step differ in GPU command words (premise of the single-walk design)

(Model: Haiku. Design step for the fps60 double walk (row_ORACLE_NEXT.md, NATIVEAPI section "Interpolation: one walk"). Debug tool
only, under the switch `icmp`; no draw change.)

## Why
With fps60, pm_frame_walk (PMF_REPL, the t = 0.5 frame L) and pm_key_walk (the key frame K) walk the same stream (pm_nusys.c:124-200).
The single-walk design emits K once and builds L from K's command words with the interpolated matrix rows. That holds only if L and K
differ in the float uniform words (matrix palette rows, model block palette rows) and in addresses (ring vertex and index buffers,
render target), not in draws, batches or state (the CPU clip and cull decisions are made with each pass's matrices). This row counts it.

## Start
`sh 3ds/tools/wt_new.sh INTERP2M`. PM_ROW=INTERP2M PM_LANE=INTERP2M.

## Touch only
3ds/src/pm_gpu.c: a new cold section above sr_take (:2030), one call in sr_take, the cursor resets in pm_gx_room and pm_gx_drain,
the frame finish after the frame-end sr_take; 3ds/src/pm_nusys.c: a flag set in pm_frame_walk when f->flags has PMF_REPL.

## Exact edits
1. pm_nusys.c: `int pm_icmp_kind; /* INTERP2M: the kind of the frame being built, read at its frame end: 0 plain, 1 L (t = 0.5),
   2 K (key) */`. Set it where each frame starts and never clear it in the walk: first line of pm_frame_walk (pm_nusys.c:132)
   `pm_icmp_kind = (f->flags & PMF_REPL) ? 1 : 0;`, first line of pm_key_walk (before its pm_gpu_begin) `pm_icmp_kind = 2;`. The L
   frame ends later in wq_step_end (pm_main.c:473 -> pm_gpu_end, pm_gpu.c:2583 pm_gpu_end_w), before the key walk starts (posted after
   it, pm_main.c:502); the K frame ends inside pm_key_walk (pm_nusys.c:207). pm_gpu.c reads it as `extern int pm_icmp_kind;`.
2. pm_gpu.c (switch icmp, cold, a 256 KB buffer from malloc at first use). Entry layout: type 0x01 and size in bytes, as the repo's
   sr_replay uses them (pm_gpu.c:2111-2135; libctru gx.h agrees). The GX queue is run and cleared mid-frame by pm_gx_room
   (:2179-2187) and pm_gx_drain (:2196-2204); both call sr_take (:2030-2035) before the clear, and so does the frame end; sr_take
   returns early in a mono frame. Add a cold `icmp_take(gxCmdQueue_s* q)` with its own cursor `static unsigned ic_from;` and call it as
   the first line of sr_take (before `if (!sr_on)`), only when PMDL("icmp") and pm_icmp_kind != 0: if q->numEntries < ic_from (cleared without a take) count
   `lost` and set ic_from = 0; read entries ic_from .. numEntries - 1 and set ic_from = numEntries. Where sr_take's callers set
   `sr_from = 0` after a clear (pm_gx_room, pm_gx_drain) also set ic_from = 0; at the frame end (pm_gpu_end_w), after the frame-end sr_take,
   finish the frame by pm_icmp_kind: 1 keep the copy as L; 2 compare with the kept L, then drop it (no L kept: count `nol`, skip); 0 drop; then
   set ic_from = 0 and pm_icmp_kind = 0. icmp_take fills the L copy when pm_icmp_kind is 1 and compares when it is 2. A command list entry is type (data[0] & 0xff) == 1
   (GX_CMD_PROCESS_COMMAND_LIST): data[1] the list address, data[2] its size in bytes; other types are skipped. In each list walk the
   PICA packets: word pairs [param][header]; header bits 0-15 register,
   20-27 extra word count n, bit 31 consecutive; n extra words follow; a packet with odd 1 + n is padded by one word. For the L frame
   copy (register, value) of every written word into the buffer in order. For the next K frame compare in order: count words of L and
   of K, equal words, and differing words by class: U float uniform (registers 0x2C0-0x2C8), A attribute and index buffers
   (0x200-0x227), T render target and framebuffer (0x110-0x12F), O other. A length mismatch counts in `lenmis` and stops the compare.
   Every 300 compared pairs: `pm_log_hw("icmp f%u pairs %u wl %u wk %u diff U %u A %u T %u O %u lenmis %u lost %u nol %u", ...)` and the 8 most
   frequent O registers with counts in a second line `icmpo f%u r%03x:%u ...`; then zero.
3. Scenario and bench icmp_mac01: copy of spot_mac01_fps60_det without shots, debug `warp=mac_01:0 pos=0:0:200 fps60 icmp
   loglevel=debug det`, until `icmp f\d+ pairs`, cap 40. Same for icmp_btl (battle_start_f182 debug + fps60 icmp). Commit with the code.

## Gate (measurement)
Build, memcheck OK, nd_host PASS. One list, 2 Azahar starts: `bench.py run icmp_mac01,icmp_btl --row INTERP2M --why "how the t = 0.5
and key walks differ in command words"`. Hand back the icmp and icmpo lines as printed. Stop after it.

## At the end
Delete this row file in the completing commit.
