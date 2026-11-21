# ws_MTX1: reader stamps replace the blanket drain at ROM reads and loads (Haiku, oracle NATIVE22, plan_maptrans.md 3, starts after the user's yes)
Objective: a write into game RAM waits only for the queued entries that read game RAM (frame walks, list prebuilds), not for the whole queue.
Counter: none of C1 to C12. Console metric: xfer5 dr waits and xfer4 c max of the load step (GUESS -60 to -115 ms). Start: main after the MTX0 merge.
## Edits (directory order, Edit tool, all new lines marked MTX1)
1. 3ds/include/pm_stamp.h (new): `#pragma once` and `static inline int pm_stamp_due(unsigned done, unsigned s) { return (int)(done - s) >= 0; }`.
2. 3ds/include/pm_port.h:119 (after pm_wq_post): declare pm_wq_post_reader (the pm_wq_post arguments), `unsigned pm_walk_stamp(void);`,
   `void pm_walk_until(unsigned s);`, `void pm_walk_read(const char* why, unsigned s);`, `extern unsigned pm_walk_rseq, pm_mt_norseq;`.
3. 3ds/src/nd_wq.c: `#include "pm_stamp.h"`. :33 WqEnt `unsigned frame, rseq;`. :39 add `WQB_READ = 3`. After :37 add
   `static volatile unsigned wq_r_push, wq_r_done, wq_r_want; unsigned pm_walk_rseq, pm_mt_norseq;`.
   - :121 wq_put becomes `wq_put_r(fn, arg, size, unsigned r)` with `e->rseq = r;` after `e->frame = pm_frame_n;`. New `wq_put(fn, arg, size)` calls `wq_put_r(..., 0u)`.
   - :68-71 pm_wq_pump_one: `pm_walk_side++; pm_walk_rseq = e->rseq;`, fn, then after the barrier `if (e->rseq) wq_r_done = e->rseq; pm_walk_rseq = 0;` before `wq_head = h + 1u;`.
   - :83 pm_wq_pump_blocked: `else if (b == WQB_READ) { while (!pm_stamp_due(wq_r_done, wq_r_want) && pm_wq_pump_one()) {} }`.
   - :153-162 pm_wq_frame: logic `wq_put_r(wq_frame_run, f, sizeof *f, ++wq_r_push);`, serial `wq_main_direct_r(wq_frame_run, (void*)f, ++wq_r_push);`. Add after :144:
```c
static void wq_main_direct_r(void (*fn)(void*), void* arg, unsigned r) { pm_lg_quiesce(); wq_pump(); pm_walk_rseq = r; wq_main_direct(fn, arg); pm_walk_rseq = 0; wq_r_done = r; }
void pm_wq_post_reader(void (*fn)(void*), void* arg, unsigned size) { /* MTX1: an entry that reads game RAM */
    if (pm_wq_on_logic()) { wq_put_r(fn, arg, size, ++wq_r_push); return; }
    if (pm_walk_side) { pm_walk_side++; fn(arg); pm_walk_side--; return; }
    wq_main_direct_r(fn, arg, ++wq_r_push); }
unsigned pm_walk_stamp(void) { return wq_r_push; } /* every reader entry posted so far */ static int c_rdone(void* c) { (void)c; return pm_stamp_due(wq_r_done, wq_r_want); }
void pm_walk_until(unsigned s) { /* MTX1: return when every reader entry up to s ran */
    if (pm_wq_on_logic()) { pm_wq_dr++; if (pm_stamp_due(wq_r_done, s)) return;
        pm_wq_drw++; wq_r_want = s; __sync_synchronize(); wq_block(c_rdone, NULL, WQB_READ); return; }
    if (!pm_walk_side) { pm_lg_quiesce(); wq_pump(); } }
```
4. 3ds/src/nd_walk.c:470: pm_walk_drain becomes `static void walk_wait(const char* why, int full, unsigned s)`. `pm_wq_idle();` becomes
   `if (full) pm_wq_idle(); else pm_walk_until(s);`. Add `void pm_walk_drain(const char* why) { walk_wait(why, 1, 0u); }` and `void pm_walk_read(const char* why, unsigned s) { walk_wait(why, 0, s); }`.
5. Call sites. Before (pm_map.c:395): `    pm_walk_drain("map");`. After: `    pm_walk_read("map", pm_walk_stamp()); /* MTX1: the reader entries only */`.
   The same at pm_os.c:500 ("rom"), pm_map.c:136 ("decode"), pm_map.c:368 ("mode"). Keep pm_walk_drain at pm_map.c:154, pm_main.c:297, background_gfx.c:263.
6. 3ds/src/pm_xfer.c:232. Before: `pm_wq_post(wq_xh, &q, sizeof q);`. After: `pm_wq_post_reader(wq_xh, &q, sizeof q); /* MTX1: the prebuild reads the shape lists */`.
7. 3ds/src/pm_tex.c:1384 (load_impl, before the ROMDR block): `if (!pm_walk_rseq && pm_mt_norseq++ == 0u) pm_log_e("mtx1: texture source read outside a reader entry f%u", pm_walk_frame);`.
   pm_phase.c:729: the format becomes `prof f%u w%u romrace %u norseq %u` with pm_mt_norseq.
8. 3ds/tools/pm_stamp_test.c and 3ds/tests/host.d/stamp.sh (model: pm_maplag_test.c, host.d/maplag.sh). Cases (done, s, due): (5,5,1) (5,6,0) (6,5,1)
   (0,0xffffffff,1) (0xffffffff,0,0) (2,0xfffffffe,1). Output `stamp ok` or `stamp FAIL <case index>`. Seed `s/>= 0/> 0/` must print `stamp FAIL 0`.
## Gate (1 build, 2 runs and the bases)
- Build, nd_host_test.sh (timeout 600000) PASS with the stamp unit, c0gate 0/0/0. Bases `--commit $(git merge-base HEAD main)`: xfer_map_shots, spot_mac01_fps60_det.
- Run both (`--row MTX1 --why "MTX1 stamps"`). Pass: both cmp N/N byte-equal (det runs are serial, so every stamp is due at once, nd_wq.c:146-150).
- capture_check `--row MTX1 --proof "xfer4 map n1,norseq 0"` with the MTX0 capture words: CAPTURE CHECK OK. A norseq above 0 is a fault: SendMessage the oracle.
- Fix only your own edits. Any DIFF or tool fault: SendMessage the oracle the output path. Fallback: the 4 call sites go back to pm_walk_drain.
- One commit, then delete this page. SubagentHandback: hash, both cmp lines, stamp test line, CAPTURE line, xfer4 and first 3 xfer5 lines of xfer_map_steps.
