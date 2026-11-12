# Row S5TRECT: one shared "load tile + texture rectangle" native record (helper for the S5 call-site rows)

(Model: Haiku. Every line of the record and runner is given below. It copies NAPI2B's hud_rect_direct and hud_rect_run. Call-site
rows S5TIMG and S5TTL use it, and later the menu rows.)

## Why
Most S5 call sites are pairs: gDPLoadTextureTile* (or LoadTextureBlock) then gSPTextureRectangle or gSPScisTextureRectangle, after
state ops that stay in the stream. One record type for that pair lets each call-site row be a pure pattern edit.

## Start
`sh 3ds/tools/wt_new.sh S5TRECT`; PM_ROW=S5TRECT PM_LANE=S5TRECT. Merge rule: this row appends to the end of nd_api.h and
nd_direct.c. No other row may run with an append there (the orchestrator checks: S5CHUD, S2W and S1PREV wait).

## Exact edits
1. 3ds/include/nd_api.h, at the end before `#endif`:
   ```
   /* S5TRECT: one texture window load plus one texture rectangle (the rect words as gbi.h writes them). 13 units. */
   typedef struct NdTRect {
       const void* img; const void* pal; uint32_t pal_n, fmt, siz, width, height;
       int32_t uls, ult, lrs, lrt; uint32_t cms, cmt, masks, maskt;
       uint32_t w0, w1, hs, hd;
       uint32_t cw0, cw1, oml, omh, prim, fill;
   } NdTRect;
   void nd_trect_run(const uint32_t* u);
   ```
   `fill` is padding to whole units. Check that `sizeof(NdTRect) % 8 == 0` with a `_Static_assert`. Units = sizeof / 8.
2. 3ds/src/nd_direct.c, at the end: nd_trect_run, written like hud_rect_direct plus hud_rect_run (src/hud_element.c):
   a. Window, with the HUD arithmetic. For siz 4b: sh = 1, stride = width >> 1. Else: sh = 2, stride = width.
      `lu = uls << sh, ll = lrs << sh;`
      `ok = nd_tex_win_desc(&d, img, stride, fmt, siz, wrap{cms, cmt, masks, maskt}, lu >> 2, ult, ((ll - lu) >> 2) + 1,
      lrt - ult + 1, uls << 2, ult << 2, lrs << 2, lrt << 2, pal, pal_n)`.
   b. pso: `NdPsoN64 pd = { .cw0 = cw0, .cw1 = cw1, .oml = oml, .omh = omh, .omh_m = (3u << G_MDSFT_TEXTLUT) |
      (3u << G_MDSFT_TEXTFILT) };` Use the field names of the NdPsoN64 struct in nd_api.h. Read them first and keep the HUD's
      order of the assignments.
   c. Direct: `if (ok && nd_direct_ready()) { NdPso id = nd_pso_n64(&pd); if (nd_direct_bind(id, prim, nd_direct_env())) {
      nd_direct_tex(&d); nd_direct_rect(w0, w1, hs, hd); nd_direct_end(); return; } }`.
   d. Fallback into `static Gfx buf[16]`:
      - a switch on siz with the literal macro forms (gbi.h pastes the siz token): 4b `gDPLoadTextureTile_4b(p++, img, fmt,
        width, height, uls, ult, lrs, lrt, 0, cms, cmt, masks, maskt, G_TX_NOLOD, G_TX_NOLOD)`; 8b, 16b and 32b
        `gDPLoadTextureTile(p++, img, fmt, G_IM_SIZ_8b / 16b / 32b, width, height, ...)`;
      - then the rectangle as `ND_GX(p++, w0, w1); gImmp1(p++, G_RDPHALF_1, hs); gImmp1(p++, G_RDPHALF_2, hd);`
        (nd_gbi.h:206-209 form);
      - then `nd_walk_state((const uint32_t*)buf, (unsigned)(p - buf));`.
      nd_direct.c needs the gbi macros: include what hud_element.c includes for them (check with a build).
   e. Debug count: `static unsigned nfb; if (!ok || ...) { if (nfb++ < 16) pm_log_d("trect fallback f%u", pm_frame_count()); }`.
      Declare pm_log_d and pm_frame_count extern if needed.
3. No call site in this row. The row is the helper and a host test.
4. 3ds/tools/nd_direct_test.c (exists since NAPI2B): add one case. An NdTRect for a 32 x 32 RGBA16 window gives the same
   NdBindDesc from nd_tex_win_desc as the HUD case with the same arguments.

## Gate
- Build, memcheck OK; `bench.py run nd_host --row S5TRECT --why "S5TRECT helper"`: PASS.
- spot_mac01_det byte-equal (1 start, base at the merge-base). Nothing calls the helper yet, so this only checks the build.
- Budget 2 Azahar starts.

## Stop
A build error you cannot fix by the steps above, or a failed gate: stop, commit, hand back. Delete this row file in the
completing commit.
