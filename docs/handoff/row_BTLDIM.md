# Row BTLDIM: lag while the battle background dims (Shooting Star)

## User facts
Trigger: in battle, use an item such as Shooting Star. The background dims; while the dim fades in and out, the game lags (O3DS XL).
The hold at full dim is fine. Reproducing scenario: add btldim_star first (step 1).

## Goal
Remove the per-frame sky texture re-upload during the shroud fade: dim the sky on the GPU (mix stage) instead of a new palette each frame.

## Start state
Worktree build3ds/wt/BTLDIM, branch BTLDIM from main 346c929d5. Copy assets/us and ver/us/build into it.

## Premise and its check (oracle)
Cause: src/battle/move/item/shooting_star.c:43-73 steps ShroudTintAmt 0,16..144 (darken) and 160..16,0 (lighten), 19 values.
src/background.c:189-250 lerps all 256 entries of gBackgroundPalette each frame when the amount is not 0; line 315 loads that TLUT.
3ds/src/pm_tex.c:1664-1730 hashes the palette (phash): each new value misses and goes to load_miss (1338-1608): CI8 decode
(tx1_build 1488), new VRAM slot (1430), linearAlloc and a 128-256 KB copy (1552-1561). Model records hit (MERLEE mask, pm_gpu.c:2816).
Check first = step 1: the btldim lines must show about 9 texture misses (darken) and 10 (lighten). If they show about 0, hand back.

## Touch only
src/battle/move/item/shooting_star.c (3DS test lines), 3ds/src/pm_warp.c (test_items near 1521: invitem=), src/battle/btl_states_menus.c
(one btlmenu line after btl_main_menu_init, about 2823), src/background.c (the fix, #ifdef TARGET_3DS), 3ds/tools/pad_btl_star.txt,
3ds/tools/azt_scenarios.json, 3ds/tools/bench_defs.json, 3ds/src/pm_tex.c (step 4), 3ds/src/nd_tex_set.c (step 4 fallback only).

## Plan
1. Test lines (commit 1): in shooting_star.c (#ifdef TARGET_3DS) copy the N(fade_line) helper of src/world/area_dro/dro_02/npc_merlee.c,
   add pm_tex_tot(&miss,&up) (pm_tex.c:1006). Print `btldim: darken start|darken done|lighten start|lighten done f%u t=%u ms mdlmiss %u
   clears %u texmiss %u texkb %u` at isInitialCall and before ApiStatus_DONE2 of CustomFadeBackgroundDarken/Lighten. pm_warp.c:
   `invitem=<hex>` sets gPlayerData.invItems[0], logs `invitem: slot 0 = 0x%03X` (ITEM_SHOOTING_STAR 0x083). btl_states_menus.c:
   `btlmenu: main open f%u`. Pad file (2 scans = 1 frame; wheel opens on Jump, Items is left): `wait<N>,left,wait20,A,wait20,A,wait20,A`.
   Scenarios (same commit as benches): btldim_star (azahar_stream: warp=kmr_03:0 story=ch0_toad_town invitem=0x083 battle battlef=100
   padscript=@pad_btl_star.txt loglevel=debug; first_marker `warp: kmr_03 entry 0`; until `btldim: lighten done f\d+`; cap 25),
   btldim_star_pic (shots shotwin=<darken start f>:14:2), btldim_star_prof (console, prof=30, no shots).
2. Fix in src/background.c (#ifdef TARGET_3DS, no toggle, the N64 #ifndef path unchanged):
   a) In the ENV_TINT_NONE/SHROUD branch (236-251): if (flags & BG_BLEND_SHOULD_LERP) and fogA != 255: bgMix = 1, do not write
      gBackgroundPalette. Keep fogA == 255 (black palette), the fog-only case and the depth/remap cases as they are.
   b) At 312-316: when bgMix load gGameStatusPtr->backgroundPalette (source palette, its slot hits).
   c) After gDPSetRenderMode(G_RM_NOOP, G_RM_NOOP2) at 308, when bgMix, in this order: gDPSetFogColor(fogR,fogG,fogB,fogA),
      gDPSetBlendColor(fogR,fogG,fogB,255), gDPSetRenderMode(PM_RM_SHROUD, G_RM_NOOP2). Order matters: the sync_render memo
      (nd_set.c:128) keys on other-mode words only. nd_mix1 (nd_set.c:120-149) gives P*A + M*(1-A); nd_rect.c:173 syncs per texrect.
   d) At the end of appendGfx_background_texture (both sky branches), when bgMix: gDPSetRenderMode(G_RM_NOOP, G_RM_NOOP2).
   e) A comment: no palette lerp, no decode/upload/VRAM slot per fade frame; 4 GBI words; no new .bss.
3. Only if step 2 still shows misses: ndmrw word 8 counts during the fade mean a render mode without shroudRm in model.c:1904-1961.
4. (oracle 2, after step 2 left 11 darken misses) Owner: EFFECT_RADIAL_SHIMMER type 1 (UseItem.inc.c:20/37), image D_09002E00 at
   0x4d7420, 32x32 I4 drawn as a 64x512 window (radial_shimmer.c:363). Bytes and scroll are stable. Suspect key field: tlut. set_tilesize
   (nd_tex_set.c) passes the last loaded palette even for non-CI tiles, and pm_tex keys on it. Check first: in the texmisslog block of
   load_miss (pm_tex.c about 1593) print `texmissk f%u img %p tlut %p/%p st %d/%d dk %08x/%08x ph %08x/%08x` against the first live slot
   with the same img. Expect only the tlut pair to differ. Fix: in pm_tex_load (about 1740) before load_impl:
   `if (fmt != FMT_CI) { tlut = NULL; tlut_n = 0; }` plus a comment. Do not touch pm_tex_load_shaded or pm_tex_load_rgba8. Pass: the
   shimmer misses once (about f405), darken texmiss about 1, lighten I-format misses fall, the byte gates stay equal. Fallback if the
   stride pair differs: the V12c ldrec loop in set_tilesize (about 165-171) uses only ldrec entries written since the last set_timg.
Out of scope (note in commit): FOG_SHROUD class (model.c:2052-2065) on fog stages.

## Done criteria
1. nd_host PASS. memcheck OK.
2. Byte gates (no-fade path equal): battle_start_f182_cold and nok11_det 0000-0015 byte-equal to build-at 346c929d5.
3. btldim_star: texmiss about 0 over darken and lighten (before about 19), model misses unchanged, clears 0.
4. btldim_star_pic: the sky and stage darken step by step, Mario and the Goomba stay bright, no seams or palette noise.
Affected scenarios (no others): btldim_star, btldim_star_pic, battle_start_f182_cold, nok11_det.

## Rules
CLAUDE.md rules 5, 7, 9, 11. PM_LANE=BTLDIM on every build.sh and bench call. No git stash. Commit after each passed step.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=BTLDIM PM_LANE=BTLDIM sh 3ds/build.sh; restore 3ds/src/map_restore_table.c.
- Test: PM_ROW=BTLDIM PM_LANE=BTLDIM python 3ds/tools/bench.py run <bench> --row BTLDIM --lane BTLDIM --why "<text>"
- Budget reset: powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget BTLDIM

## At the end
Delete this row file in the commit that completes the row. SendMessage main "BTLDIM ready <hash>".
