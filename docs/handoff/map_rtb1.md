# map_rtb1: per-batch cost cut in flush_draw (row RTB1, route B)

Map only. No source edits, no runs. Branch native, HEAD ad522d1ed. Source of the plan: docs/handoff/nds1_status_2026-11-05.md, route B.
File line numbers refer to 3ds/src/pm_gpu.c and 3ds/src/pm_tex.c at this HEAD.

## 0. Finding that changes route B (read first)

The route B premise is the texture share of 38 percent and the close share of 23 percent. These numbers come from docs/handoff/design_native_draw.md section 2.1, cited as "[D plan Evidence]". The log that is in the tree does not show them.

The only per-stage close timing in the tree is docs/handoff/diag_dc_console.md (console, O3DS build 1227, yard, 12 steady frames, 67 non-empty closes per frame). Per non-empty close:

| stage (flush_draw marks) | console us per close | console share | Azahar us per close (diag_dc.md, DC1) | Azahar share |
|---|---|---|---|---|
| pal (uniforms, palette, projection) | 11.8 | 18 % | 3.1 | 12 % |
| bind (texture binds, aux, noise) | 8.8 | 13 % | 3.2 | 12 % |
| tev (TevKey compare, rebuild on change, cen_add) | 6.8 | 10 % | 2.1 | 8 % |
| st (depth, alpha, scissor, cull, blend, user clip) | 10.0 | 15 % | 2.0 | 8 % |
| draw (C3D_DrawArrays or DrawElements) | 28.8 | 43 % | 15.4 | 60 % |
| kick (kick_check) | about 1.4 | about 2 % | 2.1 | 8 % |

- Console close total: 97.4 us per close, 67 closes per frame, 6.5 ms per frame (9.4 % of the 69.3 ms frame). The five stages sum to 66.3 us. 31 us per close is in no field. The code agent must not treat that 31 us as known.
- Console bind is 13 % of a close, not 38 %. The 38 % texture share and the 23 % close share are not reproduced by any log in the tree.
- Console bind + tev + st + pal together: 2.5 ms per frame (0.59 + 0.46 + 0.67 + 0.79). This is the total that route B can touch. The 9.7 ms gap to the 30 fps target (docs/handoff/plan_dl_design.md section 2) is not in this set.
- The console build is 1227. The current HEAD differs. Phase on inflates these numbers (plan_dl_design.md section 2: phase off is 0.74 to 0.79 of phase on).
- Route B cannot remove the draw stage (43 % on console). It cannot remove the texture change cost per model either (section 1, step 7).

Estimated ceiling for RTB1 (assumption, not measured): removing all tev and all st work gives 1.1 ms per frame on console. A realistic cut, which keeps the citro3d setter calls on each real change, is 0.1 to 0.4 ms per frame. The compare work itself is about 100 to 300 cycles per batch, which is 0.1 to 0.3 ms per frame at 67 closes. Azahar cannot price this (no cache model).

Recommended before any code: a fresh investigation agent (CLAUDE.md rule 3) explains the 10 us per close of the st stage. The compare work alone is far below 10 us (about 2,700 cycles). The cost is in the setter calls on change or in cache misses. Until that is known, the RTB1 gain is an estimate only.

## 1. Current per-batch call sequence in flush_draw (pm_gpu.c:930 to 1161)

Calls run in this order for each batch that reaches the GPU. Marks are the DC_T() timers (pm_gpu.c:961, 1041, 1061, 1080, 1129, 1143, 898).

