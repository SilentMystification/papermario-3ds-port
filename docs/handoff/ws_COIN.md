# ws_COIN: field items and coins draw as a black coin shape (Haiku). Start after Merge HWFIX: Edit 2 changes nd_comb.c (HWFIX owns it).
Objective: a combine that is not the shaded sprite combine ends the shaded sprite mode (dual_on). Counters C1..C12: no change. Model: Haiku.
Cause (FACT, code read): a shaded sprite sets dual_on = 1 (sprite_shading.c:419-426, nd_tex_set.c:508). Only a tile 0 size (nd_tex_set.c:155) or
nd_direct_bind (nd_direct.c:113) clears it. With sprite shading on, the item draw sets no tile 0 size (src/item_entity.c:979-993, :1005-1017): dual_on
stays 1, decide is skipped (nd_decide.c:302), dc_legacy gives mode 2 (nd_decide.c:122), the TEV gives texel * vertex colour (nd_tmpl.c:245-247), the
vertex colour is (0,0,0) (src/item_entity.c:77-89). Unshaded branch: the comma at nd_comb.c:191 skips the sync after the clear at :1000 (HWFIX fixes it).
Files in order: 3ds/include/nd_dual.h (new), 3ds/src/nd_comb.c, 3ds/tools/nd_dual_test.c (new), 3ds/tests/host.d/coin.sh (new). One commit.
## Edit 1, new file 3ds/include/nd_dual.h
    #ifndef ND_DUAL_H
    #define ND_DUAL_H
    static inline int nd_dual_keep(unsigned comb_a, unsigned comb_c) { return comb_a == 1u && comb_c == 2u; } /* COIN: dual_on (nd_comb.c) stays on only while cycle 1 reads TEXEL0 in a and TEXEL1 in c, the tex_shaded0 test (nd_tex_set.c). Fields after the op_setcombine rename. 1 = G_CCMUX_TEXEL0, 2 = G_CCMUX_TEXEL1. */
    #endif
## Edit 2, nd_comb.c. Before: `#include "nd_scsig.h"`  After: that line, then `#include "nd_dual.h"`. In op_setcombine, before `            box_folded = 0;` (one match) add:
            if (dual_on && !nd_dual_keep(nd_ws.tst1.comb_a, nd_ws.tst1.comb_c)) dual_on = 0; /* COIN: a combine that is not the shaded sprite combine ends the shaded sprite mode (a stale dual_on skipped decide: item_entity.c coins drew texel * vertex colour 0) */
## Edit 3, new file 3ds/tools/nd_dual_test.c. Line 1: `/* COIN: host test of nd_dual_keep (nd_dual.h) and the item combine choice after a shaded sprite. Prints "coin ok 4/4" or "coin FAIL <case>". */`, then lines 7-30 of 3ds/tools/nd_decide_test.c (includes, f32tof24, unpack8), then `#include "nd_dual.h"`, then:
    /* 8 = plan with texel-only output, 9 = plan not texel only, else the legacy mode. dual_on is 1 before the combine; force keeps it. */
    static int pick(uint32_t cw0, uint32_t cw1, uint32_t omh, int use_t1, int force) {
        static const float tx[4] = { 0.8f, 0.6f, 0.2f, 1.f }, sh[4] = { 0.f, 0.f, 0.f, 1.f }; NdPsoDesc d; NdPsoOut o; uint32_t t[16]; float out[4];
        memset(&d, 0, sizeof d); d.cw0 = cw0; d.cw1 = cw1; d.omh = omh; d.tex_on = 1; d.use_t1 = (uint8_t)use_t1; d.t1_valid = (uint8_t)use_t1; d.comb_set = 1; nd_comb_tst(cw0, cw1, use_t1, 0, t); d.dual_on = (uint8_t)(force || nd_dual_keep(t[0], t[2]));
        nd_pso_build(&d, &o); if (!o.comb.plan_on) return o.comb.leg;
        pm_comb_colours(&o.comb.plan, 0, 0, 0, 0, 0); pm_comb_eval(&o.comb.plan, tx, tx, sh, out);
        for (int i = 0; i < 4; i++) { if (out[i] - tx[i] > 0.002f || tx[i] - out[i] > 0.002f) return 9; } return 8;
    }
    int main(void) {
        if (pick(0xfcffffffu, 0xfffcf279u, 0u, 0, 0) != 8) { puts("coin FAIL decal"); return 1; }          /* G_CC_DECALRGBA x2, 1 cycle (item_entity.c:956) */
        if (pick(0xfcffffffu, 0xfffd7438u, 1u << 20, 1, 0) != 8) { puts("coin FAIL window5"); return 1; } /* PM_CC_WINDOW_5, G_CC_PASS2, 2 cycle (sprite_shading.c:433) */
        if (!nd_dual_keep(1u, 2u) || nd_dual_keep(15u, 31u)) { puts("coin FAIL keep"); return 1; }        /* PM_CC_50/51 keep it; DECALRGBA (15, 31) ends it */
        if (pick(0xfcffffffu, 0xfffcf279u, 0u, 0, 1) != 2) { puts("coin FAIL cause"); return 1; }          /* stale dual_on: legacy mode 2 */
        puts("coin ok 4/4"); return 0; }
