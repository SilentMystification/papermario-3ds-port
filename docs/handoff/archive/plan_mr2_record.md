# plan_mr2_record: per-model draw record (path 2 of plan_dl_design.md)

Plan only (Opus). Read at HEAD 037dc0317 (SPLIT0C). No source edits, no runs. Rules: CLAUDE.md 2, 5, 7. hw_guidelines.md.
Memory rule: no culling of any kind. This plan adds no skip of a model, a group or a list. Every word the game emits still has its effect.

## 0. Result first

- The plan changed in one point. The record is split by key. The texture part is keyed per texture handle (one compiled bind per
  handle, shared by every model that uses it). The combine and render-mode part is keyed per model. The texture part is the
  largest SPLIT0C share, so it is the first row (DREC1). DREC1 needs no producer change.
- The record never holds a per-frame input. Matrix, lights, fog, tint and camera still run from the words every frame.
- A mismatch runs the original words through the existing handlers. That is the current path, not a switch.

## 1. The two measurements do not reconcile in scale

| Source | Build, scene | What it measures | Value per frame |
|---|---|---|---|
| Console, phase on | 1588 (bff514be5), area 1 map 2, still camera (pos `M x 11 y 0 z 40`) | opsplit class 0c (G_PM_MODEL), sampled walker op time | 20.3 to 21.1 ms |
| Console, phase on | same | mr0: 90 plain models x 230 to 231 us | 20.7 to 20.8 ms |
| Azahar, phase on | 037dc0317, mac01t_cap1 tour view (same map, other view), 1 run | sp0c: tick periods inside every G_PM_MODEL group | tex 1714, dlc 904, rest 975 us (sum 3593 us) |

- Ratio console / Azahar: 20.7 / 3.59 = 5.8. The catalog factor `console x2.86` does not hold for this code. The older pair
  (mr0 Azahar 37 us, console 230 us, pre-MR1 path) gives 6.2. These are two builds, two views and two timing methods.
  The near-equal ratios are not a basis for a factor.
- Other differences: sp0c counts all model groups (tint, fog, panner, billboard too). mr0 counts plain models only. Each sp0c
  kind change costs one tick read (1.3 us in Azahar), and the number of reads per frame is not printed. So tex and dlc are
  upper bounds. Azahar runs the ARM code through a JIT on the host. Cache misses and memory latency are not the console's.
- The split CAN support: in Azahar, the texture kind is the largest share (48 %), then rest (27 %), then dlc (25 %).
  dlc_box_out skips 27773 of 34513 groups (80 %) in that window. The order of the shares gives the order of the rows.
- The split CANNOT support: any console millisecond value, a console tex share of 48 %, a per-plain-model split, or a saving
  estimate. The console decides. Azahar gives direction only.
- Estimate for the record, as an assumption only: if the console tex share is the same 48 %, tex is 9.9 ms of 0c phase on.
  A compiled bind that removes 50 to 80 % of it gives 5.0 to 7.9 ms phase on [E]. Only the console push at the epic end measures it.

## 2. What a plain model emits (src/model.c appendGfx_model 1384 to 2069)

`G_PM_MODEL n` (1513), PIPESYNC (1518), lights: MOVEWORD NUMLIGHT + MOVEMEM per light (1520 to 1550), G_PM_TEX (1568, 1572),
SETCOMBINE (alpha test only, 1606 to 1611), G_PM_RM (1681, 1745), G_MTX finalMtx (1988), G_DL shape list (2050), POPMTX (2062),
PIPESYNC (2065). The words from G_PM_TEX to G_PM_RM are contiguous and static for a given model. All other words are per frame
or are syncs.

Replay: mg_run (3ds/src/pm_gbi.c 3514) runs the words. G_PM_TEX goes to tb_run (2777), which calls tex_op (2915) for each word of
the handle list (8 to 10 words: SETTIMG, SETTILE, LOADBLOCK or LOADTILE, LOADTLUT, SETTILESIZE, G_TEXTURE, SETCOMBINE,
SETOTHERMODE_H). G_PM_RM goes to rm_apply (2847). G_DL goes to dlc_try (pm_dlc.inc.c 440).

## 3. Record fields

### 3a. Per frame (never in the record; the words run each frame as today)
- Matrix: G_MTX finalMtx -> load_mtx (1881), POPMTX -> gbi_popmtx (3488). The model matrix can change every frame (moving
  platforms, doors). Camera projection and viewport are set outside the group; upload_mvp (426) builds `combined` from both.
