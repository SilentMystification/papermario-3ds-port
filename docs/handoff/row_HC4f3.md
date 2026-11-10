# Row HC4f3: map shape lists typed by the converter

(Model: Haiku. Oracle batch 10, 2026-11-10. Start: `sh 3ds/tools/wt_new.sh HC4f3`; PM_ROW=PM_LANE=HC4f3 on every build.sh and bench call. CLAUDE.md rules 1-7. No git stash. Foreground waits. Long output to a file, then grep or tail it. Commit after each passed step. Delete this row file in the completing commit. A failed gate or an unexpected fact: stop, commit, hand back the printed values.)

## Goal
nd_shape_conv writes each map list word as its typed unit (nd_op.h ND_X0/ND_X1), file version 2. Every reader of map lists reads through nd_op_unit, so raw and typed words both work until HC4e4. Pictures byte-equal.

## Start state
Worktree build3ds/wt/HC4f3, branch HC4f3 from main 384c404c2. Copy assets/us and ver/us/build into it. HC4e1-e3, HC4f1-f2 merged: static lists typed; map shapes load from romfs .nds with raw list words.

## Cause (oracle code read)
- 3ds/tools/nd_shape_conv.c fix_gfx :143-145 writes the raw w0, w1; main :442 writes version 1.
- Raw readers of map lists: 3ds/src/nd_shape.c dlc_hash :40-47 (ends on raw G_ENDDL only), dlc_build :161 (the copy ends on raw G_ENDDL only), dlc_run_group :349-385 (G_VTX, G_TRI1, G_TRI2, G_GEOMETRYMODE: a typed group draws nothing), sbkw_dlchash :427-429; 3ds/src/nd_seam.c list_decode :331-377 (OP_*); src/world/area_omo/omo_02/barricade.c :46-75; src/world/common/util/GetFirstTriangleNormal.inc.c :33-70 (mim_01/03/06 hint.c); 3ds/tools/nd_seam_test.c fx_gfx :127, compile_soup :205, tedges :261-266.
- No edit (typed-safe now): src/model.c :4457, :4511, :4652 (HC4e3); 3ds/src/nd_dlc.c (dlc_op :12); nd_ent.c (ent_uid :107); nd_rec.c (rec_inline skips map lists); pm_xfer.c (calls dlc_build). Collision reads _hit files, shadows read entity lists.
- nd_walk.c nd_walk_state :365: a raw sync is skipped (nd_op_sync); a typed sync (ND_NOP_A) goes to op_ctl and bumps gv_epoch. Typed map lists hold many syncs.

## Touch only
3ds/tools/nd_shape_conv.c, 3ds/build.sh (gen_shapes), 3ds/src/pm_map.c (shape_load_romfs), 3ds/src/nd_shape.c, 3ds/src/nd_seam.c, 3ds/src/nd_walk.c (nd_walk_state), the two src/world files above, 3ds/tools/nd_seam_test.c, 3ds/tools/nd_host_test.sh (line 27).

## Step 1: readers (commit "HC4f3 readers"). The unit: `uint32_t t[2]; nd_op_unit(w0, w1, t);`, id `t[0] >> 24`, fields from t (VTX, TRI1, TRI2, GEOM, CALL keep the raw fields). Raw G_ENDDL gives NDO_END; raw syncs stay raw.
- nd_shape.c dlc_hash: lines 40-41 become `uint32_t t[2]; nd_op_unit(w0, w1, t); const unsigned op = t[0] >> 24;`; line 47 `if (op == NDO_END) break;`.
- nd_shape.c dlc_build :161: the end test becomes `{ uint32_t t[2]; nd_op_unit(l[i].words.w0, l[i].words.w1, t); if ((t[0] >> 24) == NDO_END) break; }` (the copy keeps the words; nd_dlc_compile reads both forms).
- nd_shape.c dlc_run_group :348-349: `uint32_t t[2]; nd_op_unit(l[j].words.w0, l[j].words.w1, t); u32 w0 = t[0], w1 = t[1]; switch (w0 >> 24) {`; labels G_VTX, G_TRI1, G_TRI2, G_GEOMETRYMODE become NDO_VTX, NDO_TRI1, NDO_TRI2, NDO_GEOM. Bodies unchanged.
- nd_shape.c sbkw_dlchash :427-429: the same unit; the tests become NDO_VTX and NDO_END.
- nd_seam.c: `#include "nd_op.h"` after nd_ops.h. list_decode: after `uint32_t w0 = rd32(p), w1 = rd32(p + 4);` add `{ uint32_t t[2]; nd_op_unit(w0, w1, t); w0 = t[0]; w1 = t[1]; } /* HC4f3: a typed shape list or a raw one */`. OP_ENDDL -> NDO_END; case OP_VTX -> NDO_VTX; OP_TRI1, OP_TRI2 (labels and `op == OP_TRI2`) -> NDO_TRI1, NDO_TRI2; case OP_DL -> NDO_CALL, its branch test `if (w0 & 1u) goto done;`; OP_GEOMETRYMODE -> NDO_GEOM (the OP_NOOP and sync labels stay); add `case NDO_MISC: if ((w0 & 0xFFFFFFu) != (NDO_MISC_NOP << 16)) ok = 0; break;`.
- nd_host_test.sh line 27 (ndseam): add `3ds/src/nd_op.c` after `3ds/src/nd_dlc.c`.
- barricade.c: after the includes `#ifdef TARGET_3DS` / `#define BAR_DL NDO_CALL` / `#define BAR_VTX NDO_VTX` / `#define BAR_END NDO_END` / `#else` / the same names as G_DL, G_VTX, G_ENDDL / `#endif`. DetermineSphericalSize :46-47 under `#ifdef TARGET_3DS`: `uint32_t t[2]; nd_op_unit(dlist->words.w0, dlist->words.w1, t); u32 w0 = t[0]; u32 w1 = t[1];` (`#else` the two old lines). The three compares use BAR_DL, BAR_VTX, BAR_END.
- GetFirstTriangleNormal.inc.c: the same at :33-34, with GFTN_END, GFTN_DL, GFTN_VTX, GFTN_TRI1, GFTN_TRI2 (NDO_END, NDO_CALL, NDO_VTX, NDO_TRI1, NDO_TRI2; `#else` the G_ names) for the five case labels.
- nd_walk.c nd_walk_state: after `unsigned id = q[0] >> 24;` add `if (q[0] == ND_NOP_A) continue; /* HC4f3: a typed sync or no-op, skipped as a raw sync: no op_ctl, no gv_epoch */`.
- nd_seam_test.c: `#include "ultra64.h"`, `#include "gbi_custom.h"`, `#include "nd_op.h"` after nd.h. fx_gfx :127 becomes `fx_w32(dl, ND_X0(w0, w1)); fx_w32(dl + 4, ND_X1(w0, w1)); /* HC4f3: typed as nd_shape_conv writes it */`. compile_soup :205: the end test adds `|| (lw[2 * i] >> 24) == NDO_END`. tedges: after the two memcpy at :261 add `{ uint32_t t[2]; nd_op_unit(w0, w1, t); w0 = t[0]; w1 = t[1]; }`; 0xDF -> NDO_END, 0x01 -> NDO_VTX, 0x05, 0x06 -> NDO_TRI1, NDO_TRI2.
Gate 1: nd_host PASS (`bench.py run nd_host --row HC4f3 --why "..."`); the lane build succeeds.

