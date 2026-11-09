# Handoff DIRECT (epic: the game calls the native producers directly)

STATE
- Branch DIRECT (worktree build3ds/wt/DIRECT), main c8d2c9118 merged (edce08314). Plan: plan_direct.md. Row file row_DIRECT.md stays (D4b to D9 open).
- Done: D0 frame stream, D1 model records and floor reflection ranges, D2 sprite records, D3 entity, item and animator records (merged as 9b8461838); D4a-1 load model struct NdTexM and pure SetTileSize windows tex_win0/tex_win1 (0d2816f5c); D4a-2 message glyph command NDC_GLYPH (ded6d74eb).
- D4a gates: byte-equal to the clean merge edce08314 (lane shots base4a): spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det; msgbox_talk_nowide_pic byte-equal to build-at 0d2816f5c; nd_host PASS; memcheck margin 314 KB.
- Expected console change of D4a: small, text scenes only (core 0 skips 10 to 16 word dispatches per glyph). Console list for the user: spot_mac01_prof, spot_mac00_prof, spot_sbk56_prof, starfx_hos01_prof (prof line `ndc ... gly`).
- Named fallbacks: records written inside build_custom_gfx (nd_rec_sub) stay in that list until D7; sprites the native draw refuses walk their payload words; a glyph with nd_rec_sub > 0 keeps its words.
- Next: D4b per plan_direct.md section 5 and 7 (nd_gbi.h redirect for the HUD, box, window and message files; grep audit of the group first). Not started in code.
- Tools: lane build with map_restore_table.c backup and restore (scratch b.sh pattern: gen_map_restore.py, PM_LANE=DIRECT build.sh, restore); base shots: run the gate benches on a clean lane build and rename the shots folder (base4a), or bench.py build-at <base> <bench>. nok11_det runs turbo, the others normal (a mixed list is refused).
