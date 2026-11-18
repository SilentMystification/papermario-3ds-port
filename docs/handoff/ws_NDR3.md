# ws_NDR3 (Haiku): a message glyph takes the snap: a native draw record that keeps the arm (stage S4; counter: none of C1-C12)
Designer: oracle FIN. WAIT: branch from main after the NDR0F2, REN8 and NST2K merges (nd_snap_render; nd_rec.c of REN8 and NST2K: NST2K edits the line under :672).
Worktree build3ds/wt/NDR3, branch NDR3. PM_ROW=NDR3 PM_LANE=NDR3. Edit only: 3ds/src/nd_rec.c, 3ds/include/nd_api.h (1 line), src/msg_draw.c (1 site), 3ds/src/nd_direct.c (nd_glyph_draw only).
Parallel with NDR0G1 and NDR0G2 (no shared file). NDR1C waits for this row (nd_rec.c).
## Facts (code read, main 40484ae6d1; find each line by its text)
- stream_run clears nd_snap_arm before every command but a jump, a texture rectangle and nd_nr_run (nd_rec.c:672). So a native draw runner never sees a snap.
- nd_snap_mesh retakes the snap when nd_rec_cmds changed (nd_snap.c skip test). nd_rec_cmds (nd_rec.c:648) counts every NDC_NAPI and NDC_TEXRECT, so a snap per glyph
  or per rectangle would be about 640 bytes each (GUESS: NdSnapChk with NdCombIn, NdCombOut and the NDR0F1/NDR0G1 blocks).
- The glyph record (msg_draw.c:1861) runs msg_glyph_run, then nd_glyph_draw (nd_direct.c:214-241) under the walker state of the printer setup: box_fold_sync, sync_render, ensure_tile(0).
  The glyph texture is its own bind (nd_tex_bind_glyph): binds stay as they are.
## Edits
1. nd_rec.c after nd_rec_napi: a copy of it named `nd_rec_napi_s`, with `ndc_snapc++;` after `ndc_n[NDC_NAPI]++;` and `NDC_HDR(NDC_NAPI, units, 1)` (arg 1 = takes the armed snap).
   Above it: `static unsigned ndc_snapc; /* NDR3: the records of nd_rec_napi_s (not a command of the snap memo) */`.
2. nd_rec.c :672. Before: `... && (op != NDC_NAPI || p[1] != (uint32_t)(uintptr_t)nd_nr_run)) nd_snap_arm = 0; } /* NDR0A ... */`
   After: `... && (op != NDC_NAPI || (p[1] != (uint32_t)(uintptr_t)nd_nr_run && !(h & 1u)))) nd_snap_arm = 0; } /* NDR0A ...; NDR3: a snap-taking record keeps the arm */`
3. nd_rec.c nd_rec_cmds (:648): `ndc_n[NDC_NAPI]` becomes `ndc_n[NDC_NAPI] - ndc_snapc`, and delete `ndc_n[NDC_TEXRECT] + ` (a rectangle or a glyph after one with the same inputs takes no new snap).
4. nd_api.h after the nd_rec_napi line: `void nd_rec_napi_s(NdNapiRun run, const void* rec, unsigned units); /* nd_rec.c, core 1, NDR3: as nd_rec_napi; the runner takes the armed snap */`
   and `void nd_snap_mesh(void); /* nd_snap.c, core 1: the snap of the next draw (NDR3: also before a glyph record) */`.
5. msg_draw.c :1861. Before: `        nd_rec_napi(msg_glyph_run, &rec, sizeof rec / 8u);` After: `        nd_snap_mesh(); /* NDR3 */` and `        nd_rec_napi_s(msg_glyph_run, &rec, sizeof rec / 8u);`
6. nd_direct.c: `#include "nd_snap.h"` after `#include "nd_at.h"`. nd_glyph_draw: first line `const unsigned char sn = nd_snap_arm; /* NDR3: the snap of this glyph */`.
   Before (:223): `    sync_render();` After: `    if (sn) { nd_snapchk_tri(); nd_snap_render(); } else sync_render(); /* NDR3: the compare and combine, then the render state of the snap */`
   No other line of nd_direct.c changes (NDR2 owns nd_pso_decide .. nd_direct_end).
## Steps and tests (output to build3ds/out/, read the last 20 lines; 1 build, 2 Azahar starts)
1. Edits 1-6. `sh 3ds/tools/nd_host_test.sh` PASS (stunit.sh:12 seds `p[0] = NDC_HDR(NDC_NAPI, 1, 0);` at nd_rec.c:420: do not touch that line). Commit `NDR3 1: glyphs take the snap`.
2. `python 3ds/tools/bench.py base wscmb_pause_drec,wscmb_fsel_drec,nimgm_strip_drec` (merge-base, not charged). `sh 3ds/tools/rowloop.sh --row NDR3 --bench wscmb_pause_drec,wscmb_fsel_drec,nimgm_strip_drec`: LOOP PASS, 3 byte-equal.
3. `python 3ds/tools/bench.py run nimgm_strip_snapchk,wscmb_pause_snapchk --row NDR3 --why "NDR3 snapchk"`; `grep -a snapchk` of both logs > build3ds/out/ndr3_snapchk.txt. Pass: bad 0; recs above 0.
Expected: byte-equal (GUESS: the snap combine and render equal the walker values, snapchk proves it). A build after a partial shape cache: `PM_CACHE=off sh 3ds/build.sh`.
Fix rights: the listed files; compile errors (an include of nd_snap.h in nd_direct.c). A DIFF, a crash, a bad above 0: stop, commit the outputs, send the paths to the oracle with SendMessage. Wait.
Stop: last commit `NDR3 2: gate. 3 benches byte-equal; snapchk bad <2>` deletes this page. One SubagentHandback.