## Step 2: converter (commit "HC4f3 converter v2")
- nd_shape_conv.c: after `#include <stdint.h>` add `#include "ultra64.h"`, `#include "gbi_custom.h"`, `#include "nd_op.h"`; delete `typedef uint8_t u8;` and `typedef uint32_t u32;` (ultra64.h has both; -m32 keeps u32 at 32 bits). After the ShapeFix typedef: `static u32 n_nop; /* list words G_NOOP, RDPHALF_1/2, SETZIMG: typed as ND_NOP_A */`.
- fix_gfx :143-145 become four lines: `if (op == 0xC0 || op == 0xE1 || op == 0xF1 || op == 0xFE) n_nop++;` / `w32(dl, ND_X0(w0, w1)); /* HC4f3: the typed unit (nd_op.h); op stays the raw op for the walk below */` / `if (target) wptr(s, dl + 4, target);` / `else w32(dl + 4, ND_X1(w0, w1));`.
- main: `w32(hdr + 4, 1);` -> `2`; the header comment :11 says version 2 (list words typed). The last printf: `printf("shapeconv maps %u bytes %u reloc %u nop %u\n", (unsigned)maps, (unsigned)total_bytes, (unsigned)total_reloc, (unsigned)n_nop);`.
- build.sh gen_shapes :68: `want=` adds ` $(git hash-object 3ds/include/nd_op.h)` after the converter hash. :75 the gcc call: `gcc -m32 -O2 -Wall -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I /w/include -I /w/3ds/include -I /w/3ds/stub_include -o /tmp/nd_shape_conv /w/3ds/tools/nd_shape_conv.c` (the oracle compiled nd_op.h with these flags in pm3ds-host:1: no warning).
- pm_map.c shape_load_romfs :66: after the "NDSH" test add `if (hdr[4] != 2 || hdr[5] || hdr[6] || hdr[7]) goto bad; /* HC4f3: version 2, typed list words */`; the comment :55 says version 2.
Gate 2: the build output has `shapeconv maps N bytes B reloc R nop K`. Write K in the commit message.

## Done criteria (all must pass; handback only then)
1. nd_host PASS.
2. For spot_mac01_det, mdl5_iwa10_det, spot_mac00_det: `bench.py base <bench>`, `bench.py run <bench> --row HC4f3 --lane HC4f3 --why "..."`, `bench.py cmp <bench> --row HC4f3`: exit 0, 6/6 byte-equal. Open one shot per scene: the map is drawn. No `shape load fail` line in the logs.
Fail: hand back K and the cmp diff (pixels, box). With K > 0 a typed G_NOOP compiles where the raw one refused the list (nd_dlc.c dlc_op): name that in the handback. No second try on one cause.
Affected scenarios (no others): spot_mac01_det, mdl5_iwa10_det, spot_mac00_det.

## Commands
- Build: PM_ROW=HC4f3 PM_LANE=HC4f3 sh 3ds/build.sh > build.log 2>&1; then grep -n "shapeconv\|error" build.log.
- Budget reset: powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget HC4f3 (in the worktree).

## At the end
Delete this row file in the completing commit. SendMessage main "HC4f3 ready <hash>".
