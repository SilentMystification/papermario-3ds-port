# plan_c1_texbind: native bind for model textures (row C1)
(Opus plan, saved by the orchestrator. Read at HEAD 47e9d88ea, which has G1 and H1. pm_gbi.c is cited by symbol.)

## 0. Why the earlier attempt stalled, and what changes
- A texture DL does not have one fixed result. Five of the fields it writes depend on what came before it (marked [H] below). "Snapshot the exit state, replay it as a constant" is wrong. It is also the reason the old pdl_try path never got a reuse hit (heap gen and state_in both change).
- The bind has to run the same handler code on the same words, in the same order. It skips only the dispatch prologue and the no-op syncs. Draw closes then stay identical by construction: the same pm_gpu_flush, pm_tex_set_wrap, pm_tex_set_filter and sync_combine calls happen in the same order.

## 1. Interpreter state the texture DL writes (make_texture_gfx at model.c:3226, handlers in pm_gbi.c)
Every op (prologue, per non-geometry op): gv_epoch++, pdl_pso_cur=pdl_ep_pso=0, pdl_tex_slot=-1, pm_gbi_cur_op. Stats only: tex_streak, n_ops, dls_ops, os_*, gp_*. Syncs are skipped before the prologue since G1 (g1_skip).
- TLUT load (CI4 gDPLoadTLUT_pal16(0) / CI8 _pal256, 6 ops): SETTIMG (timg=pal, timg_fmt=0, timg_siz=2, timg_stride=1, ld_valid=0, the pm_is_cfb test); SETTILE 7 (tile_fmt/siz/tmem(=256)/pal/wrap[4]/shift[2]); LOADTLUT (tlut=pal, tlut_n=16 or 256, tlut_pal[0]=pal); [H] the V20 loop: for n in {0,1}, if td[n] is a valid CI tile on bank 0 with a different palette: pm_gpu_flush, set td[n].pal and pal_n, cur_tile=-1 if n is cur_tile. AUX_INDEPENDENT adds a second TLUT into slot 1 (pal16) or slot 0 again (pal256).
- SETCOMBINE (SolidCombineModes): comb_w0/w1, comb_a..d, comb_aa..ad, comb2_* (8 fields), comb_set=1, use_t1 (TEXEL1 rename), box_folded=0, box_prod, comb_alpha_shade, then sync_combine().
- TEXTURE: tex_on=1, tex_ss=tex_ts=0xFFFF, then sync_combine(). Mipmap level ignored.
- SETOTHERMODE_H (sft 4, len 16): omode_h bits 4..19 = filt<<12 | G_TC_FILT | lut | lod | G_TP_PERSP. If TEXTFILT changes: pm_gpu_flush and pm_tex_set_filter. CYCLETYPE is outside the field: no sync_combine.
- EXTRA_TILE_NONE (RGBA16/32, IA8/16, I4/8, CI4/8; 7 ops): SETTIMG (timg=raster, timg_fmt=fmt, timg_siz=load siz, stride 1, ld_valid=0); SETTILE 7 (fmt, load siz, tmem 0, pal 0, wrap/mask/shift of tile 0); LOADBLOCK ([H] ldrec[k].img=NULL where tmem < words; [H] blk slot (matches tmem 0, else blk_n++&3) = {0, raster, bytes}); SETTILE 0 (tile_*[0]; [H] same_tile check; if different: pm_gpu_flush, pm_tex_set_wrap, st0_ep/st0_ok); SETTILESIZE 0 (tile_sz[0], td[0].valid=0, cur_tile=-1, dual_on (+sync_combine if set), m1.valid=0, tile_x0/y0/fx/fy; [H] tsz_last: glyph_same path, PG1 epoch reuse, or a new record; td[0] = {raster, fmt, siz, w, h, stride=w, 0, 0, tlut_pal[0] or tlut, tlut_n}, then gp3_t0_set).
- EXTRA_TILE_MIPMAPS (7 ops per LOD, LoadMultiTile): SETTIMG with the real stride; LOADTILE (ld_valid=1, ld_x/y/w/h, [H] ldrec slot); SETTILE L and SETTILESIZE L. L=0 binds as above; L=1 writes m1 (CI4), td[1] (ldrec lookup), cur_tile reset, sync_combine; L>=2 writes tile_sz only.
- AUX_SAME_AS_MAIN (ScrollTextureBlockHalfHeight): the NONE sequence with maskt-1 and h/2, then SETTILE 1 (tmem = half) and SETTILESIZE 1. td[1] placed from [H] blk through the V13/PT82 boff logic.
- AUX_INDEPENDENT (handle list, used when no special prop): LoadTextureTile on tile 0, then ScrollMultiTile to tile 1 (pal = aux index). Writes ld_*, ldrec, td[1], m1.
- Never written: geom, omode_l, prim/env/fog/blend, matrices, vertices, scissor, cimg.

