# PT07: Star Spirit static effect only dims the sprite (root cause found, fix is in main-owned files)

Status: FIXED by VIS-2 (PT07). TN_NOISE on texture unit 2 (64 x 64 L8 random bytes, fixed seed, screen position plus a random offset per frame, uniform nzo in both vertex shaders). The hologram form (NOISE - B) * C + D lowers to D - B * C, then NOISE * C + previous. The combiner audit gives NOISE the mean 0.5. Switch nonoise reads NOISE as 0 (A/B). Shots: 3ds/logs/perf/vis2/noise_ab.png (top nonoise, bottom fixed). Test scene: warp=kmr_00:0 story=-128, shot frame 1225.

Status: root cause proven by code. Fix not applied. Evidence level: proven (code), not yet checked by a shot of the scene.

## Cause
- The Star Spirits in the revival scene (src/world/area_kmr/kmr_00/npc.c:78, :83) and in Star Haven (src/world/area_hos/hos_01/npc.c:389 to :413, StatcAmt up to 200) use SetNpcImgFXParams with IMGFX_HOLOGRAM and IMGFX_HOLOGRAM_NOISE.
- src/imgfx.c:1041 to :1051 sets the combiner PM_CC_IMGFX_HOLOGRAM (include/macros.h:492). The colour equation is (NOISE - PRIM) * PRIM + TEXEL0. PRIM rgb = noiseAmt. The alpha is TEXEL0 alpha * PRIM alpha.
- The port has no NOISE input. 3ds/src/pm_comb.c:39 to :64, function term_rgb: slot 0 (A) with sel 7 (G_CCMUX_NOISE) has no case and falls to line 64 `return TM(TN_ZERO, 0)`. The audit in 3ds/src/pm_gbi.c:780 and :339 to :342 also reads NOISE as 0, so no "comb new" line reports it.
- Result: colour = TEXEL0 - noiseAmt * noiseAmt. With StatcAmt 200 (0.78) the sprite loses 0.61 of its brightness and has no random part. The user sees a dimmed sprite and no static.
- The decomp-side call is correct. No switch of DR1, TX1, PG, KICK or DECAL2 takes part (the path is pm_comb.c only).

## Fix text (main-owned files: pm_comb.c, pm_gpu.c, shaders/pm.v.pica)
1. Add a term TN_NOISE. In pm_comb.c term_rgb: `if (slot == 0 && sel == 7) return TM(TN_NOISE, 0);` before line 64. Same in cc_v (pm_gbi.c:780 area): slot 0 sel 7 returns a fixed 0.5 for the audit.
2. Map TN_NOISE to the PICA source GPU_TEXTURE2 (name PM_NM_NOISE in src_of, pm_comb.c:229 to :265).
3. Bind a 64 x 64 GPU_L8 white-noise texture (random bytes, made once at boot, repeat wrap, nearest filter) to unit 2 only when the plan uses NOISE.
4. In 3ds/shaders/pm.v.pica add `outtc2.xy = screen position * (1/64) + offset`. The offset is a uniform that changes every drawn frame (random pair). The RDP noise is per pixel and changes every frame.
5. Cheap stop-gap if step 3 and 4 are too large: NOISE = 0.5 constant (the mean of the RDP noise). The sprite then brightens by 0.5 * amt - amt * amt and shows no grain. Do not use it as the final fix.

## Test
- Switches: `fast fixedseed autoa warp=kmr_00:0 story=-128 loglevel=info shots shotwin=<frame>:3`. The first Star Spirit speech starts after about 1400 scene frames. Find the frame with `fxlist` or a shot every 30 frames.
- Pass: the sprite shows grain and keeps its colour. Check also PT08 (static text) in the same run.
- Same fix also covers PT08 (MSG_FX_STATIC, MSG_FX_NOISE_OUTLINE).