- Lights: MOVEWORD NUMLIGHT and MOVEMEM G_MV_LIGHT (gbi_moveword 3495, gbi_movemem 3473). g2_same (2808) already skips equal values.
- Fog: SETFOGCOLOR and MOVEWORD G_MW_FOG (fog class, model.c 1810 to 1811) come after G_PM_RM. Values come from gFogSettings.
- Tint: SETPRIMCOLOR, SETENVCOLOR, SETFOGCOLOR before the header (model.c 1483 to 1494), shroud words after G_PM_RM.
- Tex panner: SETTILESIZE words after G_MTX (model.c 2015 to 2036).
- Billboard: G_MTX of gCameras[].mtxBillboard (model.c 2038 to 2046).
- Shape list: dlc_try with the current `combined` (dlc box test, static groups, batch).

### 3b. Static: compiled bind, per texture handle (DREC1)
`TexSnap tb_snap[128]`, beside tb_list (pm_gbi.c 2763). One entry holds the exit values of the handle list, decoded once:
- tiles written by the list (mask `tmask`, bit per tile 0 to 7): tile_fmt, tile_siz, tile_tmem, tile_pal, tile_wrap[4],
  tile_shift[2], tile_sz[4].
- image state: timg, timg_fmt, timg_siz, timg_stride, ld_valid, ld_x, ld_y, ld_w, ld_h.
- load list (not the ring layout): up to 4 items {kind LOADBLOCK or LOADTILE, tmem, img, bytes or x, y, stride}. Apply runs the
  same slot choice as tex_op (2954 to 2983) on the live ldrec and blk rings. So the ring layout stays a function of the entry state, as today.
- palette: tlut, tlut_n, tlut_pal slots written (mask `pmask`).
- td[0] and td[1] (td[1] only when the list sets the size of tile 1), tile_x0, tile_y0, tile_fx, tile_fy, the td0 inputs of
  the PG1 test (src, w, h, stride, palc, tlut_n).
- G_TEXTURE values (tex_on, tex_ss, tex_ts) when the list has G_TEXTURE.
- SETCOMBINE decoded block (comb_w0, comb_w1, comb_a..comb2_ad, use_t1, box_prod, comb_alpha_shade) when the list has SETCOMBINE.
- omode_h value and mask when the list has SETOTHERMODE_H.
- flags: needs sync_combine (G_TEXTURE, SETCOMBINE, cycle type change, tile 1 size), has LOADTLUT.
- key: `gen` = tb_gen[idx] at build time. Size: about 200 bytes per entry. Only handles in use are touched.

### 3c. Static: per model (DREC2, after the DREC1 console result)
`DrawRec drec[256]`, slot from the header w1. Holds copies of the static words (G_PM_TEX w0 and w1, SETCOMBINE w0 and w1 or 0,
G_PM_RM w1), the decoded SETCOMBINE block, the render-mode list effect (omode_h and omode_l masks and values, geometry mode
and-mask and or-mask), and the needs-sync flag. About 64 bytes per model.

## 4. Key and invalidation

- Texture part: key = handle index (G_PM_TEX w0 & 0x7f) and `tb_gen[idx]`. pm_texbind_set (2764) does `tb_gen[idx]++` and clears
  tb_snap[idx]. It is the only place where a handle list is (re)built (model.c 2160). An entry is valid only when
  `tb_snap[idx].gen == tb_gen[idx]` and `tb_list[idx] == w1` (the check mg_run does today at 3545).
- Model part: key = the header slot plus an equal compare of the static words in the group against the copies in drec. The words
  are emitted every frame, so the compare sees every change (texture variation, render mode, alpha test, fog combine).
- Not used: heap addresses as a key, pm_pdl_heap_gen, any global generation (diag_dlr.md line 39: the heap gen changes every frame).
- pm_dlc_gen stays the key of the shape list only (dlc_try 442 to 446). The record does not hold dlc state.
- Live generations are never copied from the record: st0_ep, tsz_last.ep and cmb_gen_seen take the live value at apply
  (pm_tex_wrap_gen(), pm_tex_epoch(), comb_gen).

## 5. Apply each frame, and the fallback

In mg_run case G_PM_TEX (3544) and in the walker case G_PM_TEX (3662), replace `tb_run(w1)` by `tb_bind(idx, w1)`:
1. tb_list check as today. Fail: return q (mg_run) or the G_DL path (walker), as today.
2. Entry refusal: run tb_run (the original words) when any of these is true: tb_snap not valid; m1.valid or dual_on
   (shaded sprite path); the list has LOADTLUT and td[0] or td[1] is a valid CI tile that the list does not rewrite (V20 effect, 2992 to 3001).
