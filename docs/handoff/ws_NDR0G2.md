# ws_NDR0G2 (Haiku): a mesh triangle binds its tiles from the snap blocks at its epoch (stage S4; counter: none of C1-C12)
Designer: oracle FIN. WAIT: branch from main after the NDR0F2 and NDR0G1 merges (gv_epoch_enter lines of NDR0F2; NdSnapBind and sc_cur.b of NDR0G1).
Worktree build3ds/wt/NDR0G2, branch NDR0G2. PM_ROW=NDR0G2 PM_LANE=NDR0G2.
Edit only: 3ds/include/nd_snap.h, 3ds/src/nd_snap.c (new function at the end), 3ds/src/nd_tex_set.c, 3ds/src/nd_comb.c (dual_bind_load, dual_bind), 3ds/src/nd_geom.c (gv_epoch_enter, 1 weak line).
## Facts (code read, main 497dc0e164; line numbers of main, find each by its text)
- gv_epoch_enter (nd_geom.c:211-214) binds with `ensure_tile(nd_ws.use_t1 ? 1 : 0); dual_bind();` unless nd_dq_on.
- The tile facts are read only in these functions: fb_prev_base (nd_tex_set.c:257-267), tile_bound (:291-293), fb_cand (:331-333), ensure_tile_slow (:335-385),
  ensure_tile (:441-449), glyph_atlas_bound (:565-572), dual_bind_load and dual_bind (nd_comb.c:397-435). tile_texel (:295-328) is not a bind: it stays.
- NDR0G1: sc_cur.b[n] equals nd_ws.td[n] (zero when not valid) and tst1 tile_wrap, tile_sz, tile_shift of tile n at the triangle (snapchk b0 b1: bad 0).
## Edits
1. nd_snap.h after NdSnapBind: `extern const NdSnapBind* nd_bsrc; /* NDR0G2: NULL = the walker tiles; else the 2 snap blocks (nd_snap_bind_run) */` and
   ```
   #define ND_BTD(n) (nd_bsrc ? &nd_bsrc[n].td : &nd_ws.td[n])                  /* NDR0G2: the bind source of tile n */
   #define ND_BWR(n) (nd_bsrc ? nd_bsrc[n].wrap : nd_ws.tst1.tile_wrap[n])
   #define ND_BSZ(n) (nd_bsrc ? nd_bsrc[n].sz : nd_ws.tst1.tile_sz[n])
   #define ND_BSH(n) (nd_bsrc ? nd_bsrc[n].shift : nd_ws.tst1.tile_shift[n])
   ```
   Also `void nd_snap_bind_run(void); /* NDR0G2, nd_snap.c, core 0 */`.
2. nd_tex_set.c: `#include "nd_snap.h"` after `#include "nd_api.h"`; `const NdSnapBind* nd_bsrc; /* NDR0G2 */` above fb_prev_base. nd_comb.c: `#include "nd_snap.h"` after `#include "nd_api.h"`.
3. One mechanical sed per file, on the function ranges only (check each range start and end by the function names first; tile_texel stays):
   `E='s/nd_ws\.td\[(n|0|1)\]\./ND_BTD(\1)->/g; s/nd_ws\.tst1\.tile_wrap\[(n|0|1)\]/ND_BWR(\1)/g; s/nd_ws\.tst1\.tile_sz\[(n|0|1)\]/ND_BSZ(\1)/g; s/nd_ws\.tst1\.tile_shift\[(n|0|1)\]/ND_BSH(\1)/g'`
   `sed -i -E "257,293{$E}; 330,385{$E}; 441,449{$E}; 565,572{$E}" 3ds/src/nd_tex_set.c` and `sed -i -E "397,435{$E}" 3ds/src/nd_comb.c`.
   Before (nd_tex_set.c:442): `if (nd_ws.td[n].valid && tile_bound(n) && !fb_cand(n)) {` After: `if (ND_BTD(n)->valid && tile_bound(n) && !fb_cand(n)) {`
   Before (nd_comb.c:423): `... shift_mul(nd_ws.tst1.tile_shift[1][0]) / shift_mul(nd_ws.tst1.tile_shift[0][0]) ...` After: `... shift_mul(ND_BSH(1)[0]) / shift_mul(ND_BSH(0)[0]) ...`
   Then read the 5 ranges: no `nd_ws.td[` and no `nd_ws.tst1.tile_wrap[`, `_sz[`, `_shift[` is left (tile_texel keeps its own).
4. nd_snap.c at the end: `void nd_snap_bind_run(void) { nd_bsrc = sc_cur.b; ensure_tile(sc_cur.in.use_t1 ? 1 : 0); dual_bind(); nd_bsrc = NULL; } /* NDR0G2: the binds of the armed triangle */`
5. nd_geom.c: weak line next to the NDR0F2 weak nd_snap_render: `__attribute__((weak)) void nd_snap_bind_run(void) { ensure_tile(nd_ws.use_t1 ? 1 : 0); dual_bind(); } /* NDR0G2 */`.
   gv_epoch_enter, the 2 lines in `if (!nd_dq_on) {` become: `if (nd_snap_rpend) nd_snap_bind_run(); /* NDR0G2: the binds of the snap */` and `else { ensure_tile(nd_ws.use_t1 ? 1 : 0); dual_bind(); }`.
## Steps and tests (output to build3ds/out/, read the last 20 lines; 1 build, 2 Azahar starts)
1. Edits 1-5. `sh 3ds/tools/nd_host_test.sh` PASS (no new case: a source switch; the drec compare decides). Commit `NDR0G2 1: mesh binds from the snap blocks`.
2. `python 3ds/tools/bench.py base nimgm_strip_drec,wscmb_pause_drec,spot_mac01_drec` (merge-base, not charged). `sh 3ds/tools/rowloop.sh --row NDR0G2 --bench nimgm_strip_drec,wscmb_pause_drec,spot_mac01_drec`: LOOP PASS, 3 byte-equal.
3. `python 3ds/tools/bench.py run nimgm_strip_snapchk,wscmb_pause_snapchk --row NDR0G2 --why "NDR0G2 snapchk"`; `grep -a snapchk` of both logs > build3ds/out/ndr0g2_snapchk.txt. Pass: bad 0.
Expected: byte-equal (FACT: the same values reach the same calls). A build after a partial shape cache: `PM_CACHE=off sh 3ds/build.sh`. Fix rights: the listed files; compile errors (a missing include, a const cast).
A DIFF, a crash, a bad above 0: stop, commit the outputs, send the paths to the oracle with SendMessage. Wait.
Stop: last commit `NDR0G2 2: gate. 3 benches byte-equal; snapchk bad <2>` deletes this page. One SubagentHandback.
