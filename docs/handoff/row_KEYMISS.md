# Row KEYMISS: per-frame values in the native cache keys (audit, one Opus agent)

## Goal
Find every value the game changes each frame that enters a native cache key (texture cache pm_tex.c, model record cache nd_model.c /
pm_gpu.c), rank them by a census, and fix them at the shared key code (patch on hit, scroll as offset, GPU mix), not per effect.

## Start state
Worktree build3ds/wt/KEYMISS, branch KEYMISS from main AFTER the BTLDIM merge (it brings texmisslog/tm_caller, the non-CI tlut fix,
PM_RM_SHROUD, btldim scenarios, pad, invitem, btlmenu). Do not touch background.c or the BTLDIM pm_tex lines. One Opus agent, no subagents.

## Code facts (oracle)
- ndmc_cmp (nd_model.c:430) compares words 0-32, 38, 53, 54. Per-frame members are record words: w8 R4w0 fog colour, w11 R5w1 fog
  pos, w12/w13 R6 prim/env, w14 R7w0 blend, w18-21 R9/R10 pan tile size, w22/23 R11 pre/post list pointers.
- MERLEE mask (pm_gpu.c:2818) only when rec[6]&32 (set only model.c:1970). FOG_SHROUD (model.c:2065) sets bit 2: w8 RGB and w11 unmasked.
- A key hit can still be refused: model_vis_same (vmask) and mver (ndn_rng). Viewport is not compared: 2 cameras (tattle) may rebuild.
- Shaded sprites: CPU RGBA8 composition (nd_tex_set.c:651-680 -> pm_tex_load_shaded), dkey = FNV(lut, dx4, dy4): each change = miss.
- Tattle palette lerp runs only with world fog (stages kmr_06, mim_01). pm_tex_load_shaded (1742) and pm_tex_load_rgba8 (1753) also call
  load_impl: if a non-CI fmt reaches them with a tlut, move BTLDIM's rule into load_inner.

## Members and checks (run all first; drop a false one with a note)
1 battle sky (BTLDIM, check btldim texmiss about 0). 2 tattle: (a) palette lerp (likely false), (b) vmask ping-pong: rng about 2 per
frame per model while the bubble is open. 3 FOG_SHROUD: ndmrw `8:` `11:` in a fog-stage fade; kzn_08 func_80240718 set_world_fog_dist
each frame -> `11:`. 4 hos_01 stars (7 pre owners) and panners (`20:`). 5 shaded sprites (hos_01, kgr_01: shaded owner cause D each
frame). 6 HUD/item/block palettes (likely false, bounded sets). 7 effects/msg rainbow (likely false; print cell_new_n and sprite memo
hits/misses). 8 non-CI tlut (fixed by BTLDIM; census cause T = 0).

## Census tool (switch keymiss, debug tool, stays)
New 3ds/src/pm_keymiss.c: cold, noinline; tables on the heap at first use (tex 32x64 B, mdl 32 entries with last key, about 14 KB);
power-of-2 hash, masks. Hooks only on miss paths: pm_tex.c load_miss after tc_miss++ (owner img+tlut; cause P phash, B bytes, D dkey,
K w/h/stride/fmt, T tlut, E evicted), nd_recs.c:123 key miss (first differing word j + xor), pre-list branch (pre), pm_gpu_model_hit
returns at ndn_rng/ndn_texf. Owner = w24 and w0. Print at the ndn_log window and at pm_km_flush(tag) from subject lines (post through
pm_wq_post; tables are walker side). Top 8 per class, flag EVERY when frames >= window-1. Lines:
`keymiss tex <tag> f%u win %u: img %p tlut %p %ux%u fmt %u siz %u miss %u fr %u cause %s kb %u ra %p`
`keymiss mdl <tag> f%u win %u: geo %08x r0 %08x miss %u rng %u texf %u pre %u fr %u w%u xor %08x`
3ds/tools/keymiss.py: symbols from the lane .map, word names, ranked table to a file. Commit the tool alone; spot_mac01_det byte-equal.

## Scenarios (cap <= 25 s, until = subject line; scenarios and benches in one commit)
keymiss_hos01 (starfx_hos01 + keymiss), keymiss_kzn08 (walk, until `keymiss mdl win f600`, cap 20), keymiss_btl_tattle (kmr_03 battle,
Tattle by padscript, 3DS-only `tattle: open fN`/`close` in 16C8E0.c), keymiss_btl_fogstar (btldim_star on mim_01 or kmr_06 stage),
keymiss_merlee (until `merlee: darken done`), keymiss_kgr01 (walk, cap 20). Optional yard_hud. Write the ranked table into this file.

## Fix rules, in rank order (default M1+M4, M2, M3, T2; then M5/T1/T3 only if ranked EVERY)
M1 colour words out of the model key, patched on hit (pm_gpu_model_key/hit, nd_tmpl.c nd_tev_build, nd.h NdTevOut/NdNatC): mask w12/w13
(rec[6] bits 4/8), w14, w8 RGB; per-stage source code src[6] in NdNatC rebuilds tev_col on hit; value predicates (alpha 0/0xff) stay as a
class word; replaces the MERLEE mask and mix_st (delete old path). MEMORY: NdNatC aligned(32), pool 512: src[6] must fit existing padding
(_Static_assert). M4 fog position out of key: mask w11, recompute zmix from rec[11] via a one-entry memo (no division steady).
M2 scroll as texture offset: key keeps tile id and extents of w18-21, drops uls/ult; hit sets uvso/uvx like gv_params_calc; wrap by
masks; clamp/mirror tiles keep full words. M3 pre-list by content: lists nd_r11_state fully decodes as state-only hash with colour masked;
replaces the STARFX no-store rule for them. T2 in-place rewrite of the same texture identity in load_miss (slot unused 2+ frames,
no vram_up pending). M5 view id in the compared words (tattle). T1 BTLDIM GPU mix pattern in 16C8E0.c:750-765. T3 shaded sprite on the
GPU (ramp TLUT as I8 on unit 1, TEV lerp): biggest; at the 2-try limit hand back as its own row with census numbers.

## Done criteria (per fix stage)
nd_host PASS. ndmcheck on hos_01 and kzn_08 with the hit patch applied in ndc_compare: ndmcache diff 0. Byte gates only for changed paths:
model hit path: spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det 0000-0015 vs build-at base; pm_tex T2:
battle_start_f182_cold, spot_mac01_det. One picture per fixed member. keymiss counts base vs head (bench.py compare), fixed owners about
0 fr. memcheck OK, static delta < 1 KB. Console benches for the user: starfx_hos01_prof, merlee_spell_prof, btldim_star_prof,
keymiss_kzn08_prof, keymiss_tattle_prof. hw_guidelines: no division on hit, no double, census cold on heap.

## Rules
CLAUDE.md rules 5, 7, 9, 11. PM_ROW=KEYMISS PM_LANE=KEYMISS on every build.sh and bench call. No git stash. Commit after each passed stage;
revert a stage whose picture does not change. At the start cap: INCOMPLETE, -ResetBudget KEYMISS, continue.

## At the end
Findings into the subject docs. Delete this row file in the final commit. SendMessage main "KEYMISS ready <hash>".