## 2. The record and when it is invalidated
C1a (build this): the record is the validated list pointer: `static const Gfx* tb_list[128]` in pm_gbi.c (4 bytes per handle, 512 bytes total). A refused list stores NULL. Built by `pm_texbind_set(int idx, const Gfx* l)` at load time: scans to ENDDL (at most 64 ops) and accepts only SETTIMG, SETTILE, LOADBLOCK, LOADTILE, LOADTLUT, SETTILESIZE, TEXTURE, SETCOMBINE, SETOTHERMODE_H and the g1_skip ops (make_texture_gfx emits only those; the check is the guard). It does not run the interpreter at load (handlers have GPU side effects); the bind runs the same handlers on the same words.
- Invalidation: rebuilt on every load_texture_impl. The handler requires `tb_list[idx] == w1`; a reload at another address, the pm_tex_unloaded path, or the `gfx = nullptr` clear at model.c:2441 gives a mismatch and the handler runs the original list as G_DL. Content changes need nothing here: td[0] still goes through ensure_tile and pm_tex_load (cache lookup, content hash, invalidate as today). No cache slot is stored (pdl_lazy_tile cannot be reused: it returns 0 whenever td[0].valid, and the bind sets td[0]).
- C1b (only if gated in section 5): `TexBind` record, 32 bytes aligned(32), one cache line: {const Gfx* list; const void* img; const void* pal; u32 cc0, cc1; u32 st0, st1; u32 sz /* 0-23 SETTILESIZE w1, 24-25 filt, 26 ok */}. NONE kind only. Constant fields stored directly; [H] parts call the existing code, factored out. The builder rebuilds the list words from the record with the gbi.h macros and memcmps them against the handle list (not equal means ok=0). The `tbcheck` switch compares gbi_state_get after record apply against after tb_run, on first use per map.

## 3. The port op
- Opcode `G_PM_TEX 0x0A`, defined in include/gbi_custom.h next to G_PM_RM. Taken by F3DEX2: 0x00-0x08, 0xC0, 0xC8-0xCF, 0xD3-0xDF, 0xE0-0xFF. Free: 0x0A-0xBF, 0xC1-0xC7, 0xD0-0xD2. The port already uses 0x09 (G_PM_RM).
- Words: `gSPPmTexBind(pkt, idx, dl)`: w0 = 0x0A<<24 | (idx & 0x7F), w1 = dl.
- Handler:
```
case G_PM_TEX:
    if (!w1 || tb_list[w0 & 0x7f] != (const Gfx*)w1) { RS_ADD(RS_TBFB,1); w0 = (u32)G_DL << 24; goto gdl; }
    RS_ADD(RS_TBN,1); tb_run((const Gfx*)w1); break;
static __attribute__((noinline)) void tb_run(const Gfx* g) {
    for (;; g++) { unsigned op = g->words.w0 >> 24;
        if (op == G_ENDDL) return; if (g1_skip[op]) continue;
        pm_gbi_cur_op = op;            /* flush-cause class stays TEX/CC */
        tex_op(g->words.w0, g->words.w1); } }
```
- Also: the prologue runs once for the op itself (gv_epoch++ and the pdl resets) because 0x0A is not in the geometry exclusion list. Add G_PM_TEX to tex_streak_op and to os_class_of (OS_TEX) so streak and opsplit attribution do not move. sync_combine, ensure_tile (deferred through td[0] and cur_tile=-1), TMEM records (ldrec/blk), and flushes on a texture change all happen inside tex_op exactly as before. Do NOT add G_PM_TEX to pdl_op_rw or pdl_m_state_only: it must stay a PDLM_RUN in main-list templates (pdl_tpl_skel hashes w1, so each texture gets its own template).