| # | lines | step | per-batch work | console us per close |
|---|---|---|---|---|
| 1 | 930-951 | entry checks: empty batch, r_nocol, H03 failed texture | early return paths; flush_fl_dbg only when fl_on() | in the 31 us gap |
| 2 | 953-961 | FX2a off-target snapshot (off_active only); DC_T(0) | none on the main target | pal starts |
| 3 | 966-971 | gpal, rec_cap block (UC1, not in a normal run) | flag tests | pal |
| 4 | 973-1000 | graw: program bind (bound check), five-register uniform set with memcmp: g_uvso, g_cmul, g_cadd, g_zmix, g_fx (pm5 path); pcomp matrix upload loop per palette | 4 to 5 memcmp; C3D_FVUnifSet on change | pal, the largest stage on console (11.8) |
| 5 | 1000-1024 | graw pm4 path: same as 4 with sh_* arrays; palette register check per pal_n | memcmp on sh_pal_mtx when gen differs | pal |
| 6 | 1027-1034 | projection (u_proj, not gpal): memcmp of proj_last (64 bytes) | one memcmp per batch | pal |
| 7 | 1041 | DC_T(1); pm_tex_bind() (pm_tex.c:1647) | tb_last and tb_f, tb_ws, tb_wt compare; on change: texword_dump, tex_for_draw, C3D_TexSetFilter, C3D_TexSetWrap, C3D_TexBind | bind (8.8) |
| 8 | 1043-1044 | pm_tex_bind_aux() (pm_tex.c:1741) when combine 8 and uses_tex1 | au_last compare; on change: C3D_TexSetFilter, C3D_TexSetWrap, C3D_TexBind | in bind |
| 9 | 1046-1058 | rec_cap check, i8 flags, snap rebind (off_active), noise bind (unit 2, once per sh_clear) | flag tests; C3D_TexBind only when needed | in bind |
| 10 | 1061 | DC_T(2); PHF marks | none | bind ends |
| 11 | 1063-1064 | z flag, nozt_on() | flag tests | tev starts |
| 12 | 1066-1076 | TevKey build: memset of the 19-field key, 19 field stores, plan_gen | about 80 bytes of stores | tev |
| 13 | 1077 | tev_same = memcmp(tk, tev_key); on change flush_tev_rebuild (line 647, cold path per change) | one memcmp per batch | tev (6.8) |
| 14 | 1079 | cen_add(&tk, ...) (pm_gpu.c:495): FNV hash over 18 words of the key, then the census table probe; unconditional, no switch test (comment at line 490) | 18 mul-xor rounds plus a probe per batch | tev |
| 15 | 1080 | DC_T(3) | none | tev ends |
| 16 | 1085-1095 | depth test: sh_depth_test (shadow SH_DT), or the B3b and decal paths with stencil | shadow compare per call | st starts |
| 17 | 1098-1099 | sh_depth_map (SH_DM), sh_early (SH_ED) | shadow compare per call | st |
| 18 | 1100-1101 | sh_alpha_test (SH_AT) | shadow compare | st |
| 19 | 1103-1120 | scissor math: sci_x0 * 5 / 4 and sci_x1 * 5 / 4 (ui and 3D cases, lines 1110-1111), clamps, then sh_scissor (SH_SC, 5 words) | per batch: about 15 branches, 2 signed divides by 4, shadow compare | st |
| 20 | 1121 | sh_cull (SH_CU) | shadow compare | st |
| 21 | 1122-1128 | blend_off (off_active) or sh_blend (SH_BL, 6 words) | shadow compare | st (10.0 total) |
| 22 | 1129 | DC_T(4) | none | st ends |
| 23 | 1130 | sh_user_clip(graw) (SH_UC) | shadow compare; GPUCMD writes on change | st |
| 24 | 1132-1160 (graw) | C3D_GetBufInfo on buf_cur change; C3D_DrawElements; bookkeeping; dc_kick | draw | draw (28.8) |
| 25 | 1161 | flush_draw_cpu(z) (pm_gpu.c:871): C3D_BindProgram on change; C3D_GetBufInfo on pool change; decal2 passes; C3D_DrawArrays; B3b pass 2; B2 grab bookkeeping; dc_kick (898, 966) | draw | draw |

Notes on the current code:
- The state shadows (sh struct, pm_gpu.c:524 to 531; setters at 558 to 597) already skip a repeated value. A repeated value costs one compare. A changed value calls the C3D setter.
- tev_key (pm_gpu.c:488) already caches the TEV build per key. The key check is a memset and a memcmp of about 80 bytes.
- The bind shadow (tb_last, pm_tex.c:1662) already skips repeated binds of the same slot, filter and wrap.
- So route B does not add a cache. It moves one more layer of compares into one block compare. The saving is the compare overhead, not the setter calls.

## 2. Numbered edit list for the code agent

Each item is 30 changed lines or less. Items 1 to 6 form RTB1. Item 7 is a separate row. Do not start item 2 before the census gate of item 1 shows the same multiset.