3. Build: when tb_snap is not valid and the entry is not refused, run tb_run, then copy the exit values (3b) and set gen. The
   build refuses (stores `bad`, and the handle always runs tb_run) when the list read a frame buffer (pm_is_cfb) or set dual_on.
4. Apply, in this order (the same decisions tex_op makes, with the record values):
   - tile 0 written: the same_tile test of SETTILE (2931). Not same: pm_gpu_flush, pm_tex_set_wrap, st0_ep = pm_tex_wrap_gen(), st0_ok = 1.
   - omode_h written: the filter test of SETOTHERMODE_H (3228 to 3233). Changed: pm_gpu_flush, pm_tex_set_filter.
   - write the tile, image, palette, G_TEXTURE and combine fields. Run the load-list slot choice on ldrec and blk.
   - tile 1 sized: write td[1], cur_tile = -1 when cur_tile == 1.
   - tile 0 sized: td[0].valid = 0, cur_tile = -1, then the PG1 tail of SETTILESIZE (3165 to 3212). Move that tail into one
     function `t0_set(src, w, h, stride, x0, y0, uls, ult)` that tex_op and the apply both call. So the fast-path choice and tsz_last stay exact.
   - one sync_combine() when the needs-sync flag is set. The original calls it 2 to 4 times. No triangle is queued between those
     calls, so only the last call changes the GPU state.
   - gv_epoch++ once (it is compared for equality only, 1475 and 1508).
5. The matrix, lights, shape list and POPMTX words after G_PM_RM run as today, with the live state.
- Not done by the apply: GP_SET, suntile_t0, imglog (debug lines). gp_on() and dls_on already make mg_run return at once (3515).
- Check tool (debug, stays, like dlccheck): switch `tbcheck`. At each apply, save the texture block, run tb_run, save the
  result, restore, apply, and compare by field group. Count `tbbad[group]`. Print `tbs f<n> hit <n> build <n> refuse <n> bad <g:n ...>`
  in the phase window (beside sp0c, 2144). The check runs side effects twice (flush, wrap). The check run is not a picture run.

## 6. Producer and walker changes

| Row | File | Line | Change |
|---|---|---|---|
| DREC1 | 3ds/src/pm_gbi.c | 2763 to 2774 | TexSnap struct, tb_snap[128], tb_gen[128]. pm_texbind_set: tb_gen[idx]++, clear tb_snap[idx]. |
| DREC1 | 3ds/src/pm_gbi.c | before 2915 | tb_bind (section 5), snapshot build, apply, tbcheck compare. noinline. About 150 lines. |
| DREC1 | 3ds/src/pm_gbi.c | 3165 to 3212 | the td0 tail of SETTILESIZE into t0_set (moved code, unchanged). |
| DREC1 | 3ds/src/pm_gbi.c | 3545, 3663 | tb_run((const Gfx*)w1) -> tb_bind(w0 & 0x7f, (const Gfx*)w1). |
| DREC1 | 3ds/src/pm_gbi.c | 2143 to 2147 | tbs line in sp0_report. |
| DREC1 | src/model.c | none | no producer change. |
| DREC2 | src/model.c | 1504 to 1505, 1513 to 1515 | compute the mr0 plain test without pm_ph_on. Header w1 = slot + 1 (modelID & 255) for a plain model, else 0. |
| DREC2 | 3ds/src/pm_gbi.c | 3514 | mg_run: when header w1 != 0 and the static words equal drec[slot], apply the model part (combine block, masked render-mode writes, pm_gpu_set_mode once, one sync_combine) and step over the words to G_MTX. Else run the words and rebuild drec[slot]. |
| DREC3 | (open) | | only if the console split shows matrix time: skip unpack in load_mtx when the 64 raw Mtx bytes equal the last frame's bytes for that slot. This is a per-frame value compare, not an invalidation key. |

- hw_guidelines: tb_bind is noinline and outside gbi_run_body. TexSnap is read in order (one entry, about 200 bytes, 7 cache lines).
  No division and no double are added. The apply makes at most one flush.
- The row changes shared draw code (pm_gbi.c), so the byte-equal check of section 7 applies.

## 7. Gate and byte-equal scenes (DREC1)

- Scenario: mac01t_mr0 (phase on). Its until `ms0 f899` no longer prints: since CAP1 the windows restart at the map load (frame 209)
  and print at f483 and f783. The row sets the until to the shot line of frame 504 (as mac01s) and edits azt_scenarios.json
  and test_catalog.md in the same commit.
