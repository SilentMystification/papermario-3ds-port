# ws_NAT5D (Haiku): census of the core 0 fallback unit runs (the direct draws that hand their units to nd_nr_exec). Debug lines only, no draw change. Counter: none (data for the NAT5D deletion design). c0gate unchanged.
Designer: oracle NATIVE2. WAIT: none. Start commit main. Worktree build3ds/wt/NAT5D, branch NAT5D. PM_ROW=NAT5D PM_LANE=NAT5D.
Edit only: 3ds/src/nd_direct.c, 3ds/src/pm_gpu.c (drec_end only), src/hud_element.c, src/msg_draw.c, src/screen_overlays.c, src/sprite.c. No other row edits these files now.
## Facts (main 9a57fd4f65)
- FACT core 0 runs units with nd_nr_exec in 7 fallback places: hud_rect_run (hud_element.c:555-558, why = hud_rect_direct), the cplx quad (hud_element.c:639-640), msg_glyph_run (msg_draw.c:1745-1747), the transition stencil (screen_overlays.c:128-130), spr_state_run (sprite.c:279-285, every call, no direct path), nd_fill_walk (nd_direct.c:216-223), nd_trect_run (nd_direct.c:322-325).
- FACT the refusal tests read core 0 state: nd_direct_ready (nd_direct.c:33-38: target role, nd_tgt, m1.valid, pm_tex_aux_off_idx) and nd_direct_bind (:44-50: omode and geom of nd_ws). So core 1 cannot make these choices today.
- FACT drec_end (pm_gpu.c:628-638) writes the end lines of every drec run (c0src, epchk).
## Edits (new lines end with `/* NAT5D */`)
1. nd_direct.c, after the includes:
```c
unsigned nd_c0fb_n[8]; /* NAT5D: core 0 fallback unit runs by source: 1 hud, 2 cplx, 3 glyph, 4 tsten, 5 spr, 6 fill, 7 trect */
void nd_c0fb_note(unsigned src, unsigned why) { static uint64_t seen[8]; src &= 7u; nd_c0fb_n[src]++; /* NAT5D: one line per new source and why */
    if (why < 64u && !((seen[src] >> why) & 1u)) { seen[src] |= 1ull << why; pm_log_evt("c0fb src %u why %u f%u", src, why, pm_frame_count()); } }
```
   (Add `#include "pm_port.h"` or the prototypes of pm_log_evt and pm_frame_count if nd_direct.c has none.)
2. The calls (each with the prototype `void nd_c0fb_note(unsigned src, unsigned why); /* NAT5D: nd_direct.c */` at file scope in the src files):
   - hud_element.c:555: `{ const int why = ...; if (!why) return; nd_c0fb_note(1u, (unsigned)why); if (why != 5) nd_direct_fallback((unsigned)why); }`.
   - hud_element.c, directly before `cplx_quad_emit(r, &nrp);` (:639): `nd_c0fb_note(2u, 0u);`.
   - msg_draw.c, directly before `msg_glyph_emit(r, &nrp);` (:1746): `nd_c0fb_note(3u, (unsigned)(30 - nd_direct_ready()));` (prototype `int nd_direct_ready(void);` if missing; 29 means ready, so the refusal was the width test).
   - screen_overlays.c, directly before `tsten_emit(r, &nrp);` (:129): `nd_c0fb_note(4u, 0u);`.
   - sprite.c spr_state_run, before `nd_nr_exec(` (:284): `nd_c0fb_note(5u, 0u);`.
   - nd_direct.c nd_fill_walk, before `nd_fill_emit(` (:222): `nd_c0fb_note(6u, k);`.
   - nd_direct.c nd_trect_run, before `trect_emit(` (:324): `nd_c0fb_note(7u, !ok ? 1u : nd_direct_ready() != 1 ? (unsigned)(30 - nd_direct_ready()) : 3u);` (1 window, 31-34 ready, 3 bind).
3. pm_gpu.c drec_end, after the epchk line (:635): `{ extern unsigned nd_c0fb_n[8]; pm_log_evt("c0fb f%u hud %u cplx %u glyph %u tsten %u spr %u fill %u trect %u", pm_frame_count(), nd_c0fb_n[1], nd_c0fb_n[2], nd_c0fb_n[3], nd_c0fb_n[4], nd_c0fb_n[5], nd_c0fb_n[6], nd_c0fb_n[7]); } /* NAT5D */`.
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, 1 build, 8 Azahar starts of 8)
1. Edits 1-3. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat5d_host.txt 2>&1`: PASS. Commit `NAT5D 1: c0fb census of the core 0 fallback unit runs`.
2. `PM_LANE=NAT5D sh 3ds/build.sh > build3ds/out/nat5d_build.txt 2>&1`.
3. For B in spot_mac01_drec, entcape_kmr03_drec, fxparts_drec, wscmb_pause_drec, wscmb_fsel_drec, curt_title_drec, imgovl_drec, miscp_drec: `PM_ROW=NAT5D PM_LANE=NAT5D python 3ds/tools/bench.py run <B> --row NAT5D --why "NAT5D: c0fb census" > build3ds/out/nat5d_run_<B>.txt 2>&1`.
4. `grep -H "c0fb " build3ds/azt/logs/*_drec.txt > build3ds/out/nat5d_c0fb.txt`.
## Gate
Host PASS. Build exit 0. Every run exit 0 and one `c0fb f` sum line per bench. No byte compare (log lines only). Fix rights: compile errors, missing prototypes or includes. A run fault or a missing sum line: stop, send the output path to oracle NATIVE2 with SendMessage, wait.
## Hand-back
Last commit `NAT5D 2: census. c0fb lines of 8 drec benches` with `git add -f build3ds/out/nat5d_c0fb.txt`. It deletes this page. One SubagentHandback: hash, the content of nat5d_c0fb.txt.