## Edit 4, new file 3ds/tests/host.d/coin.sh: lines 1-4 of 3ds/tests/host.d/decide.sh (shebang, comment, F=, pm_comb.c object). Line 2 becomes
`# COIN: nd_dual_keep and the item combine choice. The seed keeps dual_on: decal fails first.` Line 4 writes /tmp/coin_comb.o. Then:
    mkdir -p /tmp/coin_seed; sed 's/return comb_a == 1u \&\& comb_c == 2u;/(void)comb_a; (void)comb_c; return 1;/' 3ds/include/nd_dual.h > /tmp/coin_seed/nd_dual.h
    gcc -m32 -w -I /tmp/coin_seed $F -o /tmp/coin_seed/t 3ds/tools/nd_dual_test.c 3ds/src/nd_decide.c 3ds/src/nd_tmpl.c /tmp/coin_comb.o -lm || exit 1
    got=$(/tmp/coin_seed/t); if [ "$got" != "coin FAIL decal" ]; then echo "coin seed FAIL: got [$got]"; exit 1; fi
    gcc -m32 -Wall -Wextra -Werror $F -o /tmp/coin_t 3ds/tools/nd_dual_test.c 3ds/src/nd_decide.c 3ds/src/nd_tmpl.c /tmp/coin_comb.o -lm && /tmp/coin_t || exit 1
## Gates (1 build, 2 Azahar runs). Loop: Edits 1-4, gates 1-2, one build, gate 4, one commit.
1. `sh 3ds/tests/host.d/coin.sh` prints `coin ok 4/4`. 2. `sh 3ds/tools/nd_host_test.sh`: ND HOST TESTS PASS. 3. Build rc 0, memcheck OK.
4. B=$(git merge-base HEAD main). `bench.py base spot_mac01_drec --commit $B`, `bench.py run spot_mac01_drec --row COIN --why "COIN drec"`, `bench.py cmp spot_mac01_drec --row COIN --commit $B`: all byte-equal. FACT mac_01 has no sprite shading (mac_01/main.c SetSpriteShading(SHADING_NONE)): no bench draws the fixed path. Gate 1 proves the fix.
Fix a failing test: edit only Edits 1-4. `coin FAIL window5`, `coin FAIL cause` or a gate 4 difference: write the output to build3ds/out/coin.txt, send the path to the oracle (SendMessage). Never change an expected value.
Stop: gates 1-4 pass, one commit; the commit deletes this page and its INDEX.md entry. Hand-back: build3ds/out/handback_COIN.txt (commit, coin line, host, build rc, memcheck margin, cmp line).