- Build: `python 3ds/tools/gen_map_restore.py`, `sh 3ds/build.sh`, restore 3ds/src/map_restore_table.c.
- Command: `PM_ROW=DREC1 powershell -File 3ds/tools/azt.ps1 mac01t_mr0 -Row DREC1 -Try` (add `tbcheck` for run 1 only).
- Order and starts (7 of 8):
  1. HEAD (037dc0317) mac01t_mr0, yard, forest_mim: baseline shots and the HEAD sp0c line (3 starts).
  2. DREC1 + tbcheck, mac01t_mr0: G1 = `tbs` bad 0 in every group, hit > 0 (1 start, diagnostic, not a picture run).
  3. DREC1 mac01t_mr0: G2 = sp0c tex (first window after the load) down 40 % or more against the HEAD run of step 1. G3 = top shots 2 of 2 byte-equal.
  4. DREC1 yard and forest_mim: top shot 1 of 1 byte-equal each (2 starts). yard is the MR1 second scene. forest_mim has fog models,
     which use the same G_PM_TEX path.
- Picture: the shot must show Mario and the Toad Town street (mac01t), as in SPLIT0C top shot 0000_1_2.
- Console (rule 6, epic end only): one push with phase. Quantity: opsplit 0c ms in the still windows at the pos line of
  plan_dl_design.md section 1. Pass: 0c down 2.0 ms or more against 20.3 to 21.1 ms.

## 8. Stop rule

- tbs bad > 0: one fix for the named field group, one more check run. Still bad: revert the DREC1 files, stop. A fresh
  investigation agent names the field.
- sp0c tex down less than 40 %: revert. Two unchanged readings of sp0c tex end the edit series (rule 5).
- Any top shot not byte-equal: revert.
- Console 0c down less than 2.0 ms phase on: revert the DREC1 commit, DREC2 does not start, path 2 ends. Path 1 (TEXI1) is next.
- Console 0c down 2.0 ms or more: DREC2 starts (same epic agent if under 150k tokens).

## 9. Risks

| Risk | Where | Handling |
|---|---|---|
| Tint | model.c 1476 to 1496, 1813 to 1881 | Tint words are outside G_PM_TEX. The texture apply is the same for a tinted model. DREC2 excludes tint (header w1 = 0). Shroud with ShroudTintAmt 255 returns early (n = 0). |
| Billboard | model.c 2038 | Per-frame matrix word, never in the record. DREC2 excludes it to keep the mr0 set. |
| Tex panner | model.c 2015 | SETTILESIZE words run after the apply and rewrite tile_sz and td[0] as today. A panner list never enters the snapshot (it is not in the handle list). |
| Custom gfx | model.c 2007, 2054 | A G_DL to a custom list runs after the apply and can change any state, as today. DREC2 excludes it. |
| Fog | model.c 1469, 1810 | Fog combine is in SolidCombineModes (a model word, not the handle list). Fog words are per frame. DREC2 key includes the SETCOMBINE words. |
| dlc refusal | pm_dlc.inc.c 441 to 457 | The record ends at G_PM_RM, before the G_DL. A refused or missed list runs interpreted after the apply, as today. |
| Entry-dependent effects | tex_op 2931, 2992, 3165, ldrec and blk rings | Apply recomputes each decision from the live state (section 5). tbcheck compares by field group. |
| Texture cache eviction | ensure_tile 2623, pm_tex_load | The record does not hold a pm_tex slot. ensure_tile still binds at the first draw. |
| Handle reload at the same address | model.c 2160 | pm_texbind_set bumps tb_gen. |
| Mipmap and aux tiles (2CYC) | tiles 1 to 7 | tmask covers all 8 tiles. Byte-equal on yard and forest_mim. |
| Data size (L1D 16 KB) | tb_snap | about 200 bytes per used handle. 47 texture changes per frame read about 9 KB. |

## 10. Existing group skip (for the user)

dlc_box_out (pm_dlc.inc.c 473) skips a shape group when its box is out of view. It skipped 27773 of 34513 groups (80 %) in the
SPLIT0C window. The memory rule says no culling of any kind, including renderer-side group rejection. This skip is existing port
code. This plan does not change it, and DREC1 does not depend on it. Removing it would raise the dlc share in its own row.

## Decision (2026-10-31)
- DREC1 DROPPED. The texture-bind record cannot reach its gate: the flush stays in the measured period, and the saving estimate (50 to 100 us per frame) is far below the 765 us gate. Not built.