## 4. src/model.c edits (TARGET_3DS only; no handle field)
TextureHandle stays 0x44 and matches the decomp. Index = `textureHandle - TextureHandles`.
- After line 180 (the MODEL_RM_OP block): `#ifdef TARGET_3DS` `#define MODEL_TEX_OP(pos,h) gSPPmTexBind(pos, (h) - TextureHandles, (h)->gfx)` `#else` `#define MODEL_TEX_OP(pos,h) gSPDisplayList(pos, (h)->gfx)` `#endif`.
- Lines 1530 and 1534 (earlier cited as 1523/1527, +7 after C2): `MODEL_TEX_OP((*gfxPos)++, textureHandle);`
- Line 2117, after `gSPEndDisplayList((*temp)++);`: `#ifdef TARGET_3DS` `pm_texbind_set(handle - TextureHandles, handle->gfx);` `#endif` (extern declaration in the block).

## 5. Rows (one Azahar quantity each; picture checks: yard, tour10, pause_menu, file_select)
| row | lines | content | quantity / gate |
|---|---|---|---|
| C1-0 tex_op | about 15 net (about 300 moved) | Cut the bodies of SETTIMG, SETTILE, LOADBLOCK, LOADTILE, LOADTLUT, SETTILESIZE, TEXTURE, SETCOMBINE, SETOTHERMODE_H/L verbatim into `static noinline void tex_op(u32 w0, u32 w1){switch(w0>>24){...}}`. The cases in gbi_run_body call it. | ops/frame and picture unchanged; opsplit tex+cc us within +-3% |
| C1-1 op + record + swap | about 55 | gbi_custom.h (8), pm_gbi.c tb_list/pm_texbind_set/tb_run/case/streak/os (35), model.c (10), RS_TBN/RS_TBFB + `tbsplit f n fb` line (3) | dlsrc class 2 ops/frame down >= 650 (class 2 + SKN together down >= 1000; G1 already removed the syncs). tbsplit fb = 0 in the yard; main walk us down. The `pdl` comp count also drops. Console: walk down >= 2 ms (under 1.5 ms means revert). |
| C1-2 TexBind record | about 60 | section 2 C1b. Only if after C1-1 opsplit tex us per bind is still > 40 us (Azahar) | tex us/frame down >= 30% with tbcheck at 0 mismatches |
| C1-3 other emit sites | about 20 | No other TextureHandle emitter exists (grep: only model.c 1530/1534). model.c:1523 path builds the texture inline every frame and stays. animator.c 794-837 and entity_model.c 285+ emit render-mode G_DLs: swap them to gSPPmRenderMode (C2 pattern). Their texture loads are inline (C1i/HUD work in plan_source_opts). | rmsplit n up, fb 0; prod ops for entity/animator down |

Hardware: C1a adds no record line; tb_list is 512 bytes, one 4-byte read per bind. The list in TextureHeap is read once (17 words, 5 lines, the same lines G_DL reads today). About 17 fewer prologues and dispatches per model, 68 models per frame. tex_op leaves the hot loop smaller (I-cache, rules 1 and 10). C1b record: 32 bytes = 1 line plus a 136-byte stack expansion; no division.

Risks: palette change after load is safe (pointers unchanged; pm_tex hashes content on the load; a TLUT bank shared with a bound td[1] still goes through the V20 loop). Mipmaps and aux run through tex_op the same way (C1b falls back to G_DL). Texture animation: textureVariation (map_api.c:180) selects another handle, each with its own record; pans are separate SETTILESIZE ops after the bind (model.c:1979-1994). Pause partner portraits and file select do not use TextureHandles (indirect risk only: check in pictures). Main-list templates: a new op shifts segment boundaries; watch `pdl f tpl fb`; fallback is one line: add G_PM_TEX to pdl_tpl_seg_refuse. Battle: shared TextureHandles get reloaded and every reload rebuilds the record.

Decisions: the record is a validated pointer, not a 32-byte replayed state (a replayed result is wrong for the [H] fields); no TextureHandle field is added; the gate is restated for after G1: class 2 ops down >= 650.