1. pm_gpu.c:524-531 (the sh struct area). Add a struct FlKey of 20 words: the 19 TevKey fields in TevKey order plus z, graw, gpal, g_use5, g_cullm, r_zcmp, r_zupd, r_zmark, r_decal, r_nocol, r_atest, r_blend, stencil_on, sci_x0, sci_x1, sci_y0, sci_y1, pm_wide, ui_ortho, off_w, off_h, memb. Add a struct FlBlk: the resolved arguments of sh_depth_test, sh_depth_map, sh_alpha_test, sh_scissor, sh_blend, sh_cull, sh_early, sh_user_clip, plus cen_h (census hash of the key) and valid. Old behavior: no block. New behavior: one static FlKey fl_key and one static FlBlk fl_blk, both zeroed. Old path stays until item 3.
2. pm_gpu.c:1066-1077 (TevKey build and tev_same). Old behavior: memset, 19 stores, memcmp, optional flush_tev_rebuild. New behavior: fill FlKey (one store per field, no memset), compare with memcmp of sizeof FlKey. On equal key: tev result and memb come from fl_blk. On different key: keep the flush_tev_rebuild call unchanged, then set fl_key. Nothing else changes in this block.
3. pm_gpu.c:1080-1128 (depth, alpha, scissor, cull, blend, the memb and off_active choices). Old behavior: each call to the sh_* setter runs its shadow compare on every batch. New behavior: if fl_key equals the stored key and fl_blk.valid is 1, skip this whole block. Otherwise run the same code to compute the values into fl_blk, call the same sh_* setters with those values, and set fl_blk.valid = 1 and store fl_key. The setters themselves are not changed. Keep the old if/else order.
4. pm_gpu.c:1077-1079 (cen_add call) and pm_gpu.c:495-500 (cen_add body). Old behavior: cen_add hashes the 18 key words on every batch. New behavior: cen_add takes the hash as an argument. The hash is computed in item 3 once per block build and kept in fl_blk.cen_h. On a key hit the hash is reused. The census keys stay identical, because the same fields in the same order are hashed. The plan_gen field stays out of the hash, as now.
5. pm_gpu.c:557 (sh_clear). Old behavior: sh_clear resets sh.has, tev_valid, pool_buf_ok, proj_ok, noise_bound and the tex shadow. New behavior: also set fl_blk.valid = 0 and fl_key.combine = -1. This covers every invalidation site: frame begin (pm_gpu.c:1994), target changes (1754, 1767, 1780, 2015), kick split (1883), and the other sh_clear calls. Add no other call site.
6. pm_gpu.c:1063-1064 and 1083-1128 (the z and nozt values). Old behavior: z is computed per batch. New behavior: unchanged computation. Only the block skip uses z through fl_key.z. Check that nozt_on() cannot change between batches (it reads a switch once, so it is fixed). No code change in this item beyond the key store. Also delete the old per-call dead code that item 3 replaces.

7. (separate row, not RTB1) pm_gpu.c:979-1024 (the five memcmp uniform blocks on the raw path). Old behavior: five memcmp per raw batch on g_uvso, g_cmul, g_cadd, g_zmix and g_fx. New behavior would need a generation counter bumped at each writer of those globals. The writers are spread over other files (pm_gbi.c and the model code). This is larger than 30 lines when done right. It is not part of RTB1.

Not in RTB1: the texture bind chain (pm_tex.c:1647 and 1741). Texture changes per model (47 of 71 opaque plain draws, design_native_draw.md section 1.3). A block keyed by the template cannot hold the texture words. A per-texture word cache would be a new row with its own census and picture gate.

Not in RTB1: draw (C3D_DrawArrays), 43 percent of a close on console. The draw call cannot be cut by a state block.

## 3. State key (when a block is rebuilt)

A block is rebuilt when fl_key differs from the key of the batch, or when fl_blk.valid is 0. The key is the set of values read by flush_draw from this list:

- TevKey fields (pm_gpu.c:487): combine, tex, tex1, i8_0, i8_1, rgb_scale, r_mix, draw_3d, rect_2d, dview, off_active, off_i8, r_memblend, r_replace, r_blend, prim, cconst, mixcol, plan_gen (plan_gen is 0 unless combine is 8).
- Fragment state: z (from z_on, rect_2d, rect_z, off_active and nozt), graw, gpal, g_use5, g_cullm (raw batches only), r_zcmp, r_zupd, r_zmark, r_decal, r_nocol, r_atest, stencil_on, sci_x0, sci_x1, sci_y0, sci_y1, pm_wide, ui_ortho, off_w, off_h, memb (from the TEV build).
- Not in the key, by design: decal_bias() (constant after its first call, pm_gpu.c:478), the texture slot words (they belong to the bind chain), and the vertex data.

A key change does not change the picture when the block is rebuilt from the same values. The block is only skipped when every key value matches.

Two keys that differ only in a field that item 3 does not read are a miss, not an error. Missing a field in the key is the error that matters. Add every global that the block code reads.

## 4. Gates

Run each gate with the catalog command (CLAUDE.md rule 5). Set PM_ROW=RTB1 in every shell. Write one justification line to 3ds/logs/perf/status_RTB1.txt before each Azahar start.

The census scenarios census_mac01 and census_toad_shot are not in the native catalog. They exist only in the nds1 branch (3ds/tools/azt_scenarios.json, nds1 lines 4287 and 4311). Before the census gate runs, the code agent copies these two entries and their catalog rows (docs/handoff/test_catalog.md) into native in the same commit (CLAUDE.md rule 5). The native tree has census_yard (catalog line 40).

