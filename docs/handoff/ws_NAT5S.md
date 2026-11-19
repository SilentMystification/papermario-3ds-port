# ws_NAT5S (Haiku): core 1 writes two src state records into the stream as units: the flat sprite state (spr_state_run) and a HUD record with no piece (hud_rect_direct why 5). Counter: c0gate calls -1 (sprite.c:284), interp unchanged. Gate: drec byte-equal.
Designer: oracle NATIVE2. WAIT: none. Start commit main. Worktree build3ds/wt/NAT5S, branch NAT5S. PM_ROW=NAT5S PM_LANE=NAT5S.
Edit only: src/sprite.c, src/hud_element.c. NAT5D (merged) added one c0fb line in each; keep them where the code they count stays.
## Facts (main 19781630e1 + NAT5D)
- FACT NAT5D census (build3ds/wt/NAT5D/build3ds/out/nat5d_c0fb.txt, 8 drec runs): spr_state_run runs every flat sprite component (326 to 3852 per run); hud_rect_run why 5 runs in every scene (hud 952 to 5447). Other whys: 1 to 3 hits per run.
- FACT spr_state_run (sprite.c:279-285) expands the record with spr_flat_gfx and runs the units with nd_nr_exec; spr_state_feed (:288-294) builds the same units on core 1 for nd_ds. The record site is sprite.c:319-323. The units are state units, viewports and at most one VTX (no rectangle, no list call).
- FACT hud_rect_direct returns 5 for `r->npc <= 0` (hud_element.c:486) before any core 0 state is read, so core 1 gets the same value at the record site (:885). The units of such a record are state units only (hud_rect_emit writes rectangles only per piece).
- FACT nd_rec_nr (nd_rec.c) applies each unit to nd_ds and writes it to the stream; nd_nr_exec skips ND_NOP_A (nd_walk.c:353).
## Edits (new lines end with `/* NAT5S */`)
1. sprite.c: add `void nd_rec_nr(uint32_t w0, uint32_t w1); /* NAT5S: nd_rec.c */` near the nd_ds_feed prototype (:277). Replace the 2 lines `nd_rec_napi(spr_state_run, st, 4u);` and `spr_state_feed(st); /* NST2C */` and the `st[8]` declaration by:
```c
        uint32_t sb[128]; uint32_t* nrp = sb; /* NAT5S: core 1 writes the units into the stream (the order of the old spr_state_run) */
        spr_flat_gfx(&nrp, vertices, raster, palette, width, height, nullptr, (s32)((u32)alpha & 0xffu), flags & 0xffffffu, vpAlt, vpMain);
        for (const uint32_t* q = sb; q < nrp; q += 2) if (q[0] != ND_NOP_A) nd_rec_nr(q[0], q[1]);
```
   Delete spr_state_run (:279-285) and spr_state_feed (:288-294) with their comments; move the NAT5D `nd_c0fb_note(5u, 0u)` line into the new block (before the loop).
2. hud_element.c: after hud_rect_feed (:563-568) add:
```c
void nd_rec_nr(uint32_t w0, uint32_t w1); /* NAT5S: nd_rec.c */
static void hud_rect_stream(const NdHudRec* r) { static uint32_t sb[1024]; uint32_t* p = sb; hud_rect_emit(r, &p, 0); /* NAT5S: core 1 only: a record with no piece as stream units */
    for (const uint32_t* q = sb; q < p; q += 2) if (q[0] != ND_NOP_A) nd_rec_nr(q[0], q[1]); }
```
   At the record site (:885-886) replace the block line and the `hud_rect_feed((const uint32_t*)&rec);` line by:
```c
        { NdPsoN64 pd; u32 prim; const int hw = hud_rect_direct(&rec, &pd, &prim);
          if (hw == 5) hud_rect_stream(&rec); /* NAT5S */
          else { if (!hw) nd_dir_rec(hud_rect_run, &rec, 8u + 6u * (unsigned)rec.npc, &pd, prim); else nd_rec_napi(hud_rect_run, &rec, 8u + 6u * (unsigned)rec.npc); hud_rect_feed((const uint32_t*)&rec); } } /* NDR2C */
```
   If ND_NOP_A is not defined in hud_element.c, add `#include "nd_op.h"`.
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, 1 build, 8 Azahar starts of 8)
1. Edits 1-2. `grep -n "spr_state_run\|spr_state_feed" src/sprite.c` prints nothing. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat5s_host.txt 2>&1`: PASS. Commit `NAT5S 1: sprite state and no-piece HUD records as stream units`.
2. For B in spot_mac01_drec, entcape_kmr03_drec, fxparts_drec, wscmb_pause_drec: `python 3ds/tools/bench.py base <B> > build3ds/out/nat5s_base_<B>.txt 2>&1`.
3. `sh 3ds/tools/rowloop.sh --row NAT5S --bench spot_mac01_drec,entcape_kmr03_drec,fxparts_drec,wscmb_pause_drec --no-host --why "NAT5S: src state records in the stream" > build3ds/out/nat5s_loop.txt 2>&1`.
4. `grep -H "c0fb\|c0src f\|drec frame\|drec draw .* DIFF" build3ds/azt/logs/*_drec.txt build3ds/out/loop_NAT5S_cmp_*.txt > build3ds/out/nat5s_c0src.txt`. 5. `python 3ds/tools/c0gate.py build3ds/lane-NAT5S/pm_3ds.elf > build3ds/out/nat5s_c0gate.txt 2>&1`.
## Gate (exact)
Host PASS. Rowloop exit 0 (4 drec equal). c0gate calls = main calls - 1 (c0gate counts calls in the source tree it runs in: run it once in the main folder for the base value), interp and state as main. The c0fb sum lines show spr 0 and a lower hud. Fix rights: compile errors, unused static warnings, missing prototypes or includes. A DIFF, a crash or a tool fault: stop, send nat5s_c0src.txt and the first DIFF line to oracle NATIVE2 with SendMessage, wait.
## Hand-back
Last commit `NAT5S 2: gate. 4 drec equal; sprite and no-piece HUD units in the stream` with `git add -f build3ds/out/nat5s_c0src.txt build3ds/out/nat5s_loop.txt`. It deletes this page. One SubagentHandback: hash, the gate line per bench, the c0gate line, the c0fb sum lines.
