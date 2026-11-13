# Row NPSO1: NdPsoDesc, the complete static pipeline state description (nd_api.h types only)

(Model: Haiku. 60 lines or fewer. Read only this file, CLAUDE.md and what it names.)
## Facts (ORACLE8, main 5a53dd0a2)
- The combine decision reads NdCombIn (3ds/include/nd.h:478-482), filled by comb_in_walker (3ds/src/nd_comb.c:476-482): w0, w1
  (combine words), omode_h, prim, env, prim_lo8 (nd_ws.prim_lod), key_c, key_s, conv_k4, tst[16] (the combiner fields that
  G_SETCOMBINE decodes from w0, w1), leg_mode (the mode the last decision gave, nd_comb.c:150), tex_on, use_t1, t1_valid
  (nd_ws.td[1].valid), dual_on, box_prod, box_pat (= box_prod && !box_folded, nd_comb.c:176), box_folded, comb_set,
  comb_alpha_shade. The decision key also holds pm_comb_vprim (nd_comb.c:470, pm_comb.c:312).
- nd_render_state(omode_h, omode_l, depth_img, NdRender*) (nd.h:344) reads both other mode words and depth_img (the colour image
  is the z buffer, nd_target.c ROLE_DEPTHQ). The geometry mode word is nd_ws.geom (pm_gbi_sticky, nd_comb.c).
- NdPsoDesc holds every input that is pipeline state. Not in it: prim and env (draw colour values, NDN_COL), tst (derived from
  cw0 and cw1), box_pat (derived). NPSO2 proves the decision needs no more (its stop rule).
- NdPso is uint16_t (3ds/include/nd_api.h:14). NdPsoN64 (:19) stays until the family rows move its callers.
## Blocker
HOSTDIR merged. Start: `sh 3ds/tools/wt_new.sh NPSO1`. PM_ROW=NPSO1 PM_LANE=NPSO1 on every call.
## Touch only (places)
3ds/include/nd_api.h: new lines right after `const NdPsoN64* nd_pso_get(NdPso id); /* core 0 */` (:21). New files
3ds/tools/nd_psodesc_test.c, 3ds/tests/host.d/psodesc.sh. Nothing else.
## Exact edits
1. nd_api.h after :21 add:
```
/* NPSO1 (plan_endgame.md v3 section 2): the complete static pipeline state of a draw. gen_pso_tab (NPSO3) decides each
 * description once at build time. A call site states it with ND_PSO_DESC. Colours (prim, env) are draw values, not state. */
typedef struct NdPsoDesc {
    uint32_t cw0, cw1;   /* combine words with the 0xFC byte (G_SETCOMBINE w0, w1) */
    uint32_t omh, oml;   /* other mode H and L, full words */
    uint32_t geom;       /* geometry mode word */
    uint32_t key_c, key_s, conv_k4; /* G_SETKEYR/GB words, G_SETCONVERT k4 */
    uint8_t tex_on, use_t1, t1_valid, dual_on; /* texture on, combine uses tile 1, tile 1 loaded, two-cycle dual texture */
    uint8_t box_prod, box_folded, comb_set, comb_alpha_shade; /* draw_box window product state, combine set, alpha shade */
    uint8_t leg_prev, vprim, zimg, plod; /* mode of the previous decision, pm_comb_vprim, colour image is the z image, prim LOD byte */
} NdPsoDesc;
#define ND_PSO_DESC(name, ...) static const NdPsoDesc name = { __VA_ARGS__ } /* NPSO1: the scan of gen_pso_tab finds this name */
typedef struct NdPsoOut NdPsoOut;  /* NPSO2: the decided TEV, blend and render blocks of one description */
const NdPsoOut* nd_pso(NdPso id);  /* NPSO3: core 0, the table entry of a description id */
```
2. nd_psodesc_test.c: `#include "nd_api.h"`, `#include <stdio.h>`, `#include <stddef.h>`.
   `_Static_assert(sizeof(NdPsoDesc) == 44, "NdPsoDesc 44 bytes");` `_Static_assert(offsetof(NdPsoDesc, tex_on) == 32, "u8 fields at 32");`
   `ND_PSO_DESC(t_desc, .cw0 = 0xFC121824u, .cw1 = 0xFF33FFFFu, .oml = 0x00552048u, .geom = 0x00220405u, .tex_on = 1);`
   main prints `psodesc ok size %u cw0 %08x oml %08x tex_on %u` from t_desc and returns 0.
3. host.d/psodesc.sh (LF): `gcc -m32 -Wall -Wextra -Werror -I 3ds/include -o /tmp/psodesc 3ds/tools/nd_psodesc_test.c && /tmp/psodesc`.
4. Commit: `NPSO1: NdPsoDesc, ND_PSO_DESC and the nd_pso declaration in nd_api.h, host test`.
## Gate (expected values)
1. Seeded fault: write `== 48` in the size assert. `python 3ds/tools/bench.py run nd_host --row NPSO1 --why "NPSO1 seeded fault"`:
   expected a compile error naming "NdPsoDesc 44 bytes", nd_host FAIL.
2. Write `== 44`. Same command, --why "NPSO1 gate": expected `psodesc ok size 44 cw0 fc121824 oml 00552048 tex_on 1`, PASS.
3. `PM_ROW=NPSO1 PM_LANE=NPSO1 sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: build OK, memcheck OK (every device file that
   includes nd_api.h compiles). No Azahar start. Counter: none (design form of plan section 2, used by NPSO2-5).
## Stop and end
A failed gate, a line that does not match the text above, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5).
Delete this row file in the completing commit.