(a) Census multiset, log only, equal before and after:
- `PM_ROW=RTB1 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 census_mac01 -Row RTB1`
- `PM_ROW=RTB1 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 census_toad_shot -Row RTB1`
- Multiset to compare: `cen f200 summary batches <n> tris <t> distinct <d> over <o>` and each `cen f200 k <key> tex <id0> <id1> tris <t> n <count>` line. Batches, tris and tex plus tri must match the HEAD run. Run the HEAD baseline first, before item 1 is applied (no native baseline for these scenes exists in the tree).
- census_toad_shot has no drawcensus switch, so it gives no census lines. It is a picture gate only (section 4c).
- Pass rule: the census multiset is equal (not only the counts). A changed key hash fails the gate (item 4).

(b) Host tests (no Azahar start):
- 3ds/tools/ndt_host_test.sh
- 3ds/tools/tex_conv_test.sh
- 3ds/tools/tx1_test.sh
- The nat_host_test.sh of nds1 is not in native and is not used.

(c) Picture gate on the scenes that use the changed path (pm_gpu.c is shared draw code, CLAUDE.md rule 5):
- census_toad_shot (shots at frames 200 and 204, region_diff.py against the HEAD reference shots). Pass: region_diff reports no difference. A person reads the shot, because the catalog marks this scenario as needs_picture.
- census_yard (log only, census multiset, catalog line 40). Same multiset rule as (a).
- No full sweep. No catalog run per row (CLAUDE.md rule 5).

(d) Performance (console only, not Azahar):
- Azahar cannot price a cache effect. An Azahar number is direction only.
- The row is pushed only at the end of its epic. The console quantity is the fsplit gbi and dl values, and the dcsum stage split (bind, tev, st, pal, draw, kick), with phase on and the same route as diag_dc_console.md.
- Pass rule for the console: the tev plus st stage time falls by 0.3 ms per frame or more (about 40 percent of 1.1 ms). Below that, the row is not worth the risk and the user decides.

## 5. Risks

1. Small ceiling. Bind, tev, st and pal together are 2.5 ms per frame on console (build 1227). The realistic cut is 0.1 to 0.4 ms. This does not reach the 9.7 ms gap by itself (section 0).
2. Premise. The 38 and 23 percent figures are not reproduced in the tree. The console split is bind 13 percent, draw 43 percent. Decide with the user before code (section 0).
3. Texture changes per model. 47 of 71 opaque draws change texture. A block keyed on the template misses on most batches for the bind part. Only item 2 and item 3 (TEV and fragment state) are in the block.
4. Direct register writes. If the block writes GPU registers itself (not in the plan above), citro3d dirty tracking does not see them. The plan above uses only the existing C3D_* setters, so this risk does not apply to items 1 to 6. A later row that writes registers must check the citro3d version in the build first.
5. Missed invalidation. A call site that changes a sh_* state without sh_clear leaves a stale block. Item 5 covers the known sh_clear sites. Any new writer of sh.* or the key globals needs the same check.
6. Census key drift. If item 4 changes the hash input or order, every census key changes and gate (a) fails. Keep the field order and the plan_gen exclusion.
7. Off-target paths. off_active changes the scissor, blend and depth-map choices (blend_off, scissor_off, the FX2a memb path). The key includes off_active, off_i8, off_w and off_h. A miss on any of them shows as a wrong blend in a shot.
8. Stencil paths. The B3b and decal paths (pm_gpu.c:885 to 907, 1085 to 1090) change depth and stencil inside flush_draw_cpu and across calls. The block must not cache a depth state while stencil_on changes. The key includes stencil_on and r_zmark.
9. Hot code. flush_draw is large. Per hw_guidelines.md rule 1, the block build (item 3, scissor and blend) goes into a cold or noinline function. The skip path stays in flush_draw and must be short.
10. Divide in the scissor math. sci_x0 * 5 / 4 is a divide on every batch in the current code (pm_gpu.c:1110 to 1111). Item 3 moves it to the block build, which runs on a key change only. This is a side benefit of RTB1, not its main gain.
11. Unexplained close time. 31 us per close (console) is in no measured field. If the 31 us is in the entry or the uniform block (items 1 to 6 of section 1), a block over tev and st does not reach it. The code agent should not claim the gain until the stage split covers the whole close.
12. Azahar does not model caches. A block that saves L1 misses shows no Azahar change. Only the console shows it.
13. Shared draw code. pm_gpu.c is shared draw code. A byte-equal or picture check on the scenes that use the path is required (section 4c).
14. Build. Run python 3ds/tools/gen_map_restore.py before the build, then restore 3ds/src/map_restore_table.c (CLAUDE.md rule 9). Do not commit that file.
